#include "logger.hpp"

void messageLogger(const QString& action, const QString& clientName, const QString& msg) {
    QString borderString;
    QDateTime dateTime;
    QString where;

    borderString = QString(8, '-');
    dateTime = QDateTime().currentDateTime();

    if (action.toLower() == "sent") {
        where = "to";
    } else if (action.toLower() == "received" || action.toLower() == "bad") {
        where = "from";
    } else {
        where = "?";
    }

    qDebug().noquote().nospace() << borderString << dateTime.time().toString() << borderString;
    qDebug().noquote().nospace() << action << " message " << where + ' ' << clientName << ":\n'"
                                 << msg.trimmed() << "'";
    qDebug().noquote().nospace() << borderString << borderString << borderString << Qt::endl;
}

void messageLogger(const QString& action, const QTcpSocket* socket, const QString& msg) {
    if (socket) {
        messageLogger(action, socket->peerAddress().toString(), msg);
    }
}
