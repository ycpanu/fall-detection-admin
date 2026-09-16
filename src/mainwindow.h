#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager> // 引入网络管理器
#include <QNetworkReply>         // 引入网络响应

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QNetworkAccessManager *networkManager; // 专门负责发请求的“管理员”
    void fetchAlarmData();                 // 拉取报警数据的专属函数
};

#endif // MAINWINDOW_H