#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager> 
#include <QNetworkReply>         
#include <QLabel>
#include <QTableWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

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
    void fetchAlarmData();     
    void fetchDashboardData(); 
    void fetchDeviceData();    

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager; 
    
    // 用于接收异步网络数据并刷新的 UI 成员指针
    QLabel* kpiLabels[4];         // 大盘顶部的 4 个数字卡片
    QLineSeries* trendSeries;     // 大盘底部的 7 天趋势折线
    QTableWidget* deviceTable;    // 设备管理表格
};

#endif // MAINWINDOW_H