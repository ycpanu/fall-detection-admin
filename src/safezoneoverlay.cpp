#include "safezoneoverlay.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <algorithm>

SafeZoneOverlay::SafeZoneOverlay(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);

    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void SafeZoneOverlay::setZones(const QVector<SafeZonePolygon>& zones)
{
    zones_ = zones;
    currentPoints_.clear();
    currentZoneName_.clear();
    drawing_ = false;
    setCursor(Qt::ArrowCursor);
    update();
    emit zonesChanged();
    emit currentPointCountChanged(0);
}

QVector<SafeZonePolygon> SafeZoneOverlay::zones() const
{
    return zones_;
}

bool SafeZoneOverlay::beginZone(const QString& name)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) return false;
    if (drawing_ && !currentPoints_.isEmpty()) return false;

    currentZoneName_ = trimmedName;
    currentPoints_.clear();
    drawing_ = true;
    setCursor(Qt::CrossCursor);
    update();
    emit currentPointCountChanged(0);
    return true;
}

bool SafeZoneOverlay::finishCurrentZone()
{
    if (!drawing_ || currentPoints_.size() < 3) return false;

    SafeZonePolygon zone;
    zone.name = currentZoneName_;
    zone.points = currentPoints_;
    zones_.push_back(zone);

    currentPoints_.clear();
    currentZoneName_.clear();
    drawing_ = false;
    setCursor(Qt::ArrowCursor);
    update();

    emit zonesChanged();
    emit currentPointCountChanged(0);
    return true;
}

void SafeZoneOverlay::cancelCurrentZone()
{
    currentPoints_.clear();
    currentZoneName_.clear();
    drawing_ = false;
    setCursor(Qt::ArrowCursor);
    update();
    emit currentPointCountChanged(0);
}

void SafeZoneOverlay::undoLastPoint()
{
    if (!drawing_ || currentPoints_.isEmpty()) return;
    currentPoints_.removeLast();
    update();
    emit currentPointCountChanged(currentPoints_.size());
}

void SafeZoneOverlay::removeZone(int index)
{
    if (index < 0 || index >= zones_.size()) return;
    zones_.removeAt(index);
    update();
    emit zonesChanged();
}

void SafeZoneOverlay::clearZones()
{
    zones_.clear();
    currentPoints_.clear();
    currentZoneName_.clear();
    drawing_ = false;
    setCursor(Qt::ArrowCursor);
    update();
    emit zonesChanged();
    emit currentPointCountChanged(0);
}

bool SafeZoneOverlay::isDrawing() const
{
    return drawing_;
}

int SafeZoneOverlay::currentPointCount() const
{
    return currentPoints_.size();
}

QPointF SafeZoneOverlay::widgetToNormalized(const QPointF& point) const
{
    if (width() <= 0 || height() <= 0) return {};

    const double x = std::clamp(point.x() / width(), 0.0, 1.0);
    const double y = std::clamp(point.y() / height(), 0.0, 1.0);
    return QPointF(x, y);
}

QPointF SafeZoneOverlay::normalizedToWidget(const QPointF& point) const
{
    return QPointF(point.x() * width(), point.y() * height());
}

void SafeZoneOverlay::drawPoint(QPainter& painter, const QPointF& pos, int index, const QColor& color)
{
    painter.setPen(QPen(Qt::white, 2));
    painter.setBrush(color);
    painter.drawEllipse(pos, 8, 8);

    QRectF textRect(pos.x() + 10, pos.y() - 12, 24, 20);
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, QString::number(index + 1));
}

void SafeZoneOverlay::mousePressEvent(QMouseEvent* event)
{
    if (!drawing_)
    {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        currentPoints_.push_back(widgetToNormalized(event->position()));
        update();
        emit currentPointCountChanged(currentPoints_.size());
        return;
    }

    if (event->button() == Qt::RightButton)
    {
        undoLastPoint();
        return;
    }

    QWidget::mousePressEvent(event);
}

void SafeZoneOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    for (const auto& zone : zones_)
    {
        if (zone.points.size() < 3) continue;

        QPolygonF polygon;
        for (const auto& point : zone.points) polygon << normalizedToWidget(point);

        painter.setPen(QPen(QColor(0, 220, 100), 2));
        painter.setBrush(QColor(0, 220, 100, 55));
        painter.drawPolygon(polygon);

        for (int i = 0; i < polygon.size(); ++i)
            drawPoint(painter, polygon[i], i, QColor(0, 220, 100));

        painter.setPen(Qt::white);
        painter.drawText(polygon.first() + QPointF(12, -10), zone.name);
    }

    if (!drawing_ || currentPoints_.isEmpty()) return;

    QPolygonF polyline;
    for (const auto& point : currentPoints_) polyline << normalizedToWidget(point);

    painter.setPen(QPen(QColor(255, 180, 0), 3, Qt::SolidLine));
    painter.setBrush(Qt::NoBrush);

    if (polyline.size() >= 2) painter.drawPolyline(polyline);

    for (int i = 0; i < polyline.size(); ++i)
        drawPoint(painter, polyline[i], i, QColor(255, 80, 80));

    if (polyline.size() >= 3)
    {
        painter.setPen(QPen(QColor(255, 180, 0), 2, Qt::DashLine));
        painter.drawLine(polyline.last(), polyline.first());

        QPainterPath path;
        path.addPolygon(polyline);
        path.closeSubpath();
        painter.fillPath(path, QColor(255, 180, 0, 45));
    }

    painter.setPen(Qt::white);
    painter.drawText(10, 25, QString("正在绘制：%1").arg(currentZoneName_));
    painter.drawText(10, 48, QString("已选顶点：%1").arg(currentPoints_.size()));
    painter.drawText(10, 71, "左键添加点，右键撤销最后一个点");
}