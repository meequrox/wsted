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
    LOG_CALL();

    const QSize screenSize = QApplication::primaryScreen()->size();
    const qreal screenRatio = QApplication::primaryScreen()->devicePixelRatio();

    QSize s;
    s.setWidth((screenSize.width() * screenRatio) / 3);
    s.setHeight((screenSize.height() * screenRatio) / 2);

    return s;
}

RoomWindow::RoomWindow(QWidget* parent) : QWidget(parent), m_clientSocketDisconnected(false) {
    LOG_CALL();

    m_actionDownload = new QAction(this);
    m_textMessages = new QTextEdit(this);
    m_lineMessage = new QLineEdit(this);
    m_listUsers = new QListWidget(this);
    m_listFiles = new QListWidget(this);
    m_pushButtonSendMessage = new QPushButton(this);
    m_pushButtonSendFile = new QPushButton(this);
    m_pushButtonDisconnect = new QPushButton(this);

    this->setMinimumSize(480, 320);
    this->resize(getDefaultWindowSize());

    ui_setupGeometry();
    ui_loadContents();

    m_clientSocket = new QTcpSocket();
    this->connect(m_clientSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    this->connect(m_clientSocket, SIGNAL(connected()), this, SLOT(connected()));
    this->connect(m_clientSocket, SIGNAL(disconnected()), this, SLOT(pushButtonDisconnect_clicked()));
}

void RoomWindow::ui_setupGeometry() {
    LOG_CALL();

    // Window
    this->setWindowFlag(Qt::WindowFullscreenButtonHint, false);

    qDebug() << "Window" << size();

    m_textMessages->setGeometry(QRect(0, 0, size().width() * 0.75, size().height() * 0.93));
    m_lineMessage->setGeometry(
        QRect(0, m_textMessages->height(), size().width() * 2 / 3, size().height() * 0.07));

    m_pushButtonSendMessage->setGeometry(QRect(m_lineMessage->width(), m_textMessages->height(),
                                               size().width() * 0.25 / 3, size().height() * 0.07));
    m_pushButtonDisconnect->setGeometry(QRect(m_textMessages->width(), m_pushButtonSendMessage->y(),
                                              size().width() * 0.25, size().height() * 0.07));
    m_pushButtonSendFile->setGeometry(QRect(
        m_pushButtonDisconnect->x(), m_pushButtonDisconnect->y() - m_pushButtonDisconnect->height() - 1,
        m_pushButtonDisconnect->width(), m_pushButtonDisconnect->height()));

    m_listUsers->setGeometry(
        QRect(m_textMessages->width(), 0, size().width() * 0.25, m_textMessages->height() * 0.50));
    m_listFiles->setGeometry(QRect(m_textMessages->width(), m_listUsers->height(), m_listUsers->width(),
                                   m_listUsers->height() - m_pushButtonSendFile->height() - 2));
}

void RoomWindow::ui_loadContents() {
    LOG_CALL();

    this->setWindowTitle("Connecting...");
    this->setWindowIcon(QIcon(":/icons/app"));

    m_actionDownload->setText("Download");
    this->connect(m_actionDownload, SIGNAL(triggered()), SLOT(actionDownload_triggered()));

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

    m_listUsers->setStyleSheet(
        "color:white;border:0px;border-left:1px solid white;border-right:1px solid "
        "white;border-radius:1px");
    m_listUsers->clear();

    m_listFiles->setStyleSheet("color:white;border:1px solid white;border-radius:1px");
    m_listFiles->clear();
    m_listFiles->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_listFiles->insertAction(nullptr, m_actionDownload);

    m_pushButtonSendMessage->setStyleSheet("color:white;border:1px solid white;border-radius:1px");
    m_pushButtonSendMessage->setText("Say");
    m_pushButtonSendMessage->setDefault(true);

    m_pushButtonSendFile->setStyleSheet(m_pushButtonSendMessage->styleSheet());
    m_pushButtonSendFile->setText("Upload file");

    m_pushButtonDisconnect->setStyleSheet(m_pushButtonSendMessage->styleSheet());
    m_pushButtonDisconnect->setText("Disconnect");

    this->connect(m_lineMessage, SIGNAL(returnPressed()), SLOT(pushButtonSendMessage_clicked()));

    this->connect(m_pushButtonSendMessage, SIGNAL(clicked()), SLOT(pushButtonSendMessage_clicked()));
    this->connect(m_pushButtonSendFile, SIGNAL(clicked()), SLOT(pushButtonSendFile_clicked()));
    this->connect(m_pushButtonDisconnect, SIGNAL(clicked()), SLOT(pushButtonDisconnect_clicked()));
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
    LOG_CALL();

    QString message = m_lineMessage->text().trimmed();

    if (!message.isEmpty()) {
        message = "/message " + m_roomId + ":" + message + '\n';

        m_clientSocket->write(message.toUtf8());
        m_lineMessage->clear();

        messageLogger("Sent", m_clientSocket, message);
    }

    m_lineMessage->setFocus();
}

void RoomWindow::pushButtonSendFile_clicked() {
    LOG_CALL();

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

    fileName = "'" + filePath.mid(filePath.lastIndexOf('/') + 1) + "'";
    messageToWrite = "/sendfile " + fileName + ' ' + m_roomId + ':' + file.readAll().toBase64() + '\n';

    m_clientSocket->write(messageToWrite.toUtf8());
    messageLogger("Sent FILE", m_clientSocket,
                  messageToWrite.mid(0, messageToWrite.indexOf(':') + 1) + "_BASE64_DATA_");
}

void RoomWindow::pushButtonDisconnect_clicked() {
    LOG_CALL();

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
    this->close();
}

void RoomWindow::readyRead() {
    LOG_CALL();

    QString userName;
    QString line;
    QString message;

    QRegExp messageRegex("^/([a-z]+) ([a-zA-Z0-9]+):(.*)$");  // /command room:data
    QString command;

    QRegExp fileRegex("^/([a-z]+) '(.*)' ([a-zA-Z0-9]+):(.*)$");  // /command filename room:data
    QString filename;

    QString roomId;
    QString data;

    while (m_clientSocket->canReadLine()) {
        line = QString::fromUtf8(m_clientSocket->readLine().trimmed());
        messageLogger("Received", m_clientSocket, line);

        if (messageRegex.indexIn(line) != -1) {
            // Message from server

            command = messageRegex.cap(1);
            roomId = messageRegex.cap(2);
            data = messageRegex.cap(3);

            if (command == "roomid") {
                this->setRoomId(roomId);
            } else if (command == "userid") {
                this->setUserName(data);
            } else if (command == "users" && roomId == m_roomId) {
                QStringList userList = data.split(',');

                m_listUsers->clear();
                for (const auto& user : userList) {
                    m_listUsers->addItem(user);
                }
            } else if (command == "files" && roomId == m_roomId) {
                QStringList fileList = data.split('/');

                m_listFiles->clear();
                for (const auto& fileName : fileList) {
                    m_listFiles->addItem(fileName);
                }
            }
        } else if (fileRegex.indexIn(line) != -1) {
            // File from server

            command = fileRegex.cap(1);
            filename = fileRegex.cap(2);
            roomId = fileRegex.cap(3);
            data = fileRegex.cap(4);

            if (command == "sendfile" && !filename.isEmpty() && !data.isEmpty()) {
                messageLogger("Received FILE", m_clientSocket,
                              '/' + command + " '" + filename + "' " + roomId + ":_BASE64_DATA_");

                QString downloadRoomPath = QString(getenv("HOME")) + "/Downloads/" + roomId + "/";

                QDir dir;
                if (!dir.mkpath(downloadRoomPath)) {
                    qDebug() << "Failed to create path" << downloadRoomPath;
                    return;
                }

                QFile file(downloadRoomPath + filename);

                while (file.exists()) {
                    qDebug() << "Duplicate filename" << filename;

                    auto idx = filename.lastIndexOf('.');
                    if (idx != -1) {
                        filename = filename.mid(0, idx) + "-1" + filename.mid(idx);
                    } else {
                        filename = filename + "-1";
                    }

                    file.setFileName(downloadRoomPath + filename);

                    qDebug() << "Changing to" << filename;
                }

                if (!file.open(QIODevice::Append, QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
                    qDebug() << file.fileName() << file.errorString();
                    return;
                }

                file.write(QByteArray::fromBase64(data.toUtf8()));
                file.close();

                messageLogger("Received FILE", m_clientSocket, filename);
            }
        } else if (line.contains(':')) {
            auto idx = line.indexOf(':');
            userName = line.mid(0, idx);
            message = line.mid(idx + 1);

            m_textMessages->append("<b>" + userName + "</b>: " + message);
        } else {
            qDebug("^ Bad message\n");
        }
    }
}

void RoomWindow::connected() {
    LOG_CALL();
    QThread::msleep(10);

    QString message = "/join " + m_roomId + ':' + m_userName + '\n';
    m_clientSocket->write(message.toUtf8());

    messageLogger("Sent", m_clientSocket, message);
}

void RoomWindow::resizeEvent(QResizeEvent* ev) {
    LOG_CALL();

    QWidget::resizeEvent(ev);
    ui_setupGeometry();
}

void RoomWindow::show() {
    LOG_CALL();

    QWidget::show();
    emit opened();
}

bool RoomWindow::connectToServer() {
    LOG_CALL();

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

    qDebug().noquote() << __FUNCTION__ << ": connect to address" << address << "port" << port;

    // m_clientSocket->connectToHost(m_serverAddress, port);
    m_clientSocket->connectToHost("0.0.0.0", DEFAULT_PORT);

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
    LOG_CALL();

    this->setWindowTitle(m_userName + '@' + m_roomId + " | " + m_serverAddress);
}

RoomWindow::~RoomWindow() {
    LOG_CALL();

    m_actionDownload->deleteLater();
    m_textMessages->deleteLater();
    m_lineMessage->deleteLater();
    m_listUsers->deleteLater();
    m_listFiles->deleteLater();
    m_pushButtonSendMessage->deleteLater();
    m_pushButtonSendFile->deleteLater();
    m_pushButtonDisconnect->deleteLater();

    m_clientSocket->deleteLater();
}
