#ifndef WSTED_HPP
#define WSTED_HPP

#include <QAction>
#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QWidget>

class wsted : public QWidget {
    Q_OBJECT

   public:
    wsted(QWidget* parent = nullptr);
    ~wsted();

   private:
    void setupUi();
    void loadUi();

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
};
#endif  // WSTED_HPP
