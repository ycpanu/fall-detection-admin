#include "safezonedialog.h"
#include "safezoneoverlay.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QStackedLayout>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <QVideoSink>
#include <QPixmap>

SafeZoneDialog::SafeZoneDialog(const QString& deviceId, const QString& apiBaseUrl, const QString& rtmpUrl, const QString& hlsUrl, QWidget* parent)
    : QDialog(parent), deviceId_(deviceId), apiBaseUrl_(apiBaseUrl), rtmpUrl_(rtmpUrl), hlsUrl_(hlsUrl)
{
    setWindowTitle(QString("安全区域配置 - %1").arg(deviceId_));
    resize(1120, 780);

    networkManager_ = new QNetworkAccessManager(this);
    snapshotPlayer_ = new QMediaPlayer(this);
    videoSink_ = new QVideoSink(this);
    snapshotTimeoutTimer_ = new QTimer(this);

    snapshotPlayer_->setVideoSink(videoSink_);
    snapshotTimeoutTimer_->setSingleShot(true);

    buildUi();
    loadSafeZones();

    connect(videoSink_, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame)
    {
        if (!frame.isValid()) return;

        const QImage image = frame.toImage();
        if (image.isNull()) return;

        snapshotTimeoutTimer_->stop();
        snapshotPlayer_->stop();
        updateSnapshotImage(image);
        statusLabel_->setText("画面已刷新，可开始绘制安全区域");
    });

    connect(snapshotTimeoutTimer_, &QTimer::timeout, this, [this]()
    {
        snapshotPlayer_->stop();
        statusLabel_->setText("画面刷新超时，请点击“刷新画面”重试");
    });

    startLive();
}

SafeZoneDialog::~SafeZoneDialog()
{
    if (snapshotPlayer_) snapshotPlayer_->stop();
    stopLive();
}

void SafeZoneDialog::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);

    auto* topLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel(QString("<b>设备：</b>%1").arg(deviceId_));
    statusLabel_ = new QLabel("正在连接设备...");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    topLayout->addWidget(statusLabel_);
    rootLayout->addLayout(topLayout);

    auto* contentLayout = new QHBoxLayout();

    auto* imageContainer = new QWidget();
    imageContainer->setMinimumSize(780, 540);

    auto* stackLayout = new QStackedLayout(imageContainer);
    stackLayout->setContentsMargins(0, 0, 0, 0);
    stackLayout->setStackingMode(QStackedLayout::StackAll);

    imageLabel_ = new QLabel(imageContainer);
    imageLabel_->setStyleSheet("background-color: black;");
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setScaledContents(true);

    overlay_ = new SafeZoneOverlay(imageContainer);

    stackLayout->addWidget(imageLabel_);
    stackLayout->addWidget(overlay_);

    stackLayout->setCurrentWidget(overlay_);
    overlay_->raise();
    overlay_->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    contentLayout->addWidget(imageContainer, 1);

    auto* controlWidget = new QWidget();
    controlWidget->setFixedWidth(300);
    auto* controlLayout = new QVBoxLayout(controlWidget);

    controlLayout->addWidget(new QLabel("<b>安全躺卧区域配置</b>"));

    auto* tipLabel = new QLabel(
        "推荐流程：\n"
        "1. 点击“刷新画面”获取一帧静态图片\n"
        "2. 输入区域名称\n"
        "3. 点击“开始绘制”\n"
        "4. 在图片上依次点击顶点\n"
        "5. 点击“完成区域”\n"
        "6. 点击“保存配置”\n\n"
        "右键可撤销最后一个点。"
    );
    tipLabel->setWordWrap(true);
    controlLayout->addWidget(tipLabel);

    auto* refreshBtn = new QPushButton("刷新画面");
    controlLayout->addWidget(refreshBtn);

    zoneNameEdit_ = new QLineEdit();
    zoneNameEdit_->setPlaceholderText("例如：床 / 沙发 / 休息区");
    controlLayout->addWidget(zoneNameEdit_);

    auto* drawBtn = new QPushButton("开始绘制");
    auto* finishBtn = new QPushButton("完成区域");
    auto* row1 = new QHBoxLayout();
    row1->addWidget(drawBtn);
    row1->addWidget(finishBtn);
    controlLayout->addLayout(row1);

    auto* undoBtn = new QPushButton("撤销顶点");
    auto* cancelBtn = new QPushButton("取消当前绘制");
    auto* row2 = new QHBoxLayout();
    row2->addWidget(undoBtn);
    row2->addWidget(cancelBtn);
    controlLayout->addLayout(row2);

    pointCountLabel_ = new QLabel("当前顶点：0");
    controlLayout->addWidget(pointCountLabel_);

    controlLayout->addWidget(new QLabel("已配置区域："));

    zoneList_ = new QListWidget();
    controlLayout->addWidget(zoneList_, 1);

    auto* removeBtn = new QPushButton("删除选中区域");
    auto* clearBtn = new QPushButton("清空全部区域");
    controlLayout->addWidget(removeBtn);
    controlLayout->addWidget(clearBtn);

    auto* saveBtn = new QPushButton("保存配置");
    saveBtn->setMinimumHeight(38);
    controlLayout->addWidget(saveBtn);

    contentLayout->addWidget(controlWidget);
    rootLayout->addLayout(contentLayout, 1);

    auto* bottomLayout = new QHBoxLayout();
    auto* closeBtn = new QPushButton("关闭");
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeBtn);
    rootLayout->addLayout(bottomLayout);

    connect(refreshBtn, &QPushButton::clicked, this, &SafeZoneDialog::captureSnapshot);
    connect(drawBtn, &QPushButton::clicked, this, &SafeZoneDialog::startDrawing);
    connect(finishBtn, &QPushButton::clicked, this, &SafeZoneDialog::finishDrawing);
    connect(undoBtn, &QPushButton::clicked, this, &SafeZoneDialog::undoPoint);
    connect(cancelBtn, &QPushButton::clicked, this, &SafeZoneDialog::cancelDrawing);
    connect(removeBtn, &QPushButton::clicked, this, &SafeZoneDialog::removeSelectedZone);
    connect(clearBtn, &QPushButton::clicked, this, &SafeZoneDialog::clearAllZones);
    connect(saveBtn, &QPushButton::clicked, this, &SafeZoneDialog::saveSafeZones);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    connect(overlay_, &SafeZoneOverlay::zonesChanged, this, &SafeZoneDialog::refreshZoneList);
    connect(overlay_, &SafeZoneOverlay::currentPointCountChanged, this, [this](int count)
    {
        pointCountLabel_->setText(QString("当前顶点：%1").arg(count));
    });
}

