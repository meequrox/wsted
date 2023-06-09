#include "logger.hpp"

void messageLogger(const QString& action, const QString& clientName, const QString& msg) {
    QString actionLower;
    QString borderString;
    QTime time;
    QString where;

    actionLower = action.toLower();
    borderString = QString(8, '-');
    time = QDateTime().currentDateTime().time();

    if (actionLower.startsWith("sent")) {
        where = "to";
    } else if (actionLower.startsWith("received") || actionLower.startsWith("bad")) {
        where = "from";
    } else {
        where = "?";
    }

    qDebug().noquote().nospace() << borderString << time.toString() << borderString;
    qDebug().noquote().nospace() << action << " message " << where + ' ' << clientName << ":\n'"
                                 << msg.trimmed() << "'";
    qDebug().noquote().nospace() << borderString << borderString << borderString << Qt::endl;
}

void messageLogger(const QString& action, const QTcpSocket* socket, const QString& msg) {
    if (socket) {
        messageLogger(action, socket->peerAddress().toString(), msg);
    }
}
