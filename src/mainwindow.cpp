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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ================= 网络请求与 JSON 解析 =================
    networkManager = new QNetworkAccessManager(this);

    connect(networkManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply)
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

            if (jsonDoc.isObject())
            {
                QJsonObject rootObj = jsonDoc.object();
                if (rootObj["code"].toInt() == 200)
                {
                    QJsonArray jsonArray = rootObj["data"].toArray();
                    ui->alarmTableWidget->setRowCount(jsonArray.size());

                    for (int i = 0; i < jsonArray.size(); ++i)
                    {
                        QJsonObject obj = jsonArray[i].toObject();

                        // 字段 1: 时间戳转换 (优先使用云端接收时间 server_receive_time)
                        qint64 ts = obj["server_receive_time"].toVariant().toLongLong();
                        if (ts == 0) ts = obj["timestamp"].toVariant().toLongLong();
                        QString timeStr = QDateTime::fromMSecsSinceEpoch(ts).toString("yyyy-MM-dd HH:mm:ss");

                        // 字段 2, 3: 提取设备与状态
                        QString deviceId = obj["device_id"].toString();
                        QString status = obj["status"].toString();
                        
                        // 字段 4: 提取最新的云端视频 URL
                        QString videoUrl = obj["video_url"].toString();

                        // 填入表格：时间与设备ID
                        ui->alarmTableWidget->setItem(i, 0, new QTableWidgetItem(timeStr));
                        ui->alarmTableWidget->setItem(i, 1, new QTableWidgetItem(deviceId));

                        // 填入表格：高亮渲染检测状态
                        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
                        if (status == "CRITICAL" || status == "摔倒") {
                            statusItem->setForeground(QBrush(Qt::red));
                            QFont font = statusItem->font();
                            font.setBold(true);
                            statusItem->setFont(font);
                        }
                        ui->alarmTableWidget->setItem(i, 2, statusItem);

                        // 填入表格：动态生成视频富文本超链接 (利用 QLabel 实现)
                        QLabel* linkLabel = new QLabel();
                        if (!videoUrl.isEmpty()) {
                            linkLabel->setText(QString("<a href='%1'>🎬 播放现场录像</a>").arg(videoUrl));
                            linkLabel->setOpenExternalLinks(true); // 允许点击直接调用浏览器打开
                        } else {
                            linkLabel->setText("暂无视频");
                            linkLabel->setStyleSheet("color: gray;");
                        }
                        linkLabel->setAlignment(Qt::AlignCenter);
                        
                        // 注意：这里使用的是 setCellWidget 插入控件，而不是 setItem
                        ui->alarmTableWidget->setCellWidget(i, 3, linkLabel);
                    }
                    
                    // 在底部状态栏提示成功
                    ui->statusbar->showMessage(QString("数据刷新成功，共加载 %1 条报警记录。").arg(jsonArray.size()), 5000);
                }
            }
        }
        else
        {
            ui->statusbar->showMessage("⚠️ API 请求失败: " + reply->errorString(), 8000);
            qDebug() << "API Request Failed:" << reply->errorString();
        }
        reply->deleteLater(); 
    });

    // ================= 界面联动与初始化 =================
    // 1. 左侧导航与右侧堆栈页面的精确映射
    connect(ui->listWidget, &QListWidget::currentRowChanged,
            ui->stackedWidget, &QStackedWidget::setCurrentIndex);

    // 2. 绑定“刷新数据”按钮的点击事件
    connect(ui->refreshButton, &QPushButton::clicked, this, &MainWindow::fetchAlarmData);

    // ================= 报警中心表格美化 =================
    ui->alarmTableWidget->setColumnCount(4);
    ui->alarmTableWidget->setHorizontalHeaderLabels(QStringList() << "报警时间" << "设备ID" << "检测状态" << "现场视频");
    ui->alarmTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->alarmTableWidget->verticalHeader()->setVisible(false); // 隐藏行号
    ui->alarmTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->alarmTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止双击修改

    // 默认跳转到索引 2 (报警中心)，并立刻请求一次数据
    ui->listWidget->setCurrentRow(2);
    ui->stackedWidget->setCurrentIndex(2);
    fetchAlarmData();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fetchAlarmData()
{
    // 调用 FastAPI 真实接口，加入 limit=50 参数获取最新数据
    ui->statusbar->showMessage("正在连接云端拉取最新数据...");
    QUrl url("http://10.48.212.22:8000/api/alerts?limit=50");
    QNetworkRequest request(url);
    networkManager->get(request);
}