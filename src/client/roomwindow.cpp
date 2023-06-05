#include "roomwindow.hpp"

#include <QApplication>
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

    m_textMessages = new QTextEdit(this);
    m_lineMessage = new QLineEdit(this);
    m_listUsers = new QListWidget(this);
    m_pushButtonSend = new QPushButton(this);
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

    m_listUsers->setGeometry(
        QRect(m_textMessages->width(), 0, m_textMessages->width(), m_textMessages->height()));

    m_pushButtonSend->setGeometry(QRect(m_lineMessage->width(), m_textMessages->height(),
                                        size().width() * 0.25 / 3, size().height() * 0.07));
    m_pushButtonDisconnect->setGeometry(QRect(m_textMessages->width(), m_pushButtonSend->y(),
                                              size().width() * 0.25, size().height() * 0.07));
}

void RoomWindow::ui_loadContents() {
    LOG_CALL();

    this->setWindowTitle("Connecting...");
    this->setWindowIcon(QIcon(":/icons/app"));

    m_textMessages->setStyleSheet("color:white;border:0px;border-radius:1px");
    m_textMessages->setReadOnly(true);
    m_textMessages->setText("");

    m_lineMessage->setStyleSheet(
        "color:white;border:1px solid white;border-radius:1px;border-right:0px");
    m_lineMessage->setClearButtonEnabled(true);
    m_lineMessage->setText("");
    m_lineMessage->setMaxLength(2048 - 16 - 2);
    m_lineMessage->setPlaceholderText("Type here");

    m_listUsers->setStyleSheet("color:white;border:0px;border-left:1px solid white;border-radius:1px");
    m_listUsers->clear();

    m_pushButtonSend->setStyleSheet("color:white;border:1px solid white;border-radius:1px");
    m_pushButtonSend->setText("Send");
    m_pushButtonSend->setDefault(true);

    m_pushButtonDisconnect->setStyleSheet(m_pushButtonSend->styleSheet());
    m_pushButtonDisconnect->setText("Disconnect");

    this->connect(m_lineMessage, SIGNAL(returnPressed()), SLOT(pushButtonSend_clicked()));
    this->connect(m_pushButtonSend, SIGNAL(clicked()), SLOT(pushButtonSend_clicked()));
    this->connect(m_pushButtonDisconnect, SIGNAL(clicked()), SLOT(pushButtonDisconnect_clicked()));
}

void RoomWindow::pushButtonSend_clicked() {
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

    emit closed();
    this->close();
}

void RoomWindow::readyRead() {
    LOG_CALL();

    QString line;
    QString userName;
    QString message;

    /* /command room:data */
    QRegExp regex("^/([a-z]+) ([a-zA-Z0-9]+):(.*)$");
    QString command;
    QString roomId;
    QString data;

    while (m_clientSocket->bytesAvailable()) {
        line = QString::fromUtf8(m_clientSocket->readLine().trimmed());
        messageLogger("Received", m_clientSocket, line);

        if (regex.indexIn(line) != -1) {
            command = regex.cap(1);
            roomId = regex.cap(2);
            data = regex.cap(3);

            if (command == "roomid") {
                this->setRoomId(roomId);
            } else if (command == "userid") {
                this->setUserName(data);
            } else if (command == "users" && roomId == this->m_roomId) {
                QStringList userList = data.split(',');

                m_listUsers->clear();
                for (const auto& user : userList) {
                    m_listUsers->addItem(user);
                }
            } else {
                messageLogger("Bad", "server", line);
            }
        } else if (line.contains(':')) {
            auto idx = line.indexOf(':');
            userName = line.mid(0, idx);
            message = line.mid(idx + 1);

            m_textMessages->append("<b>" + userName + "</b>: " + message);
        }
    }
}

void RoomWindow::connected() {
    LOG_CALL();
    QThread::msleep(10);

    QString message = "/join " + m_roomId + ':' + m_userName;
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

    delete m_textMessages;
    delete m_lineMessage;
    delete m_listUsers;
    delete m_pushButtonSend;
    delete m_pushButtonDisconnect;

    delete m_clientSocket;
}
