#ifndef SERVER_HPP
#define SERVER_HPP

#include <QFile>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

typedef QMap<QTcpSocket*, QString> userMap;
typedef QString roomId;

class Server : public QTcpServer {
    Q_OBJECT
   public:
    explicit Server(int _port, QObject* parent = nullptr);
    ~Server();

   private:
    void incomingConnection(qintptr fd);
    void sendUserList(roomId roomId);
    void sendFileList(roomId roomId, QTcpSocket* client = nullptr);

    QString generateNewRoomId();

    QSet<QTcpSocket*> clients;
    QMap<roomId, userMap> users;
    QMap<roomId, QSet<QString>> files;

   public slots:
    void readyRead();
    void disconnected();
};

#endif  // SERVER_HPP
