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

    // Messages
    void receiveTextMessage(const QString& msg);

    // Users
    void setUserList(const QString& separatedString);

    // Files
    void setFileList(const QString& separatedString);
    void receiveFile(QString& fileName, const QString& base64_data, const QString& outputDir);

    QString m_userName;
    QString m_roomId;
    QString m_serverAddress;

    QTcpSocket* m_clientSocket;
    bool m_clientSocketDisconnected;

    // Messages
    QTextEdit* m_textMessages;
    QLineEdit* m_lineMessage;
    QPushButton* m_pushButtonSendMessage;

    // Users
    QListWidget* m_listUsers;

    // Files
    QListWidget* m_listFiles;
    QAction* m_actionDownload;
    QPushButton* m_pushButtonSendFile;

    // Disconnect
    QPushButton* m_pushButtonDisconnect;

   public slots:
    void actionDownload_triggered();
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
