#ifndef LOGINWINDOW_HPP
#define LOGINWINDOW_HPP

#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QWidget>

#include "roomwindow.hpp"

class LoginWindow : public QWidget {
    Q_OBJECT

   public:
    LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow();

   private:
    void ui_setupGeometry();
    void ui_loadContents();

    // Menubar
    QAction* m_actionAbout;
    QMenu* m_menuHelp;
    QMenuBar* m_menubar;

    // Servers
    QComboBox* m_comboBoxServers;

    // Connect
    QLineEdit* m_lineUserName;
    QLineEdit* m_lineRoomId;
    QPushButton* m_pushButtonConnect;

    // Next windows
    RoomWindow* m_widgetRoom;

   public slots:
    void pushButtonConnect_clicked();
    void actionAbout_triggered();
};
#endif  // LOGINWINDOW_HPP
