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
    QString messageToWrite;

    /* /command room:data */
    QRegExp regex("^/([a-z]+) ([a-zA-Z0-9]+):(.*)$");
    QString command;
    QString roomId;
    QString data;

    client = (QTcpSocket*) sender();

    while (client->bytesAvailable()) {
        line = QString::fromUtf8(client->readLine().trimmed());
        qDebug() << "Received message from client" << client->peerAddress().toString() << ":\n" << line;

        if (regex.indexIn(line) != -1) {
            // Got new command

            command = regex.cap(1);
            roomId = regex.cap(2);
            data = regex.cap(3);

            if (command == "join") {
                // New user in room

                userName = data;

                if (roomId == "new") {
                    do {
                        roomId = generateNewRoomId();
                    } while (users.contains(roomId));

                    messageToWrite = "/roomid " + roomId + ":" + data + "\n";
                    client->write(messageToWrite.toUtf8());

                    qDebug() << "Send message to user" << userName << ":\n" << messageToWrite;

                    QThread::msleep(500);
                    for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                        messageToWrite = "Server: " + userName + " has joined.\n";
                        clientInRoom->write(messageToWrite.toUtf8());

                        qDebug() << "Send message to user" << clientUserName << ":\n" << messageToWrite;
                    }
                }

                if (!users.contains(roomId)) users.insert(roomId, userMap());
                users[roomId][client] = userName;

                for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                    messageToWrite = "Server: " + userName + " has joined.\n";
                    clientInRoom->write(messageToWrite.toUtf8());

                    qDebug() << "Send message to user" << clientUserName << ":\n" << messageToWrite;
                }

                sendUserList(roomId);
            } else if (command == "message") {
                // New message from client

                userName = users[roomId][client];

                for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                    messageToWrite = userName + ":" + data + "\n";
                    clientInRoom->write(messageToWrite.toUtf8());

                    qDebug() << "Send message to user" << clientUserName << ":\n" << messageToWrite;
                }

                qDebug() << "User" << userName << "sent new message:\n" << data;
            }
        } else if (users[roomId].contains(client)) {
            // Part of the message

            userName = users[roomId][client];

            for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                messageToWrite = userName + ':' + line + "\n";
                clientInRoom->write(messageToWrite.toUtf8());

                qDebug() << "Send message to user" << clientUserName << ":\n" << messageToWrite;
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
    QString messageToWrite;

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

    qDebug() << "Client disconnected:" << client->peerAddress().toString();

    clients.remove(client);
    // delete client;

    if (fromRoomId != "unknown?") {
        qDebug() << "This client was in room" << fromRoomId << "\n";

        for (const auto [clientInRoom, clientUserName] : users[fromRoomId].asKeyValueRange()) {
            messageToWrite = "Server: " + userName + " has left.\n";
            clientInRoom->write(messageToWrite.toUtf8());

            qDebug() << "Send message to user" << clientUserName << ":\n" << messageToWrite;
        }

        sendUserList(fromRoomId);
    } else {
        qDebug() << "This client was not in any room"
                 << "\n";
    }
}

void Server::sendUserList(QString roomId) {
    LOG_CALL();

    QStringList userList;
    QString message;

    foreach (const auto& userName, users[roomId].values()) {
        userList.append(userName);
    }

    message = "/users " + roomId + ":" + userList.join(',') + "\n";

    for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
        clientInRoom->write(message.toUtf8());
        qDebug() << "Sent message to user" << clientUserName << ":\n" << message;
    }
}