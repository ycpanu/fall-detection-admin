#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager> 
#include <QNetworkReply>         

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void fetchAlarmData(); // 从私有函数改为槽函数，方便按钮绑定

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager; 
};

#endif // MAINWINDOW_H