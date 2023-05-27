#include "wsted.hpp"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QScreen>

#define LOG_CALL() qDebug().nospace() << __FUNCTION__ << "() call"

wsted::wsted(QWidget* parent) : QWidget(parent) {
    LOG_CALL();

    m_actionAbout = new QAction(this);
    m_menuHelp = new QMenu(this);
    m_menubar = new QMenuBar(this);
    m_comboBoxServers = new QComboBox(this);
    m_lineUserName = new QLineEdit(this);
    m_lineRoomId = new QLineEdit(this);
    m_pushButtonConnect = new QPushButton(this);

    setupUi();
    loadUi();
}

static void increaseCurrentObjectAY(QRect& obj) {
    int ax, ay, aw, ah;
    const int gap = 10;

    obj.getRect(&ax, &ay, &aw, &ah);
    obj.setRect(ax, ay + ah + gap, aw, ah);
}

void wsted::setupUi() {
    LOG_CALL();

    QRect currentObjectSize;

    const QSize screenSize = QApplication::primaryScreen()->size();
    const qreal screenRatio = QApplication::primaryScreen()->devicePixelRatio();
    const int windowWidth = (screenSize.width() * screenRatio) / 6;
    const int windowHeight = (screenSize.height() * screenRatio) / 4.5 - 5;

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

    // Servers
    m_comboBoxServers->setGeometry(currentObjectSize);
    increaseCurrentObjectAY(currentObjectSize);

    // Connect
    m_lineUserName->setGeometry(currentObjectSize);
    increaseCurrentObjectAY(currentObjectSize);

    m_lineRoomId->setGeometry(currentObjectSize);
    increaseCurrentObjectAY(currentObjectSize);

    m_pushButtonConnect->setGeometry(currentObjectSize);
    increaseCurrentObjectAY(currentObjectSize);
}

void wsted::loadUi() {
    LOG_CALL();

    // Window
    this->setWindowTitle("wsted");

    // Menubar
    m_menuHelp->setTitle("Help");
    m_actionAbout->setText("About");

    connect(m_actionAbout, &QAction::triggered, [=]() {
        QString text =
            "wsted allows users to quickly and easily share files within a room, as well as chat in "
            "real time.";
        QMessageBox::information(this, "About", text, QMessageBox::Close, QMessageBox::Close);
    });

    // Servers
    m_comboBoxServers->addItem("127.0.0.1");
    m_comboBoxServers->addItem("anotherserv.io");
    m_comboBoxServers->addItem("super.bx");

    // Connect
    m_lineUserName->setPlaceholderText("Username");
    m_lineUserName->setText("");
    m_lineUserName->setAlignment(Qt::AlignHCenter);
    m_lineUserName->setMaxLength(16);
    m_lineRoomId->setPlaceholderText("Room ID");
    m_lineRoomId->setText("");
    m_lineRoomId->setAlignment(Qt::AlignHCenter);
    m_lineRoomId->setMaxLength(10);
    m_pushButtonConnect->setText("Connect");
}

wsted::~wsted() {
    LOG_CALL();

    delete m_actionAbout;
    delete m_menuHelp;
    delete m_menubar;
    delete m_comboBoxServers;
    delete m_lineUserName;
    delete m_lineRoomId;
    delete m_pushButtonConnect;
}
