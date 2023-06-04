#include "server.hpp"

#include <QDebug>
#include <QRandomGenerator>
#include <QRegExp>
#include <QThread>
#include <QTime>

#define LOG_CALL() qDebug().nospace() << __PRETTY_FUNCTION__ << " call"

Server::Server(int _port, QObject* parent) : QTcpServer(parent) {
    LOG_CALL();

    QHostAddress address = QHostAddress::AnyIPv4;

    if (this->listen(address, _port) == false) {
        qDebug() << "Could not listen at address" << address.toString() << "on port" << _port;
        exit(EXIT_FAILURE);
    }

    qDebug() << "Server: listening at address" << address.toString() << "on port" << _port;
}

Server::~Server() {
    LOG_CALL();

    this->deleteLater();
}

void Server::incomingConnection(qintptr socketDescriptor) {
    LOG_CALL();

    QTcpSocket* client = new QTcpSocket(this);
    client->setSocketDescriptor(socketDescriptor);

    this->connect(client, SIGNAL(readyRead()), this, SLOT(readyRead()));
    this->connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));

    clients.insert(client);

    qDebug() << "New client: incoming connection from" << client->peerAddress().toString();
}

QString Server::generateNewRoomId() {
    QRandomGenerator generator(QDateTime::currentSecsSinceEpoch());

    QString allowedChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString str;

    while (str.size() != 10) {
        auto r = generator.generate() % allowedChars.size();
        str += allowedChars[r];
    }

    return str;
}

void Server::readyRead() {
    LOG_CALL();

    QTcpSocket* client;
    QString userName;
    QString line;

    /* /command room:data */
    QRegExp regex("^/([a-z]+) ([a-zA-Z0-9]+):(.*)$");
    QString command;
    QString roomId;
    QString data;

    client = (QTcpSocket*) sender();

    while (client->canReadLine()) {
        line = QString::fromUtf8(client->readLine().trimmed());

        qDebug() << "Got line from" << client->peerAddress().toString() << ":\n" << line;

        if (regex.indexIn(line) != -1) {
            // Got new command

            command = regex.cap(1);
            roomId = regex.cap(2);
            data = regex.cap(3).trimmed();

            if (command == "join") {
                // New user in room

                if (roomId == "new") {
                    do {
                        roomId = generateNewRoomId();
                    } while (users.contains(roomId));

                    client->write(QString("/roomid" + roomId + ":.").toUtf8());
                    QThread::msleep(1000);
                }

                if (!users.contains(roomId)) users.insert(roomId, userMap());

                userName = data;
                users[roomId][client] = userName;

                for (const auto [clientInRoom, value] : users[roomId].asKeyValueRange()) {
                    clientInRoom->write(QString("Server: " + userName + " has joined.\n").toUtf8());
                }

                sendUserList(roomId);
            } else if (command == "message") {
                // New message from client

                userName = users[roomId][client];

                for (const auto [clientInRoom, value] : users[roomId].asKeyValueRange()) {
                    clientInRoom->write(QString(userName + ":" + data + "\n").toUtf8());
                }

                qDebug() << "User" << userName << "sent new message:\n" << data;
            }
        } else if (users[roomId].contains(client)) {
            // Part of the message

            userName = users[roomId][client];

            for (const auto [clientInRoom, value] : users[roomId].asKeyValueRange()) {
                clientInRoom->write(QString(userName + ':' + line + "\n").toUtf8());
            }

            qDebug() << "User" << userName << "sent message:\n" << line;
        } else {
            qWarning() << "Client" << client->peerAddress().toString() << "sent bad message";
        }
    }
}

void Server::disconnected() {
    LOG_CALL();

    QTcpSocket* client = (QTcpSocket*) sender();
    QString userName("unknown");
    QString fromRoomId("unknown?");

    for (auto [roomId, usersInRoom] : users.asKeyValueRange()) {
        if (usersInRoom.contains(client)) {
            fromRoomId = roomId;
            userName = usersInRoom[client];

            usersInRoom.remove(client);
            break;
        }
    }

    clients.remove(client);
    qDebug() << "Client disconnected:" << client->peerAddress().toString();

    sendUserList(fromRoomId);

    if (fromRoomId != "unknown?") {
        for (const auto [clientInRoom, value] : users[fromRoomId].asKeyValueRange()) {
            clientInRoom->write(QString("Server: " + userName + " has left.\n").toUtf8());
        }
    }

    delete client;
}

void Server::sendUserList(QString roomId) {
    LOG_CALL();

    QStringList userList;

    foreach (const auto& userName, users[roomId].values()) {
        userList.append(userName);
    }

    for (const auto [clientInRoom, value] : users[roomId].asKeyValueRange()) {
        clientInRoom->write(QString("/users: " + userList.join(',') + "\n").toUtf8());
    }
}
