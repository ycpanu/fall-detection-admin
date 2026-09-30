#ifndef SAFEZONEOVERLAY_H
#define SAFEZONEOVERLAY_H

#include <QPointF>
#include <QString>
#include <QVector>
#include <QWidget>

struct SafeZonePolygon
{
    QString name;
    QVector<QPointF> points;
};

class SafeZoneOverlay : public QWidget
{
    Q_OBJECT

public:
    explicit SafeZoneOverlay(QWidget* parent = nullptr);

    void setZones(const QVector<SafeZonePolygon>& zones);
    QVector<SafeZonePolygon> zones() const;

    bool beginZone(const QString& name);
    bool finishCurrentZone();
    void cancelCurrentZone();
    void undoLastPoint();
    void removeZone(int index);
    void clearZones();

    bool isDrawing() const;
    int currentPointCount() const;

signals:
    void zonesChanged();
    void currentPointCountChanged(int count);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QPointF widgetToNormalized(const QPointF& point) const;
    QPointF normalizedToWidget(const QPointF& point) const;
    void drawPoint(QPainter& painter, const QPointF& pos, int index, const QColor& color);

    QVector<SafeZonePolygon> zones_;
    QVector<QPointF> currentPoints_;
    QString currentZoneName_;
    bool drawing_ = false;
};

#endif