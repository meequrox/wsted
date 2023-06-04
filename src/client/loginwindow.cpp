#include "loginwindow.hpp"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QScreen>

#define LOG_CALL() qDebug().nospace() << __PRETTY_FUNCTION__ << " call"

typedef QRegularExpression QRegExp;
typedef QRegularExpressionValidator QRegExpValidator;

static QSize getDefaultWindowSize() {
    LOG_CALL();

    const QSize screenSize = QApplication::primaryScreen()->size();
    const qreal screenRatio = QApplication::primaryScreen()->devicePixelRatio();

    QSize s;
    s.setWidth((screenSize.width() * screenRatio) / 5);
    s.setHeight((screenSize.height() * screenRatio) / 4);

    return s;
}

LoginWindow::LoginWindow(QWidget* parent) : QWidget(parent) {
    LOG_CALL();

    m_actionAbout = new QAction(this);
    m_menuHelp = new QMenu(this);
    m_menubar = new QMenuBar(this);
    m_comboBoxServers = new QComboBox(this);
    m_lineUserName = new QLineEdit(this);
    m_lineRoomId = new QLineEdit(this);
    m_pushButtonConnect = new QPushButton(this);

    m_widgetRoom = new RoomWindow(nullptr);

    this->setFixedSize(getDefaultWindowSize());

    ui_setupGeometry();
    ui_loadContents();
}

static void increaseCurrentObjectAY(QRect& obj, int extraGap = 0) {
    int ax, ay, aw, ah;
    const int gap = 10;

    obj.getRect(&ax, &ay, &aw, &ah);
    obj.setRect(ax, ay + ah + gap + extraGap, aw, ah);
}

void LoginWindow::ui_setupGeometry() {
    LOG_CALL();

    QRect currentObjectSize;

    this->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    this->setWindowFlag(Qt::WindowFullscreenButtonHint, false);

    qDebug() << "Window" << this->size() << this->windowFlags();

    // Menubar
    m_menuHelp->addAction(m_actionAbout);
    m_menubar->setGeometry(QRect(0, 0, this->geometry().width(), 25));
    m_menubar->addAction(m_menuHelp->menuAction());

    currentObjectSize = QRect(size().width() / 4, 35, size().width() / 2, 40);
    qDebug() << "Object size" << currentObjectSize;

    // Servers
    m_comboBoxServers->setGeometry(currentObjectSize);
    increaseCurrentObjectAY(currentObjectSize, 10);

    // Connect
    m_lineUserName->setGeometry(currentObjectSize);
    increaseCurrentObjectAY(currentObjectSize);

    m_lineRoomId->setGeometry(currentObjectSize);
    increaseCurrentObjectAY(currentObjectSize);

    m_pushButtonConnect->setGeometry(currentObjectSize.x(),
                                     size().height() - currentObjectSize.height() - 20,
                                     currentObjectSize.width(), currentObjectSize.height());
}

void LoginWindow::ui_loadContents() {
    LOG_CALL();

    this->setWindowTitle("wsted");
    this->setWindowIcon(QIcon(":/icons/app"));

    // Menubar
    m_menuHelp->setTitle("Help");
    m_actionAbout->setText("About");
    this->connect(m_actionAbout, SIGNAL(triggered()), SLOT(actionAbout_triggered()));

    // Servers
    m_comboBoxServers->setStyleSheet("color:white;border:1px solid white;border-radius:1px");
    m_comboBoxServers->setEditable(true);
    m_comboBoxServers->lineEdit()->setReadOnly(true);
    m_comboBoxServers->lineEdit()->setAlignment(Qt::AlignCenter);

    m_comboBoxServers->addItem("127.0.0.1:7999");
    m_comboBoxServers->addItem("0.0.0.0:7999");
    m_comboBoxServers->addItem("anotherserv.io");
    m_comboBoxServers->addItem("super.bx:8814");

    for (int i = 0; i < m_comboBoxServers->count(); i++) {
        m_comboBoxServers->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
    }

    // Connect
    auto userNameValidator = new QRegExpValidator(QRegExp("[a-zA-Z0-9_-]{1,16}"), this);
    auto roomIdValidator = new QRegExpValidator(QRegExp("[a-zA-Z0-9]{1,10}"), this);

    m_lineUserName->setStyleSheet("color:white;border:1px solid white;border-radius:6px");
    m_lineUserName->setPlaceholderText("Username");
    m_lineUserName->setText("");
    m_lineUserName->setAlignment(Qt::AlignHCenter);
    m_lineUserName->setMaxLength(16);
    m_lineUserName->setValidator(userNameValidator);
    m_lineUserName->setClearButtonEnabled(true);

    m_lineRoomId->setStyleSheet(m_lineUserName->styleSheet());
    m_lineRoomId->setPlaceholderText("Room ID (can be blank)");
    m_lineRoomId->setText("");
    m_lineRoomId->setAlignment(Qt::AlignHCenter);
    m_lineRoomId->setMaxLength(10);
    m_lineRoomId->setValidator(roomIdValidator);
    m_lineRoomId->setClearButtonEnabled(true);

    m_pushButtonConnect->setStyleSheet(m_lineUserName->styleSheet());
    m_pushButtonConnect->setText("Connect");
    this->connect(m_pushButtonConnect, SIGNAL(clicked()), SLOT(pushButtonConnect_clicked()));

    // Next windows
    this->connect(m_widgetRoom, SIGNAL(opened()), this, SLOT(hide()));
    this->connect(m_widgetRoom, SIGNAL(closed()), this, SLOT(show()));
}

void LoginWindow::actionAbout_triggered() {
    LOG_CALL();

    QString text(
        "wsted allows users to quickly and easily share files within a room, as well as chat in real "
        "time");
    QMessageBox::information(this, "About", text, QMessageBox::Close, QMessageBox::Close);
}

void LoginWindow::pushButtonConnect_clicked() {
    LOG_CALL();

    QString messageBoxText;

    if (m_lineUserName->text().isEmpty()) {
        messageBoxText = QString("Empty username");
        qDebug() << messageBoxText;

        QMessageBox::warning(this, "Connect", messageBoxText, QMessageBox::Close, QMessageBox::Close);
        return;
    }

    qDebug() << "\nShow room window";

    m_widgetRoom->setUserName(m_lineUserName->text());
    m_widgetRoom->setRoomId(m_lineRoomId->text());
    m_widgetRoom->setServerAddress(m_comboBoxServers->currentText());

    if (m_widgetRoom->connectToServer() == false) {
        messageBoxText =
            QString("Can't join " + m_widgetRoom->getRoomId() + '@' + m_widgetRoom->getServerAddress() +
                    " as " + m_widgetRoom->getUserName());
        qDebug() << messageBoxText;

        QMessageBox::warning(this, "Connect", messageBoxText, QMessageBox::Close, QMessageBox::Close);
        return;
    }

    m_widgetRoom->show();
}

LoginWindow::~LoginWindow() {
    LOG_CALL();

    delete m_actionAbout;
    delete m_menuHelp;
    delete m_menubar;
    delete m_comboBoxServers;
    delete m_lineUserName->validator();
    delete m_lineUserName;
    delete m_lineRoomId->validator();
    delete m_lineRoomId;
    delete m_pushButtonConnect;
    delete m_widgetRoom;
}
