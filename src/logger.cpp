#include "logger.hpp"

void messageLogger(const QString& action, const QString& clientName, const QString& msg) {
    QString borderString;
    QDateTime dateTime;

    borderString = QString(6, '-');
    dateTime = QDateTime();

    qDebug() << borderString << dateTime.time() << borderString;
    qDebug() << action << "message from" << clientName << ":\n" << msg;
    qDebug() << borderString << borderString << borderString + '\n';
}

void messageLogger(const QString& action, const QTcpSocket* socket, const QString& msg) {
    if (socket) {
        messageLogger(action, socket->peerAddress().toString(), msg);
    }
}
