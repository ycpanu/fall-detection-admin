#include "MainWindow.h"
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
#include <QVariant>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ================= 网络请求与 JSON 解析 =================
    networkManager = new QNetworkAccessManager(this);

    // 当网络请求完成时，自动执行下面这个 Lambda 函数
    connect(networkManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply)
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            // 1. 读取服务器返回的所有数据
            QByteArray responseData = reply->readAll();

            qDebug() << "=== 收到后端真实数据 ===";
            qDebug() << responseData;

            // 2. 将字符串解析为 JSON 文档
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

            // 3. 针对真实数据的精准解析
            if (jsonDoc.isObject())
            {
                QJsonObject rootObj = jsonDoc.object();

                // 确保后端返回的是成功状态 (code 200)
                if (rootObj["code"].toInt() == 200)
                {
                    // 提取真正的数组 "data"
                    QJsonArray jsonArray = rootObj["data"].toArray();

                    ui->alarmTableWidget->setRowCount(jsonArray.size());

                    for (int i = 0; i < jsonArray.size(); ++i)
                    {
                        QJsonObject obj = jsonArray[i].toObject();

                        // 字段 1: 时间戳转换 (13位毫秒转为标准时间字符串)
                        qint64 ts = obj["timestamp"].toVariant().toLongLong();
                        QString timeStr = QDateTime::fromMSecsSinceEpoch(ts).toString("yyyy-MM-dd HH:mm:ss");

                        // 字段 2, 3, 4: 提取文本
                        QString deviceId = obj["device_id"].toString();
                        QString status = obj["status"].toString();
                        QString videoName = obj["video_name"].toString();

                        // 填入表格
                        ui->alarmTableWidget->setItem(i, 0, new QTableWidgetItem(timeStr));
                        ui->alarmTableWidget->setItem(i, 1, new QTableWidgetItem(deviceId));
                        ui->alarmTableWidget->setItem(i, 2, new QTableWidgetItem(status));
                        ui->alarmTableWidget->setItem(i, 3, new QTableWidgetItem(videoName));
                    }
                }
            }
        }
        else
        {
            // 如果报错（比如服务器没开），在底部输出窗口打印错误信息
            qDebug() << "API Request Failed:" << reply->errorString();
        }
        reply->deleteLater(); // 释放内存
    });

    // 界面初始化完成后，立刻发起一次请求
    fetchAlarmData();

    // 1. 核心联动：将左侧列表的行切换信号，连接到右侧堆栈页面的切换槽函数
    connect(ui->listWidget, &QListWidget::currentRowChanged,
            ui->stackedWidget, &QStackedWidget::setCurrentIndex);

    // 2. 初始状态：让程序一跑起来就默认选中第 0 行（数据大盘）
    QWidget* tablePage = ui->alarmTableWidget->parentWidget();
    ui->stackedWidget->addWidget(tablePage);

    // 再次默认选中第 0 行
    ui->listWidget->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);

    // ================= 报警中心表格初始化 =================
    // 1. 设置列数和表头文字
    ui->alarmTableWidget->setColumnCount(4);
    ui->alarmTableWidget->setHorizontalHeaderLabels(
        QStringList() << "报警时间" << "设备ID" << "检测状态" << "关联视频"
        );

    // 2. 样式优化：让列宽自动拉伸撑满屏幕
    ui->alarmTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 3. 交互优化：隐藏左侧自带的默认行号（1,2,3...）
    ui->alarmTableWidget->verticalHeader()->setVisible(false);

    // 4. 交互优化：点击时自动选中整行，而不是单个单元格
    ui->alarmTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 5. 交互优化：禁止用户双击修改表格里的内容（后台数据只能看不能改）
    ui->alarmTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fetchAlarmData()
{
    // 这里填入你的 FastAPI 真实接口地址
    // 如果后端还没跑起来，我们可以先用一个测试地址或者写死本地 127.0.0.1
    QUrl url("http://10.48.212.22:8000/api/alerts");
    QNetworkRequest request(url);

    // 发起 GET 请求
    networkManager->get(request);
}