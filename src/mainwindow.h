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
    void fetchAlarmData();     
    void fetchDashboardData(); 
    void fetchDeviceData();    

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;

    QLabel* kpiLabels[5];

    QLineSeries* fallTrendSeries;
    QLineSeries* helpTrendSeries;

    QBarCategoryAxis* trendAxisX;
    QValueAxis* trendAxisY;

    QTableWidget* deviceTable;
};

#endif // MAINWINDOW_H