#ifndef ROOMWINDOW_HPP
#define ROOMWINDOW_HPP

#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QPushButton>
#include <QTcpSocket>
#include <QTextEdit>
#include <QWidget>

class RoomWindow : public QWidget {
    Q_OBJECT
   public:
    explicit RoomWindow(QWidget* parent = nullptr);
    ~RoomWindow();

    void resizeEvent(QResizeEvent* ev) override;
    void show();

    bool connectToServer();

    QString getUserName();
    QString getRoomId();
    QString getServerAddress();

    void setUserName(const QString& str);
    void setRoomId(const QString& str);
    void setServerAddress(const QString& str);
    void updateWindowTitle();

   private:
    void ui_setupGeometry();
    void ui_loadContents();

    QString m_userName;
    QString m_roomId;
    QString m_serverAddress;

    QTextEdit* m_textMessages;
    QLineEdit* m_lineMessage;
    QListWidget* m_listUsers;
    QListWidget* m_listFiles;
    QPushButton* m_pushButtonSendMessage;
    QPushButton* m_pushButtonSendFile;
    QPushButton* m_pushButtonDisconnect;

    QTcpSocket* m_clientSocket;
    bool m_clientSocketDisconnected;

   public slots:
    void pushButtonSendMessage_clicked();
    void pushButtonSendFile_clicked();
    void pushButtonDisconnect_clicked();

    void readyRead();
    void connected();

   signals:
    void opened();
    void closed();
};

#endif  // ROOMWINDOW_HPP
