#include "roomwindow.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegExp>
#include <QScreen>
#include <QThread>

#include "../logger.hpp"

#define DEFAULT_PORT 8044

static QSize getDefaultWindowSize() {
    const QSize screenSize = QApplication::primaryScreen()->size();
    const qreal screenRatio = QApplication::primaryScreen()->devicePixelRatio();

    QSize s;
    s.setWidth((screenSize.width() * screenRatio) / 3);
    s.setHeight((screenSize.height() * screenRatio) / 2);

    return s;
}

RoomWindow::RoomWindow(QWidget* parent) : QWidget(parent), m_clientSocketDisconnected(false) {
    // Messages
    m_textMessages = new QTextEdit(this);
    m_lineMessage = new QLineEdit(this);
    m_pushButtonSendMessage = new QPushButton(this);

    // Users
    m_listUsers = new QListWidget(this);

    // Files
    m_listFiles = new QListWidget(this);
    m_actionDownload = new QAction(this);
    m_pushButtonSendFile = new QPushButton(this);

    // Disconnect
    m_pushButtonDisconnect = new QPushButton(this);

    setMinimumSize(480, 320);
    resize(getDefaultWindowSize());

    ui_setupGeometry();
    ui_loadContents();

    m_clientSocket = new QTcpSocket();
    connect(m_clientSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(m_clientSocket, SIGNAL(connected()), this, SLOT(connected()));
    connect(m_clientSocket, SIGNAL(disconnected()), this, SLOT(pushButtonDisconnect_clicked()));
}

void RoomWindow::ui_setupGeometry() {
    // Window
    setWindowFlag(Qt::WindowFullscreenButtonHint, false);
    qDebug() << "Window" << size();

    // Messages
    m_textMessages->setGeometry(QRect(0, 0, size().width() * 0.75, size().height() * 0.93));
    m_lineMessage->setGeometry(
        QRect(0, m_textMessages->height(), size().width() * 2 / 3, size().height() * 0.07));
    m_pushButtonSendMessage->setGeometry(QRect(m_lineMessage->width(), m_textMessages->height(),
                                               size().width() * 0.25 / 3, size().height() * 0.07));

    // Users
    m_listUsers->setGeometry(
        QRect(m_textMessages->width(), 0, size().width() * 0.25, m_textMessages->height() * 0.50));

    // Disconnect
    m_pushButtonDisconnect->setGeometry(QRect(m_textMessages->width(), m_pushButtonSendMessage->y(),
                                              size().width() * 0.25, size().height() * 0.07));

    // Files
    m_pushButtonSendFile->setGeometry(QRect(
        m_pushButtonDisconnect->x(), m_pushButtonDisconnect->y() - m_pushButtonDisconnect->height() - 1,
        m_pushButtonDisconnect->width(), m_pushButtonDisconnect->height()));
    m_listFiles->setGeometry(QRect(m_textMessages->width(), m_listUsers->height(), m_listUsers->width(),
                                   m_listUsers->height() - m_pushButtonSendFile->height() - 2));
}

void RoomWindow::ui_loadContents() {
    // Window
    setWindowTitle("Connecting...");
    setWindowIcon(QIcon(":/icons/app"));

    // Messages
    m_textMessages->setStyleSheet(
        "color:white;border:0px;border-left:1px solid white;border-radius:1px");
    m_textMessages->setReadOnly(true);
    m_textMessages->setText("");

    m_lineMessage->setStyleSheet(
        "color:white;border:1px solid white;border-radius:1px;border-right:0px");
    m_lineMessage->setClearButtonEnabled(true);
    m_lineMessage->setText("");
    m_lineMessage->setMaxLength(2048 - 8 - 10 - 3);
    m_lineMessage->setPlaceholderText("Type here");
    connect(m_lineMessage, SIGNAL(returnPressed()), SLOT(pushButtonSendMessage_clicked()));

    m_pushButtonSendMessage->setStyleSheet("color:white;border:1px solid white;border-radius:1px");
    m_pushButtonSendMessage->setText("Say");
    m_pushButtonSendMessage->setDefault(true);
    connect(m_pushButtonSendMessage, SIGNAL(clicked()), SLOT(pushButtonSendMessage_clicked()));

    // Users
    m_listUsers->setStyleSheet(
        "color:white;border:0px;border-left:1px solid white;border-right:1px solid "
        "white;border-radius:1px");
    m_listUsers->clear();

    // Files
    m_listFiles->setStyleSheet("color:white;border:1px solid white;border-radius:1px");
    m_listFiles->clear();
    m_listFiles->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_listFiles->insertAction(nullptr, m_actionDownload);

    m_actionDownload->setText("Download");
    connect(m_actionDownload, SIGNAL(triggered()), SLOT(actionDownload_triggered()));

    m_pushButtonSendFile->setStyleSheet(m_pushButtonSendMessage->styleSheet());
    m_pushButtonSendFile->setText("Upload file");
    connect(m_pushButtonSendFile, SIGNAL(clicked()), SLOT(pushButtonSendFile_clicked()));

    // Disconnect
    m_pushButtonDisconnect->setStyleSheet(m_pushButtonSendMessage->styleSheet());
    m_pushButtonDisconnect->setText("Disconnect");
    connect(m_pushButtonDisconnect, SIGNAL(clicked()), SLOT(pushButtonDisconnect_clicked()));
}

void RoomWindow::receiveTextMessage(const QString& msg) {
    QString userName;
    QString message;
    QString timeString;

    auto firstSpaceIdx = msg.indexOf(' ');
    timeString = msg.mid(0, firstSpaceIdx);

    auto delimiterColonIdx = firstSpaceIdx + msg.mid(firstSpaceIdx).indexOf(':');
    userName = msg.mid(firstSpaceIdx + 1, delimiterColonIdx - firstSpaceIdx - 1);
    message = msg.mid(delimiterColonIdx + 1);

    m_textMessages->append("<i>" + timeString + "</i> <b>" + userName + "</b>: " + message);
}

void RoomWindow::setUserList(const QString& separatedString) {
    QStringList userList = separatedString.split(',');

    m_listUsers->clear();
    for (const auto& user : userList) {
        m_listUsers->addItem(user);
    }
}

void RoomWindow::setFileList(const QString& separatedString) {
    QStringList fileList = separatedString.split('/');

    m_listFiles->clear();
    for (const auto& fileName : fileList) {
        m_listFiles->addItem(fileName);
    }
}

void RoomWindow::receiveFile(QString& fileName, const QString& base64_data, const QString& outputDir) {
    QDir dir;
    if (!dir.mkpath(outputDir)) {
        qDebug() << "Failed to create path" << outputDir;
        return;
    }

    QFile file(outputDir + '/' + fileName);

    while (file.exists()) {
        qDebug() << "Duplicate filename" << fileName;

        auto idx = fileName.lastIndexOf('.');
        if (idx != -1) {
            fileName = fileName.mid(0, idx) + "-1" + fileName.mid(idx);
        } else {
            fileName = fileName + "-1";
        }

        file.setFileName(outputDir + '/' + fileName);

        qDebug() << "Changing to" << fileName;
    }

    if (!file.open(QIODevice::Append, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        qDebug() << file.fileName() << file.errorString();
        return;
    }

    file.write(QByteArray::fromBase64(base64_data.toUtf8()));
    file.close();
}

void RoomWindow::actionDownload_triggered() {
    QString fileName;
    QString message;

    fileName = m_listFiles->currentItem()->text();

    if (!fileName.isEmpty()) {
        message = "/getfile '" + fileName + "' " + m_roomId + ":." + '\n';

        m_clientSocket->write(message.toUtf8());
        messageLogger("Sent", m_clientSocket, message);
    }
}

void RoomWindow::pushButtonSendMessage_clicked() {
    QString message = m_lineMessage->text().trimmed();

    if (!message.isEmpty()) {
        message = "/msg " + m_roomId + ":" + message + '\n';

        m_clientSocket->write(message.toUtf8());
        messageLogger("Sent", m_clientSocket, message);
    }

    m_lineMessage->clear();
    m_lineMessage->setFocus();
}

void RoomWindow::pushButtonSendFile_clicked() {
    QString filePath;
    QString fileName;
    QString messageToWrite;

    filePath = QFileDialog::getOpenFileName(this);
    if (filePath.isEmpty()) {
        qDebug() << "No file has been chosen for upload";
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << file.fileName() << file.errorString();
        return;
    }

    if (file.size() > (1 << 20) * 512) {
        messageToWrite =
            "The size of the selected file is larger than allowed (512 MiB), the process is aborted.";
        qDebug() << messageToWrite;

        QMessageBox::warning(this, "Upload file", messageToWrite, QMessageBox::Close,
                             QMessageBox::Close);

        file.close();
        return;
    }

    fileName = "'" + filePath.mid(filePath.lastIndexOf('/') + 1).replace('\'', '_') + "'";
    messageToWrite = "/sendfile " + fileName + ' ' + m_roomId + ':' + file.readAll().toBase64() + '\n';

    m_clientSocket->write(messageToWrite.toUtf8());
    messageLogger("Sent FILE", m_clientSocket,
                  messageToWrite.mid(0, messageToWrite.indexOf(':') + 1) + "_BASE64_DATA_");
}

void RoomWindow::pushButtonDisconnect_clicked() {
    if (m_clientSocketDisconnected) return;

    m_clientSocketDisconnected = true;
    m_clientSocket->disconnectFromHost();

    if (m_clientSocket->state() == QAbstractSocket::UnconnectedState ||
        m_clientSocket->waitForDisconnected(10000)) {
        qDebug() << "Disconnected!";
    }

    m_textMessages->clear();
    m_lineMessage->clear();
    m_listUsers->clear();
    m_listFiles->clear();

    emit closed();
    close();
}

void RoomWindow::readyRead() {
    QString line;

    QRegExp messageRegex("^/([a-z]+) ([a-zA-Z0-9]+):(.*)$");  // /command room:data
    QString command;

    QRegExp fileRegex("^/([a-z]+) '(.*)' ([a-zA-Z0-9]+):(.*)$");  // /command filename room:data
    QString filename;

    QString roomId;
    QString data;

    while (m_clientSocket->canReadLine()) {
        line = QString::fromUtf8(m_clientSocket->readLine().trimmed());

        if (messageRegex.indexIn(line) != -1) {
            // Message from server

            command = messageRegex.cap(1);
            roomId = messageRegex.cap(2);
            data = messageRegex.cap(3);

            if (command == "roomid") {
                // Room ID for client from server
                setRoomId(roomId);

                messageLogger("Received ROOM_ID", m_clientSocket, line);
            } else if (command == "userid") {
                // Username for client from server
                setUserName(data);

                messageLogger("Received USER_ID", m_clientSocket, line);
            } else if (command == "users" && roomId == m_roomId) {
                // User list from server
                setUserList(data);

                messageLogger("Received USER_LIST", m_clientSocket, line);
            } else if (command == "files" && roomId == m_roomId) {
                // File list from server
                setFileList(data);

                messageLogger("Received FILE_LIST", m_clientSocket, line);
            }
        } else if (fileRegex.indexIn(line) != -1) {
            // File from server

            command = fileRegex.cap(1);
            filename = fileRegex.cap(2);
            roomId = fileRegex.cap(3);
            data = fileRegex.cap(4);

            if (command == "sendfile" && !filename.isEmpty() && !data.isEmpty()) {
                // File contents from server

                QString downloadRoomPath = QString(getenv("HOME")) + "/Downloads/" + roomId;
                receiveFile(filename, data, downloadRoomPath);

                m_textMessages->append("Downloaded file <b>'" + filename + "'</b> to <b>" +
                                       downloadRoomPath + "</b>");

                messageLogger("Received FILE", m_clientSocket, filename);
            }
        } else if (line.contains(':')) {
            // Text message from server
            receiveTextMessage(line);

            messageLogger("Received TEXT", m_clientSocket, line);
        } else {
            messageLogger("Received BAD", m_clientSocket, line);
        }
    }
}

void RoomWindow::connected() {
    QString message = "/join " + m_roomId + ':' + m_userName + '\n';

    QThread::msleep(10);

    m_clientSocket->write(message.toUtf8());
    messageLogger("Sent", m_clientSocket, message);
}

void RoomWindow::resizeEvent(QResizeEvent* ev) {
    QWidget::resizeEvent(ev);
    ui_setupGeometry();
}

void RoomWindow::show() {
    QWidget::show();
    emit opened();
}

bool RoomWindow::connectToServer() {
    QString address;
    quint16 port;
    bool success;

    auto idx = m_serverAddress.lastIndexOf(':');

    if (idx != -1) {
        address = m_serverAddress.mid(0, idx);
        port = m_serverAddress.mid(idx + 1).toUInt();
    } else {
        address = m_serverAddress;
        port = DEFAULT_PORT;
    }

    qDebug() << "Connect to" << m_serverAddress;
    m_clientSocket->connectToHost(address, port);

    if (m_clientSocket->state() == QAbstractSocket::ConnectedState ||
        m_clientSocket->waitForConnected(10000)) {
        success = true;
        m_clientSocketDisconnected = false;

        qDebug() << "Connected!";
    } else {
        success = false;
        qDebug() << m_clientSocket->errorString();
    }

    return success;
}

QString RoomWindow::getUserName() { return m_userName; }

QString RoomWindow::getRoomId() { return m_roomId; }

QString RoomWindow::getServerAddress() { return m_serverAddress; }

void RoomWindow::setServerAddress(const QString& str) {
    m_serverAddress = std::move(str);
    updateWindowTitle();
}

void RoomWindow::setUserName(const QString& str) {
    m_userName = std::move(str);
    updateWindowTitle();
}

void RoomWindow::setRoomId(const QString& str) {
    if (str.isEmpty()) {
        m_roomId = "new";
    } else {
        m_roomId = std::move(str);
    }

    updateWindowTitle();
}

void RoomWindow::updateWindowTitle() {
    setWindowTitle(m_userName + '@' + m_roomId + " | " + m_serverAddress);
}

RoomWindow::~RoomWindow() {
    // Messages
    m_textMessages->deleteLater();
    m_lineMessage->deleteLater();
    m_pushButtonSendMessage->deleteLater();

    // Users
    m_listUsers->deleteLater();

    // Files
    m_listFiles->deleteLater();
    m_actionDownload->deleteLater();
    m_pushButtonSendFile->deleteLater();

    // Disconnect
    m_pushButtonDisconnect->deleteLater();

    m_clientSocket->deleteLater();
}
