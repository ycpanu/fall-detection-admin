#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QListWidget>
#include <QStackedWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
#include <QDateTime>
#include <QLabel>
#include <QBrush>
#include <QFont>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    networkManager = new QNetworkAccessManager(this);

    // ================= 1. 构建【数据大盘】页面 (Dashboard) =================
    QWidget* dashboardPage = ui->stackedWidget->widget(0);
    
    // 【核心修复】：精准查找并删除 UI 文件中残留的占位标签
    QLabel* ghostDash = dashboardPage->findChild<QLabel*>("label_dashboard");
    if (ghostDash) ghostDash->deleteLater();

    if (!dashboardPage->layout()) {
        new QVBoxLayout(dashboardPage);
    }
    QVBoxLayout* dashboardLayout = qobject_cast<QVBoxLayout*>(dashboardPage->layout());
    
    // 顶部四个核心指标卡片
    QHBoxLayout* kpiLayout = new QHBoxLayout();
    QStringList kpiTitles = {"当前在线网关", "今日报警总数", "待处理警情", "NPU 平均利用率"};
    QStringList kpiValues = {"3 台", "12 起", "2 起", "68 %"}; // 模拟数据
    
    for (int i = 0; i < 4; ++i) {
        QWidget* card = new QWidget();
        card->setStyleSheet("background-color: #f8f9fa; border-radius: 8px; border: 1px solid #dee2e6;");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        QLabel* titleLabel = new QLabel(kpiTitles[i]);
        titleLabel->setStyleSheet("color: #6c757d; font-size: 14px;");
        QLabel* valueLabel = new QLabel(kpiValues[i]);
        valueLabel->setStyleSheet("color: #212529; font-size: 24px; font-weight: bold;");
        valueLabel->setAlignment(Qt::AlignCenter);
        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(valueLabel);
        kpiLayout->addWidget(card);
    }
    dashboardLayout->addLayout(kpiLayout);

    // 底部趋势图表 (使用 Qt Charts)
    QLineSeries *series = new QLineSeries();
    series->append(0, 5); series->append(1, 2); series->append(2, 8);
    series->append(3, 4); series->append(4, 12); series->append(5, 3); series->append(6, 6);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("近 7 天摔倒报警趋势图");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->hide();
    
    QValueAxis *axisX = new QValueAxis;
    axisX->setTitleText("天数 (倒推)");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis;
    axisY->setTitleText("报警次数");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    dashboardChartView = new QChartView(chart);
    dashboardChartView->setRenderHint(QPainter::Antialiasing); 
    dashboardLayout->addWidget(dashboardChartView);


    // ================= 2. 构建【设备管理】页面 (Device Management) =================
    QWidget* devicePage = ui->stackedWidget->widget(1);
    
    // 【核心修复】：精准查找并删除 UI 文件中残留的占位标签
    QLabel* ghostDev = devicePage->findChild<QLabel*>("label_device");
    if (ghostDev) ghostDev->deleteLater();

    if (!devicePage->layout()) {
        new QVBoxLayout(devicePage);
    }
    QVBoxLayout* deviceLayout = qobject_cast<QVBoxLayout*>(devicePage->layout());
    
    // 控制栏
    QHBoxLayout* devTopBar = new QHBoxLayout();
    QLabel* devTitle = new QLabel("<b>边缘网关设备列表</b>");
    devTitle->setStyleSheet("font-size: 16px;");
    QPushButton* devRefreshBtn = new QPushButton("🔄 刷新设备状态");
    devRefreshBtn->setStyleSheet("QPushButton { background-color: #007bff; color: white; border-radius: 4px; padding: 6px; } "
                                 "QPushButton:hover { background-color: #0056b3; }");
    connect(devRefreshBtn, &QPushButton::clicked, this, &MainWindow::fetchDeviceData);
    
    devTopBar->addWidget(devTitle);
    devTopBar->addStretch();
    devTopBar->addWidget(devRefreshBtn);
    deviceLayout->addLayout(devTopBar);

    // 设备表格
    QTableWidget* deviceTable = new QTableWidget();
    deviceTable->setColumnCount(5);
    deviceTable->setHorizontalHeaderLabels(QStringList() << "设备标识 (Device ID)" << "部署位置" << "在线状态" << "模型版本" << "操作");
    deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    deviceTable->verticalHeader()->setVisible(false);
    deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceLayout->addWidget(deviceTable);

    deviceTable->setRowCount(3);
    QStringList devIds = {"OrangePi_Gateway_001", "OrangePi_Gateway_002", "OrangePi_Gateway_003"};
    QStringList devLocs = {"1号楼-101室", "1号楼-203室", "2号楼-走廊"};
    QStringList devStatus = {"🟢 在线", "🔴 离线", "🟢 在线"};
    for(int i = 0; i < 3; ++i){
        deviceTable->setItem(i, 0, new QTableWidgetItem(devIds[i]));
        deviceTable->setItem(i, 1, new QTableWidgetItem(devLocs[i]));
        deviceTable->setItem(i, 2, new QTableWidgetItem(devStatus[i]));
        deviceTable->setItem(i, 3, new QTableWidgetItem("YOLOv8n-Pose.rknn"));
        
        QPushButton* cfgBtn = new QPushButton("⚙️ 配置参数");
        deviceTable->setCellWidget(i, 4, cfgBtn);
    }


    // ================= 3. 报警中心逻辑 =================
    QWidget* alarmPage = ui->stackedWidget->widget(2);
    QVBoxLayout* alarmLayout = qobject_cast<QVBoxLayout*>(alarmPage->layout());
    if (!alarmLayout) {
        alarmLayout = new QVBoxLayout(alarmPage);
    }
    
    if (alarmLayout->count() == 1) { 
        QHBoxLayout* topBarLayout = new QHBoxLayout();
        QLabel* titleLabel = new QLabel("<b>实时报警中心</b>");
        titleLabel->setStyleSheet("font-size: 16px;");
        QPushButton* refreshBtn = new QPushButton("🔄 手动刷新数据");
        refreshBtn->setFixedWidth(150);
        refreshBtn->setStyleSheet("QPushButton { background-color: #07c160; color: white; border-radius: 4px; padding: 6px; } "
                                  "QPushButton:hover { background-color: #06ad56; }");
        connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::fetchAlarmData);
        topBarLayout->addWidget(titleLabel);
        topBarLayout->addStretch();
        topBarLayout->addWidget(refreshBtn);
        alarmLayout->insertLayout(0, topBarLayout);
    }

    ui->alarmTableWidget->setColumnCount(4);
    ui->alarmTableWidget->setHorizontalHeaderLabels(QStringList() << "报警时间" << "设备ID" << "检测状态" << "现场视频");
    ui->alarmTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->alarmTableWidget->verticalHeader()->setVisible(false);
    ui->alarmTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->alarmTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(networkManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply)
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            if (reply->request().url().toString().contains("/api/alerts")) {
                QByteArray responseData = reply->readAll();
                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
                if (jsonDoc.isObject() && jsonDoc.object()["code"].toInt() == 200)
                {
                    QJsonArray jsonArray = jsonDoc.object()["data"].toArray();
                    ui->alarmTableWidget->setRowCount(jsonArray.size());
                    for (int i = 0; i < jsonArray.size(); ++i)
                    {
                        QJsonObject obj = jsonArray[i].toObject();
                        qint64 ts = obj["server_receive_time"].toVariant().toLongLong();
                        if (ts == 0) ts = obj["timestamp"].toVariant().toLongLong();
                        QString timeStr = QDateTime::fromMSecsSinceEpoch(ts).toString("yyyy-MM-dd HH:mm:ss");

                        QString deviceId = obj["device_id"].toString();
                        QString status = obj["status"].toString();
                        QString videoUrl = obj["video_url"].toString();

                        ui->alarmTableWidget->setItem(i, 0, new QTableWidgetItem(timeStr));
                        ui->alarmTableWidget->setItem(i, 1, new QTableWidgetItem(deviceId));

                        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
                        if (status == "CRITICAL" || status == "摔倒") {
                            statusItem->setForeground(QBrush(Qt::red));
                            QFont font = statusItem->font();
                            font.setBold(true);
                            statusItem->setFont(font);
                        }
                        ui->alarmTableWidget->setItem(i, 2, statusItem);

                        QLabel* linkLabel = new QLabel();
                        if (!videoUrl.isEmpty()) {
                            linkLabel->setText(QString("<a href='%1'>🎬 播放现场录像</a>").arg(videoUrl));
                            linkLabel->setOpenExternalLinks(true);
                        } else {
                            linkLabel->setText("暂无视频");
                            linkLabel->setStyleSheet("color: gray;");
                        }
                        linkLabel->setAlignment(Qt::AlignCenter);
                        ui->alarmTableWidget->setCellWidget(i, 3, linkLabel);
                    }
                    ui->statusbar->showMessage(QString("✅ 数据刷新成功，共加载 %1 条报警记录。").arg(jsonArray.size()), 5000);
                }
            }
        }
        else
        {
            ui->statusbar->showMessage("⚠️ API 请求失败: " + reply->errorString(), 8000);
        }
        reply->deleteLater(); 
    });


    // ================= 4. 界面联动与路由分发 =================
    connect(ui->listWidget, &QListWidget::currentRowChanged, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    
    connect(ui->listWidget, &QListWidget::currentRowChanged, this, [=](int row){
        if (row == 0) fetchDashboardData();
        else if (row == 1) fetchDeviceData();
        else if (row == 2) fetchAlarmData();
    });

    ui->listWidget->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);
    fetchDashboardData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fetchAlarmData()
{
    ui->statusbar->showMessage("正在连接云端拉取最新报警数据...");
    QUrl url("http://10.48.212.22:8000/api/alerts?limit=50");
    networkManager->get(QNetworkRequest(url));
}

void MainWindow::fetchDashboardData()
{
    ui->statusbar->showMessage("📊 数据大盘统计已更新", 3000);
}

void MainWindow::fetchDeviceData()
{
    ui->statusbar->showMessage("🖥️ 网关设备状态已刷新", 3000);
}