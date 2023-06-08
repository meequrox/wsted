#include "server.hpp"

#include <QDebug>
#include <QDir>
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

    QString tmpAppPath = "/tmp/wsted/";
    QDir dir(tmpAppPath);
    qDebug().nospace() << "Removed directory " << tmpAppPath << ": " << dir.removeRecursively();

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

    QRegExp messageRegex("^/([a-z]+) ([a-zA-Z0-9]+):(.*)$");  // /command room:data
    QString command;

    QRegExp fileRegex("^/([a-z]+) '(.*)' ([a-zA-Z0-9]+):(.*)$");  // /command filename room:data
    QString filename;

    QString roomId;
    QString data;

    client = (QTcpSocket*) sender();

    while (client->canReadLine()) {
        line = QString::fromUtf8(client->readLine().trimmed());

        if (messageRegex.indexIn(line) != -1) {
            // New message from client

            command = messageRegex.cap(1);
            roomId = messageRegex.cap(2);
            data = messageRegex.cap(3);

            if (command == "join") {
                // New user in room

                messageLogger("Received JOIN", client, line);

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

                QString tmpRoomPath = "/tmp/wsted/" + roomId + "/";
                QDir dir(tmpRoomPath);

                qDebug().nospace() << "Created directory " << tmpRoomPath << ": "
                                   << dir.mkpath(tmpRoomPath);

                users[roomId][client] = userName;

                messageToWrite = "/userid " + roomId + ':' + userName + '\n';
                client->write(messageToWrite.toUtf8());
                messageLogger("Sent", userName, messageToWrite);

                for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                    messageToWrite = "Server: " + userName + " has joined.\n";
                    clientInRoom->write(messageToWrite.toUtf8());
                }

                sendUserList(roomId);
                sendFileList(roomId, client);
            } else if (command == "message") {
                // New text message from client

                messageLogger("Received TEXT", client, line);

                userName = users[roomId][client];

                for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                    messageToWrite = userName + ":" + data + '\n';
                    clientInRoom->write(messageToWrite.toUtf8());
                }
            }
        } else if (fileRegex.indexIn(line) != -1) {
            // New file from client

            command = fileRegex.cap(1);
            filename = fileRegex.cap(2);
            roomId = fileRegex.cap(3);
            data = fileRegex.cap(4);

            if (command == "sendfile" && !filename.isEmpty() && !data.isEmpty()) {
                messageLogger("Received FILE", client,
                              '/' + command + " '" + filename + "' " + roomId + ":_BASE64_DATA_");

                QString tmpRoomPath = "/tmp/wsted/" + roomId + "/";

                QDir dir;
                if (!dir.mkpath(tmpRoomPath)) {
                    qDebug() << "Failed to create path" << tmpRoomPath;
                    return;
                }

                QFile file(tmpRoomPath + filename);

                while (file.exists()) {
                    qDebug() << "Duplicate filename" << filename;

                    auto idx = filename.lastIndexOf('.');
                    if (idx != -1) {
                        filename = filename.mid(0, idx) + "-1" + filename.mid(idx);
                    } else {
                        filename = filename + "-1";
                    }

                    file.setFileName(tmpRoomPath + filename);

                    qDebug() << "Changing to" << filename;
                }

                if (!file.open(QIODevice::Append, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
                    qDebug() << file.fileName() << file.errorString();
                    return;
                }

                file.write(QByteArray::fromBase64(data.toUtf8()));
                file.close();

                messageLogger("Received FILE", client, filename);
                files[roomId].insert(filename);

                userName = users[roomId][client];

                for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
                    messageToWrite = "Server: " + userName + " has uploaded file '" + filename + "'.\n";
                    clientInRoom->write(messageToWrite.toUtf8());
                }

                sendFileList(roomId);
            }
        } else {
            messageLogger("Received BAD", client, line);
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

        if (users[fromRoomId].size() == 0) {
            users.remove(fromRoomId);
            files.remove(fromRoomId);

            QString tmpRoomPath = "/tmp/wsted/" + fromRoomId + "/";
            QDir dir(tmpRoomPath);

            qDebug().nospace() << "Removed directory " << tmpRoomPath << ": " << dir.removeRecursively();
            qDebug() << "Deleted room" << fromRoomId << "(no more users in room)" << '\n';
        } else {
            for (const auto [clientInRoom, clientUserName] : users[fromRoomId].asKeyValueRange()) {
                messageToWrite = "Server: " + userName + " has left.\n";
                clientInRoom->write(messageToWrite.toUtf8());
            }

            sendUserList(fromRoomId);
        }
    } else {
        qDebug() << "This client was not in any room\n";
    }
}

void Server::sendUserList(roomId roomId) {
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

void Server::sendFileList(roomId roomId, QTcpSocket* client) {
    LOG_CALL();

    QStringList fileList;
    QString message;

    foreach (const auto& fileName, files[roomId]) {
        fileList.append(fileName);
    }

    message = "/files " + roomId + ":" + fileList.join('/') + '\n';

    if (client) {
        client->write(message.toUtf8());
    } else {
        for (const auto [clientInRoom, clientUserName] : users[roomId].asKeyValueRange()) {
            clientInRoom->write(message.toUtf8());
        }
    }
}
