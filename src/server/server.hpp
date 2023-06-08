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

    // Messages
    void sendTextMessage(const QString& userName, const QString& roomId, const QString& msg);

    // Users
    void sendUserList(roomId roomId);

    // Files
    void sendFileList(roomId roomId, QTcpSocket* client = nullptr);
    void receiveFile(const QString& userName, QString& filename, const QString& roomId,
                     const QString& base64_data);
    void sendFile(const QString& userName, const QString& filename, const QString& roomId,
                  QTcpSocket* client);

    // Rooms
    void processJoinRoom(QString& userName, QString& roomId, QTcpSocket* client);
    QString generateNewRoomId();

    QSet<QTcpSocket*> clients;
    QMap<roomId, userMap> users;
    QMap<roomId, QSet<QString>> files;

   public slots:
    void readyRead();
    void disconnected();
};

#endif  // SERVER_HPP
