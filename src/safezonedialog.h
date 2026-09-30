#ifndef SAFEZONEDIALOG_H
#define SAFEZONEDIALOG_H

#include <QDialog>
#include <QImage>
#include <QNetworkAccessManager>

class QLabel;
class QListWidget;
class QLineEdit;
class QMediaPlayer;
class QTimer;
class QVideoSink;
class SafeZoneOverlay;

class SafeZoneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SafeZoneDialog(const QString& deviceId, const QString& apiBaseUrl, const QString& rtmpUrl, const QString& hlsUrl, QWidget* parent = nullptr);
    ~SafeZoneDialog() override;

private:
    void buildUi();
    void loadSafeZones();
    void saveSafeZones();
    void startLive();
    void stopLive();

    void captureSnapshot();
    void updateSnapshotImage(const QImage& image);
    void refreshZoneList();

    void startDrawing();
    void finishDrawing();
    void cancelDrawing();
    void undoPoint();
    void removeSelectedZone();
    void clearAllZones();

    QString deviceId_;
    QString apiBaseUrl_;
    QString rtmpUrl_;
    QString hlsUrl_;

    QNetworkAccessManager* networkManager_ = nullptr;

    QLabel* imageLabel_ = nullptr;
    SafeZoneOverlay* overlay_ = nullptr;
    QLineEdit* zoneNameEdit_ = nullptr;
    QListWidget* zoneList_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* pointCountLabel_ = nullptr;

    QMediaPlayer* snapshotPlayer_ = nullptr;
    QVideoSink* videoSink_ = nullptr;
    QTimer* snapshotTimeoutTimer_ = nullptr;
    QImage lastSnapshot_;
};

#endif