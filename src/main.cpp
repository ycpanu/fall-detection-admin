#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.setWindowTitle("摔倒检测系统 - 后台管理");
    w.resize(1024, 768);
    w.show();

    return app.exec();
}