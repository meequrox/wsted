#include <QApplication>

#include "wsted.hpp"

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    wsted w;
    w.show();

    return a.exec();
}
