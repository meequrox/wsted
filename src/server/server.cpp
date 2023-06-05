#include "server.hpp"

#include <QDebug>
#include <QRandomGenerator>
#include <QRegExp>
#include <QThread>
#include <QTime>

#include "../logger.hpp"

Server::Server(int _port, QObject* parent) : QTcpServer(parent) {
    LOG_CALL();

    QHostAddress address = QHostAddress::AnyIPv4;

    if (this->listen(address, _port) == false) {
        qDebug() << "Could not listen at address" << address.toString() << "on port" << _port;
        exit(EXIT_FAILURE);
    }

    qDebug() << "Server: listening at address" << address.toString() << "on port" << _port;
}

Server::~Server() { LOG_CALL(); }

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
    QString newRoomId;

    do {
        newRoomId.clear();

        for (; newRoomId.size() != 10;) {
            auto r = generator.generate() % allowedChars.size();
            newRoomId += allowedChars[r];
        }
    } while (users.contains(newRoomId));

    return newRoomId;
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
        messageLogger("Received", client, line);

        if (regex.indexIn(line) != -1) {
            // Got new command

            command = regex.cap(1);
            roomId = regex.cap(2);
            data = regex.cap(3);

            if (command == "join") {
                // New user in room

                userName = data;

                if (roomId == "new") {
                    roomId = generateNewRoomId();

                    messageToWrite = "/roomid " + roomId + ":" + data + '\n';
                    client->write(messageToWrite.toUtf8());
                    messageLogger("Sent", client, messageToWrite);

                    for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                        messageToWrite = "Server: " + userName + " has joined.\n";
                        clientInRoom->write(messageToWrite.toUtf8());
                    }
                }

                if (!users.contains(roomId)) {
                    users.insert(roomId, userMap());
                } else {
                    bool isUserNameFree = false;

                    while (!isUserNameFree) {
                        isUserNameFree = true;

                        foreach (const auto& clientUserName, users[roomId]) {
                            if (clientUserName == userName) {
                                isUserNameFree = false;
                                userName += "-1";

                                qDebug() << "Duplicate username" << clientUserName << ", changing to"
                                         << userName;
                                break;
                            }
                        }
                    }
                }

                users[roomId][client] = userName;

                messageToWrite = "/userid " + roomId + ':' + userName + '\n';
                client->write(messageToWrite.toUtf8());
                messageLogger("Sent", userName, messageToWrite);

                for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                    messageToWrite = "Server: " + userName + " has joined.\n";
                    clientInRoom->write(messageToWrite.toUtf8());
                }

                sendUserList(roomId);
            } else if (command == "message") {
                // New message from client

                userName = users[roomId][client];

                for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                    messageToWrite = userName + ":" + data + '\n';
                    clientInRoom->write(messageToWrite.toUtf8());
                }
            }
        } else if (users[roomId].contains(client)) {
            // Part of the message

            userName = users[roomId][client];

            for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                messageToWrite = userName + ':' + line + '\n';
                clientInRoom->write(messageToWrite.toUtf8());
            }
        } else {
            messageLogger("Bad", client, line);
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
    client->deleteLater();

    if (fromRoomId != "unknown?") {
        qDebug() << "This client was in room" << fromRoomId << '\n';

        for (const auto [clientInRoom, clientUserName] : users[fromRoomId].asKeyValueRange()) {
            messageToWrite = "Server: " + userName + " has left.\n";
            clientInRoom->write(messageToWrite.toUtf8());
        }

        sendUserList(fromRoomId);
    } else {
        qDebug() << "This client was not in any room\n";
    }
}

void Server::sendUserList(QString roomId) {
    LOG_CALL();

    QStringList userList;
    QString message;

    foreach (const auto& userName, users[roomId].values()) {
        userList.append(userName);
    }

    message = "/users " + roomId + ":" + userList.join(',') + '\n';

    for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
        clientInRoom->write(message.toUtf8());
    }
}
