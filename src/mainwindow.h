#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager> 
#include <QNetworkReply>         
// 引入 Qt Charts 模块
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>

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
    void fetchAlarmData();     // 拉取报警记录
    void fetchDashboardData(); // 拉取数据大盘统计
    void fetchDeviceData();    // 拉取设备列表

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager; 
    
    // 图表视图指针，用于动态刷新数据
    QChartView* dashboardChartView;
};

#endif // MAINWINDOW_H