void SafeZoneDialog::loadSafeZones()
{
    const QUrl url(QString("%1/api/devices/%2/safe-zones").arg(apiBaseUrl_, deviceId_));
    QNetworkReply* reply = networkManager_->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        const QByteArray responseData = reply->readAll();

        if (reply->error() != QNetworkReply::NoError)
        {
            statusLabel_->setText(QString("读取安全区域失败：%1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (!doc.isObject())
        {
            statusLabel_->setText("读取安全区域失败：返回格式错误");
            reply->deleteLater();
            return;
        }

        const QJsonObject root = doc.object();
        if (root["code"].toInt() != 200)
        {
            statusLabel_->setText(root["message"].toString("读取安全区域失败"));
            reply->deleteLater();
            return;
        }

        QVector<SafeZonePolygon> zones;
        const QJsonArray zoneArr = root["data"].toObject()["safe_lie_zones"].toArray();

        for (const auto& zoneVal : zoneArr)
        {
            const QJsonObject zoneObj = zoneVal.toObject();
            SafeZonePolygon zone;
            zone.name = zoneObj["name"].toString();

            const QJsonArray pointArr = zoneObj["points"].toArray();
            for (const auto& pointVal : pointArr)
            {
                const QJsonArray p = pointVal.toArray();
                if (p.size() != 2) continue;
                zone.points.push_back(QPointF(p[0].toDouble(), p[1].toDouble()));
            }

            if (!zone.name.isEmpty() && zone.points.size() >= 3)
                zones.push_back(zone);
        }

        overlay_->setZones(zones);
        refreshZoneList();
        reply->deleteLater();
    });
}

void SafeZoneDialog::saveSafeZones()
{
    if (overlay_->isDrawing())
    {
        QMessageBox::warning(this, "无法保存", "当前还有未完成的区域，请先完成或取消当前绘制。");
        return;
    }

    QJsonArray zonesJson;
    for (const auto& zone : overlay_->zones())
    {
        QJsonObject zoneObj;
        zoneObj["name"] = zone.name;

        QJsonArray pointsJson;
        for (const auto& point : zone.points)
        {
            QJsonArray p;
            p.append(point.x());
            p.append(point.y());
            pointsJson.append(p);
        }

        zoneObj["points"] = pointsJson;
        zonesJson.append(zoneObj);
    }

    QJsonObject body;
    body["safe_lie_zones"] = zonesJson;

    const QUrl url(QString("%1/api/devices/%2/safe-zones").arg(apiBaseUrl_, deviceId_));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = networkManager_->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    statusLabel_->setText("正在保存安全区域...");

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        const QByteArray responseData = reply->readAll();

        if (reply->error() != QNetworkReply::NoError)
        {
            QMessageBox::critical(this, "保存失败", reply->errorString());
            statusLabel_->setText("安全区域保存失败");
            reply->deleteLater();
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (!doc.isObject())
        {
            QMessageBox::critical(this, "保存失败", "服务器响应格式错误");
            reply->deleteLater();
            return;
        }

        const QJsonObject root = doc.object();
        if (root["code"].toInt() != 200)
        {
            QMessageBox::critical(this, "保存失败", root["message"].toString("安全区域配置保存失败"));
            reply->deleteLater();
            return;
        }

        statusLabel_->setText("安全区域配置已保存并下发到设备");
        QMessageBox::information(this, "保存成功", "安全区域配置已保存，并已下发到边缘网关。");
        reply->deleteLater();
    });
}

