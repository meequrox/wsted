#include "wsted.hpp"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QScreen>

#define LOG_CALL() qDebug().nospace() << __PRETTY_FUNCTION__ << " call"

typedef QRegularExpression QRegExp;
typedef QRegularExpressionValidator QRegExpValidator;

wsted::wsted(QWidget* parent) : QWidget(parent) {
    LOG_CALL();

    m_actionAbout = new QAction(this);
    m_menuHelp = new QMenu(this);
    m_menubar = new QMenuBar(this);
    m_comboBoxServers = new QComboBox(this);
    m_lineUserName = new QLineEdit(this);
    m_lineRoomId = new QLineEdit(this);
    m_pushButtonConnect = new QPushButton(this);

    ui_setupGeometry();
    ui_loadContents();
}

static void increaseCurrentObjectAY(QRect& obj, int extraGap = 0) {
    int ax, ay, aw, ah;
    const int gap = 10;

    obj.getRect(&ax, &ay, &aw, &ah);
    obj.setRect(ax, ay + ah + gap + extraGap, aw, ah);
}

void wsted::ui_setupGeometry() {
    LOG_CALL();

    QRect currentObjectSize;

    const QSize screenSize = QApplication::primaryScreen()->size();
    const qreal screenRatio = QApplication::primaryScreen()->devicePixelRatio();
    const int windowWidth = (screenSize.width() * screenRatio) / 5;
    const int windowHeight = (screenSize.height() * screenRatio) / 4;

    // Window
    this->setFixedSize(windowWidth, windowHeight);
    this->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    this->setWindowFlag(Qt::WindowFullscreenButtonHint, false);

    qDebug() << "Window" << this->size() << this->windowFlags();

    // Menubar
    m_menuHelp->addAction(m_actionAbout);
    m_menubar->setGeometry(QRect(0, 0, this->geometry().width(), 25));
    m_menubar->addAction(m_menuHelp->menuAction());

    currentObjectSize = QRect(windowWidth / 4, 35, windowWidth / 2, 40);
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
                                     windowHeight - currentObjectSize.height() - 20,
                                     currentObjectSize.width(), currentObjectSize.height());
}

void wsted::ui_loadContents() {
    LOG_CALL();

    this->setWindowTitle("wsted");

    // Menubar
    m_menuHelp->setTitle("Help");
    m_actionAbout->setText("About");
    this->connect(m_actionAbout, SIGNAL(triggered()), SLOT(actionAbout_triggered()));

    // Servers
    m_comboBoxServers->addItem("127.0.0.1");
    m_comboBoxServers->addItem("anotherserv.io");
    m_comboBoxServers->addItem("super.bx");

    // Connect
    auto userNameValidator = new QRegExpValidator(QRegExp("[a-zA-Z0-9_-]{1,16}"), this);
    auto roomIdValidator = new QRegExpValidator(QRegExp("[a-zA-Z0-9]{1,10}"), this);

    m_lineUserName->setPlaceholderText("Username");
    m_lineUserName->setText("");
    m_lineUserName->setAlignment(Qt::AlignHCenter);
    m_lineUserName->setMaxLength(16);
    m_lineUserName->setValidator(userNameValidator);

    m_lineRoomId->setPlaceholderText("Room ID (can be blank)");
    m_lineRoomId->setText("");
    m_lineRoomId->setAlignment(Qt::AlignHCenter);
    m_lineRoomId->setMaxLength(10);
    m_lineRoomId->setValidator(roomIdValidator);

    m_pushButtonConnect->setText("Connect");
    this->connect(m_pushButtonConnect, SIGNAL(clicked()), SLOT(pushButtonConnect_clicked()));
}

void wsted::actionAbout_triggered() {
    LOG_CALL();

    QString text(
        "wsted allows users to quickly and easily share files within a room, as well as chat in real "
        "time");
    QMessageBox::information(this, "About", text, QMessageBox::Close, QMessageBox::Close);
}

void wsted::pushButtonConnect_clicked() {
    LOG_CALL();

    QString messageBoxText;

    if (m_lineUserName->text().isEmpty()) {
        messageBoxText = QString("Empty username");
        qDebug() << messageBoxText;

        QMessageBox::warning(this, "Connect", messageBoxText, QMessageBox::Close, QMessageBox::Close);
        return;
    }

    if (m_lineRoomId->text().isEmpty()) {
        // TODO: Request new room ID from server
        qDebug() << "Empty room ID, requesting a new room from the server";
    }

    // TODO: Connect to server and open room window
    // This messageBox is temporary to make sure all checks work correctly
    messageBoxText = QString("CHECKS PASSED. CONNECTION IS NOT IMPLEMENTED");
    QMessageBox::warning(this, "Connect", messageBoxText, QMessageBox::Close, QMessageBox::Close);
}

wsted::~wsted() {
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
}
