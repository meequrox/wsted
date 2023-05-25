#include "wsted.hpp"

#include <QDebug>

#define LOG_CALL() qDebug().nospace() << __FUNCTION__ << "() call"

wsted::wsted(QWidget* parent) : QWidget(parent) {
    LOG_CALL();

    m_actionAbout = new QAction(this);
    m_menuHelp = new QMenu(this);
    m_menubar = new QMenuBar(this);
    m_comboBoxServers = new QComboBox(this);
    m_lineRoomId = new QLineEdit(this);
    m_pushButtonConnect = new QPushButton(this);
    m_statusbar = new QStatusBar(this);

    setupUi();
    loadUi();
}

void wsted::setupUi() {
    LOG_CALL();

    // Size
    const int windowWidth = 360;
    const int windowHeight = 340;
    const QRect defaultObjectSize(windowWidth / 4, 0, windowWidth / 2, 40);
    const int gap = 10;

    // Window
    this->resize(windowWidth, windowHeight);
    this->setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    this->setWindowFlag(Qt::WindowFullscreenButtonHint, false);

    qDebug() << "Window" << this->size() << this->windowFlags();

    // Menubar
    m_menuHelp->addAction(m_actionAbout);
    m_menubar->setGeometry(QRect(0, 0, this->geometry().width(), 25));
    m_menubar->addAction(m_menuHelp->menuAction());

    // Servers
    m_comboBoxServers->setGeometry(defaultObjectSize.left(),
                                   m_menubar->y() + m_menubar->size().height() + gap,
                                   defaultObjectSize.width(), defaultObjectSize.height());

    // Connect
    m_lineRoomId->setGeometry(defaultObjectSize.left(),
                              m_comboBoxServers->y() + m_comboBoxServers->size().height() + gap,
                              defaultObjectSize.width(), defaultObjectSize.height());
    m_pushButtonConnect->setGeometry(defaultObjectSize.left(),
                                     m_lineRoomId->y() + m_comboBoxServers->size().height() + gap,
                                     defaultObjectSize.width(), defaultObjectSize.height());
}

void wsted::loadUi() {
    LOG_CALL();

    // Window
    this->setWindowTitle("wsted");

    // Menubar
    m_actionAbout->setText("About");
    m_menuHelp->setTitle("Help");

    // Servers
    m_comboBoxServers->addItem("127.0.0.1");
    m_comboBoxServers->addItem("anotherserv.io");
    m_comboBoxServers->addItem("super.bx");

    // Connect
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
    delete m_lineRoomId;
    delete m_pushButtonConnect;
    delete m_statusbar;
}