void SafeZoneDialog::startLive()
{
    const QUrl url(QString("%1/api/device/%2/live/start").arg(apiBaseUrl_, deviceId_));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["rtmp_url"] = rtmpUrl_;

    QNetworkReply* reply = networkManager_->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    statusLabel_->setText("正在请求设备现场画面...");

    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        const QByteArray responseData = reply->readAll();

        if (reply->error() != QNetworkReply::NoError)
        {
            statusLabel_->setText(QString("启动现场画面失败：%1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (!doc.isObject() || doc.object()["code"].toInt() != 200)
        {
            QString message = "启动现场画面失败";
            if (doc.isObject()) message = doc.object()["message"].toString(message);
            statusLabel_->setText(message);
            reply->deleteLater();
            return;
        }

        statusLabel_->setText("设备已开始推流，准备抓取静态画面...");
        QTimer::singleShot(1500, this, &SafeZoneDialog::captureSnapshot);
        reply->deleteLater();
    });
}

void SafeZoneDialog::stopLive()
{
    QNetworkAccessManager* manager = new QNetworkAccessManager(qApp);
    const QUrl url(QString("%1/api/device/%2/live/stop").arg(apiBaseUrl_, deviceId_));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = manager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, manager, [reply, manager]()
    {
        reply->deleteLater();
        manager->deleteLater();
    });
}

void SafeZoneDialog::captureSnapshot()
{
    statusLabel_->setText("正在刷新画面...");
    snapshotPlayer_->stop();
    snapshotPlayer_->setSource(QUrl());
    snapshotTimeoutTimer_->start(5000);
    snapshotPlayer_->setSource(QUrl(hlsUrl_));
    snapshotPlayer_->play();
}

void SafeZoneDialog::updateSnapshotImage(const QImage& image)
{
    lastSnapshot_ = image;
    imageLabel_->setPixmap(QPixmap::fromImage(lastSnapshot_));
}

void SafeZoneDialog::refreshZoneList()
{
    zoneList_->clear();
    const auto zones = overlay_->zones();

    for (int i = 0; i < zones.size(); ++i)
        zoneList_->addItem(QString("%1  (%2 个顶点)").arg(zones[i].name).arg(zones[i].points.size()));
}

void SafeZoneDialog::startDrawing()
{
    const QString zoneName = zoneNameEdit_->text().trimmed();
    if (zoneName.isEmpty())
    {
        QMessageBox::warning(this, "区域名称", "请先输入安全区域名称。");
        return;
    }

    if (!overlay_->beginZone(zoneName))
    {
        QMessageBox::warning(this, "无法开始绘制", "请先完成或取消当前区域，再绘制新的区域。");
        return;
    }

    statusLabel_->setText(QString("正在绘制安全区域：%1").arg(zoneName));
}

void SafeZoneDialog::finishDrawing()
{
    if (!overlay_->finishCurrentZone())
    {
        QMessageBox::warning(this, "无法完成区域", "一个安全区域至少需要 3 个顶点。");
        return;
    }

    zoneNameEdit_->clear();
    statusLabel_->setText("区域绘制完成，点击“保存配置”后生效");
}

void SafeZoneDialog::cancelDrawing()
{
    overlay_->cancelCurrentZone();
    statusLabel_->setText("已取消当前绘制");
}

void SafeZoneDialog::undoPoint()
{
    overlay_->undoLastPoint();
}

void SafeZoneDialog::removeSelectedZone()
{
    const int row = zoneList_->currentRow();
    if (row < 0)
    {
        QMessageBox::information(this, "删除区域", "请先选择需要删除的区域。");
        return;
    }

    overlay_->removeZone(row);
    statusLabel_->setText("区域已删除，保存配置后生效");
}

void SafeZoneDialog::clearAllZones()
{
    const auto ret = QMessageBox::question(this, "清空区域", "确定清空所有安全区域吗？");
    if (ret != QMessageBox::Yes) return;

    overlay_->clearZones();
    statusLabel_->setText("已清空所有安全区域，保存配置后生效");
}