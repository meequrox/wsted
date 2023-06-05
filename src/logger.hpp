#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QDebug>
#include <QString>
#include <QTcpSocket>

#define LOG_CALL() qDebug().nospace() << __PRETTY_FUNCTION__ << " call" << Qt::endl;

void messageLogger(const QString& action, const QString& clientName, const QString& msg);
void messageLogger(const QString& action, const QTcpSocket* socket, const QString& msg);

#endif
