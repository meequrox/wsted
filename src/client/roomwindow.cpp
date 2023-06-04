#include "roomwindow.hpp"

#include <QApplication>
#include <QRegExp>
#include <QScreen>

#define DEFAULT_PORT 8044

#define LOG_CALL() qDebug().nospace() << __PRETTY_FUNCTION__ << " call"

static QSize getDefaultWindowSize() {
    LOG_CALL();

    const QSize screenSize = QApplication::primaryScreen()->size();
    const qreal screenRatio = QApplication::primaryScreen()->devicePixelRatio();

    QSize s;
    s.setWidth((screenSize.width() * screenRatio) / 3);
    s.setHeight((screenSize.height() * screenRatio) / 2);

    return s;
}

RoomWindow::RoomWindow(QWidget* parent) : QWidget(parent) {
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
}

void RoomWindow::ui_setupGeometry() {
    LOG_CALL();

    // Window
    this->setWindowFlag(Qt::WindowFullscreenButtonHint, false);

    qDebug() << "Window" << this->size() << this->windowFlags();

    m_textMessages->setGeometry(QRect(0, 0, size().width() * 0.75, size().height() * 0.93));
    m_lineMessage->setGeometry(
        QRect(0, m_textMessages->height(), size().width() * 0.75, size().height() * 0.07));

    m_listUsers->setGeometry(
        QRect(m_textMessages->width(), 0, m_textMessages->width(), m_textMessages->height()));

    m_pushButtonSend->setGeometry(QRect(m_textMessages->width(), m_textMessages->height(),
                                        size().width() * 0.25 / 3, size().height() * 0.07));
    m_pushButtonDisconnect->setGeometry(QRect(m_pushButtonSend->x() + m_pushButtonSend->width(),
                                              m_pushButtonSend->y(), size().width() * 0.25 * 2 / 3,
                                              size().height() * 0.07));
}

void RoomWindow::ui_loadContents() {
    LOG_CALL();

    this->setWindowTitle("Connecting...");
    this->setWindowIcon(QIcon(":/icons/app"));

    m_textMessages->setStyleSheet("color:white;border:1px solid white;border-radius:1px");
    m_textMessages->setReadOnly(true);
    m_textMessages->setText("");

    m_lineMessage->setStyleSheet(m_textMessages->styleSheet());
    m_lineMessage->setClearButtonEnabled(true);
    m_lineMessage->setText("");
    m_lineMessage->setPlaceholderText("Type here");

    m_listUsers->setStyleSheet(m_textMessages->styleSheet());
    m_listUsers->clear();

    m_pushButtonSend->setStyleSheet(m_textMessages->styleSheet());
    m_pushButtonSend->setText("Send");

    m_pushButtonDisconnect->setStyleSheet(m_pushButtonSend->styleSheet());
    m_pushButtonDisconnect->setText("Disconnect");

    this->connect(m_pushButtonDisconnect, SIGNAL(clicked()), SLOT(pushButtonDisconnect_clicked()));
}

void RoomWindow::pushButtonSend_clicked() {
    LOG_CALL();

    QString message = m_lineMessage->text().trimmed();

    if (!message.isEmpty()) {
        m_clientSocket->write(QString("/message " + m_roomId + ":" + message + "\n").toUtf8());
        message.clear();
    }

    m_lineMessage->setFocus();
}

void RoomWindow::pushButtonDisconnect_clicked() {
    LOG_CALL();

    m_clientSocket->disconnectFromHost();

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

    while (m_clientSocket->canReadLine()) {
        line = QString::fromUtf8(m_clientSocket->readLine().trimmed());

        if (regex.indexIn(line) != -1) {
            if (command == "roomid") {
                this->setRoomId(roomId);
                updateWindowTitle();
            } else if (command == "users") {
                QStringList userList = data.split(',');

                m_listUsers->clear();
                for (const auto& user : userList) {
                    m_listUsers->addItem(user);
                }

            } else {
                qDebug() << "Unknown command from server:\n" << line;
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

    m_clientSocket->write(QString("/join " + m_roomId + ':' + m_userName).toUtf8());
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

    auto idx = m_serverAddress.lastIndexOf(':');

    if (idx != -1) {
        address = m_serverAddress.mid(0, idx);
        port = m_serverAddress.mid(idx + 1).toUInt();
    } else {
        address = m_serverAddress;
        port = DEFAULT_PORT;
    }

    qDebug().nospace() << __FUNCTION__ << ": connect to address " << address << " port " << port;

    m_clientSocket->connectToHost(m_serverAddress, port);

    bool success = m_clientSocket->waitForConnected();

    if (!success) {
        qDebug() << m_clientSocket->error() << m_clientSocket->errorString();
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
    ;
    delete m_pushButtonSend;
    delete m_pushButtonDisconnect;

    delete m_clientSocket;
}
