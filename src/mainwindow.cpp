#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QListWidget>
#include <QStackedWidget>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>
#include <QDateTime>
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

    // ================= 1. 构建【数据大盘】 UI =================
    QWidget* dashboardPage = ui->stackedWidget->widget(0);
    QLabel* ghostDash = dashboardPage->findChild<QLabel*>("label_dashboard");
    if (ghostDash) ghostDash->deleteLater();
    if (!dashboardPage->layout()) new QVBoxLayout(dashboardPage);
    QVBoxLayout* dashboardLayout = qobject_cast<QVBoxLayout*>(dashboardPage->layout());
    
    QHBoxLayout* kpiLayout = new QHBoxLayout();
    QStringList kpiTitles = {"当前在线网关", "今日报警总数", "待处理警情", "NPU 平均利用率"};
    for (int i = 0; i < 4; ++i) {
        QWidget* card = new QWidget();
        card->setStyleSheet("background-color: #f8f9fa; border-radius: 8px; border: 1px solid #dee2e6;");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        QLabel* titleLabel = new QLabel(kpiTitles[i]);
        titleLabel->setStyleSheet("color: #6c757d; font-size: 14px;");
        
        kpiLabels[i] = new QLabel("加载中..."); // 初始状态，等待真实数据
        kpiLabels[i]->setStyleSheet("color: #212529; font-size: 24px; font-weight: bold;");
        kpiLabels[i]->setAlignment(Qt::AlignCenter);
        
        cardLayout->addWidget(titleLabel);
        cardLayout->addWidget(kpiLabels[i]);
        kpiLayout->addWidget(card);
    }
    dashboardLayout->addLayout(kpiLayout);

    trendSeries = new QLineSeries(); // 初始化为成员变量，不再塞入假数据
    QChart *chart = new QChart();
    chart->addSeries(trendSeries);
    chart->setTitle("近 7 天摔倒报警趋势图");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->hide();
    
    QValueAxis *axisX = new QValueAxis;
    axisX->setTitleText("天数 (倒推)");
    axisX->setRange(0, 6);
    chart->addAxis(axisX, Qt::AlignBottom);
    trendSeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis;
    axisY->setTitleText("报警次数");
    axisY->setRange(0, 10); // 初始化 Y 轴，接收到真实数据后会动态拔高
    chart->addAxis(axisY, Qt::AlignLeft);
    trendSeries->attachAxis(axisY);

    QChartView* dashboardChartView = new QChartView(chart);
    dashboardChartView->setRenderHint(QPainter::Antialiasing); 
    dashboardLayout->addWidget(dashboardChartView);


    // ================= 2. 构建【设备管理】 UI =================
    QWidget* devicePage = ui->stackedWidget->widget(1);
    QLabel* ghostDev = devicePage->findChild<QLabel*>("label_device");
    if (ghostDev) ghostDev->deleteLater();
    if (!devicePage->layout()) new QVBoxLayout(devicePage);
    QVBoxLayout* deviceLayout = qobject_cast<QVBoxLayout*>(devicePage->layout());
    
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

    deviceTable = new QTableWidget(); // 成员变量
    deviceTable->setColumnCount(5);
    deviceTable->setHorizontalHeaderLabels(QStringList() << "设备标识 (Device ID)" << "部署位置" << "在线状态" << "模型版本" << "操作");
    deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    deviceTable->verticalHeader()->setVisible(false);
    deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceLayout->addWidget(deviceTable);


    // ================= 3. 构建【报警中心】 UI =================
    QWidget* alarmPage = ui->stackedWidget->widget(2);
    QVBoxLayout* alarmLayout = qobject_cast<QVBoxLayout*>(alarmPage->layout());
    if (!alarmLayout) alarmLayout = new QVBoxLayout(alarmPage);
    
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


    // ================= 4. 统一处理网络回调响应 (核心业务路由分发) =================
    connect(networkManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply)
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QString currentUrl = reply->request().url().toString();

            if (jsonDoc.isObject() && jsonDoc.object()["code"].toInt() == 200)
            {
                // [路由 A] : 接收到数据大盘数据
                if (currentUrl.contains("/api/dashboard")) 
                {
                    QJsonObject dataObj = jsonDoc.object()["data"].toObject();
                    
                    kpiLabels[0]->setText(QString("%1 台").arg(dataObj["online_gateways"].toInt()));
                    kpiLabels[1]->setText(QString("%1 起").arg(dataObj["today_alerts"].toInt()));
                    kpiLabels[2]->setText(QString("%1 起").arg(dataObj["pending_alerts"].toInt()));
                    kpiLabels[3]->setText(dataObj["npu_usage"].toString());

                    QJsonArray trendArr = dataObj["trend_7_days"].toArray();
                    trendSeries->clear();
                    int maxAlerts = 10;
                    for (int i = 0; i < trendArr.size(); ++i) {
                        int count = trendArr[i].toInt();
                        trendSeries->append(i, count);
                        if (count > maxAlerts) maxAlerts = count;
                    }
                    // 动态调整 Y 轴高度以适应真实数据最大值
                    QList<QAbstractAxis*> axes = trendSeries->chart()->axes(Qt::Vertical);
                    if (!axes.isEmpty()) qobject_cast<QValueAxis*>(axes.first())->setRange(0, maxAlerts + 5);

                    ui->statusbar->showMessage("✅ 大盘真实数据已更新", 3000);
                }
                // [路由 B] : 接收到设备列表数据
                else if (currentUrl.contains("/api/devices")) 
                {
                    QJsonArray arr = jsonDoc.object()["data"].toArray();
                    deviceTable->setRowCount(arr.size());
                    for (int i = 0; i < arr.size(); ++i) {
                        QJsonObject obj = arr[i].toObject();
                        deviceTable->setItem(i, 0, new QTableWidgetItem(obj["device_id"].toString()));
                        deviceTable->setItem(i, 1, new QTableWidgetItem(obj["location"].toString()));
                        deviceTable->setItem(i, 2, new QTableWidgetItem(obj["status"].toString()));
                        deviceTable->setItem(i, 3, new QTableWidgetItem(obj["model_version"].toString()));
                        
                        QPushButton* cfgBtn = new QPushButton("⚙️ 配置参数");
                        deviceTable->setCellWidget(i, 4, cfgBtn);
                    }
                    ui->statusbar->showMessage("✅ 设备真实数据已更新", 3000);
                }
                // [路由 C] : 接收到报警中心数据
                else if (currentUrl.contains("/api/alerts")) 
                {
                    QJsonArray arr = jsonDoc.object()["data"].toArray();
                    ui->alarmTableWidget->setRowCount(arr.size());
                    for (int i = 0; i < arr.size(); ++i) {
                        QJsonObject obj = arr[i].toObject();
                        qint64 ts = obj["server_receive_time"].toVariant().toLongLong();
                        if (ts == 0) ts = obj["timestamp"].toVariant().toLongLong();
                        QString timeStr = QDateTime::fromMSecsSinceEpoch(ts).toString("yyyy-MM-dd HH:mm:ss");

                        QString status = obj["status"].toString();
                        QString videoUrl = obj["video_url"].toString();

                        ui->alarmTableWidget->setItem(i, 0, new QTableWidgetItem(timeStr));
                        ui->alarmTableWidget->setItem(i, 1, new QTableWidgetItem(obj["device_id"].toString()));

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
                    ui->statusbar->showMessage(QString("✅ 报警中心刷新成功，共加载 %1 条记录。").arg(arr.size()), 5000);
                }
            }
        }
        else
        {
            ui->statusbar->showMessage("⚠️ 网络请求失败: " + reply->errorString(), 8000);
        }
        reply->deleteLater(); 
    });


    // ================= 5. 界面联动与路由触发 =================
    connect(ui->listWidget, &QListWidget::currentRowChanged, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    
    // 点击不同 Tab 时触发对应数据的物理拉取
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

// 三大 HTTP 发射器，向 FastAPI 后端发送请求
void MainWindow::fetchDashboardData() {
    ui->statusbar->showMessage("正在拉取大盘统计数据...");
    networkManager->get(QNetworkRequest(QUrl("http://10.48.212.22:8000/api/dashboard")));
}

void MainWindow::fetchDeviceData() {
    ui->statusbar->showMessage("正在拉取设备台账...");
    networkManager->get(QNetworkRequest(QUrl("http://10.48.212.22:8000/api/devices")));
}

void MainWindow::fetchAlarmData() {
    ui->statusbar->showMessage("正在拉取最新报警记录...");
    networkManager->get(QNetworkRequest(QUrl("http://10.48.212.22:8000/api/alerts?limit=50")));
}