#include "arcsliderwidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <cmath>

namespace {
const int kMarginTop    = 10;
const int kMarginBottom = 10;
const int kThumbRadius  = 9;
const double kArcBulge  = 22.0; // horizontal bulge of the arc track in px
const double kTrackWidthPx = 5.0;
}

ArcSliderWidget::ArcSliderWidget(QWidget *parent) :
    QWidget(parent),
    m_min(0),
    m_max(100),
    m_value(0),
    m_pressed(false)
{
    setObjectName("ArcSliderWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(32, 120);
    setCursor(Qt::PointingHandCursor);
}

void ArcSliderWidget::setRange(int min, int max)
{
    if (max < min) max = min;
    if (m_min == min && m_max == max) return;
    m_min = min;
    m_max = max;
    const int clamped = qBound(m_min, m_value, m_max);
    if (clamped != m_value) {
        m_value = clamped;
        emit valueChanged(m_value);
    }
    update();
}

void ArcSliderWidget::setValue(int value)
{
    const int clamped = qBound(m_min, value, m_max);
    if (clamped == m_value) return;
    m_value = clamped;
    emit valueChanged(m_value);
    update();
}

int ArcSliderWidget::pixelToValue(int y, int trackHeight, int min, int max)
{
    if (trackHeight <= 0 || min >= max) return min;
    const int clamped_y = qBound(0, y, trackHeight);
    const double frac = 1.0 - double(clamped_y) / double(trackHeight);
    return min + int(std::round(frac * double(max - min)));
}

int ArcSliderWidget::valueToPixel(int value, int trackHeight, int min, int max)
{
    if (trackHeight <= 0 || min >= max) return 0;
    const int clamped = qBound(min, value, max);
    const double frac = double(clamped - min) / double(max - min);
    return int(std::round((1.0 - frac) * double(trackHeight)));
}

int ArcSliderWidget::trackHeight() const
{
    return qMax(0, height() - kMarginTop - kMarginBottom);
}

int ArcSliderWidget::trackTop() const
{
    return kMarginTop;
}

void ArcSliderWidget::setValueFromPixel(int local_y)
{
    const int track_y = local_y - trackTop();
    setValue(pixelToValue(track_y, trackHeight(), m_min, m_max));
}

void ArcSliderWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int th = trackHeight();
    if (th <= 0) return;

    const double cx = width() / 2.0;
    const double top    = trackTop();
    const double bottom = top + th;

    // Cubic Bézier track: P0=(cx, top), P1=P2=(cx+bulge, ·), P3=(cx, bottom).
    // For this specific curve the x-offset from cx reduces to a closed form,
    // which we use below to position the thumb exactly on the arc.
    QPainterPath path;
    path.moveTo(cx, top);
    path.cubicTo(cx + kArcBulge, top + th * 0.25,
                 cx + kArcBulge, top + th * 0.75,
                 cx, bottom);

    const double thumb_y = trackTop() + valueToPixel(m_value, th, m_min, m_max);
    const double t = th > 0 ? (thumb_y - top) / double(th) : 0.0;
    const double thumb_x = cx + 3.0 * t * (1.0 - t) * kArcBulge;

    // Theme tokens — match prototype v3. Custom paint, no QSS.
    static const QColor kInactiveTrack(0x38, 0x2D, 0x25);  // border-default
    static const QColor kActiveTrack  (0xE0, 0x92, 0x4A);  // accent
    static const QColor kThumbFill    (0xE0, 0x92, 0x4A);  // accent
    static const QColor kThumbEdge    (0x26, 0x1B, 0x14);  // bg-header (dark contrast)

    // Inactive (above thumb): dim track.
    p.setPen(QPen(kInactiveTrack, kTrackWidthPx, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(path);

    // Active (from bottom up to thumb): accent color, drawn by re-stroking
    // the full path under a clip that excludes everything above thumb_y.
    p.save();
    p.setClipRect(QRectF(0, thumb_y, double(width()), bottom - thumb_y + kTrackWidthPx));
    p.setPen(QPen(kActiveTrack, kTrackWidthPx, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(path);
    p.restore();

    // Thumb positioned on the arc.
    p.setPen(QPen(kThumbEdge, 1.5));
    p.setBrush(kThumbFill);
    p.drawEllipse(QPointF(thumb_x, thumb_y), kThumbRadius, kThumbRadius);
}

void ArcSliderWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = true;
    setValueFromPixel(event->pos().y());
    QWidget::mousePressEvent(event);
}

void ArcSliderWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed) setValueFromPixel(event->pos().y());
    QWidget::mouseMoveEvent(event);
}

void ArcSliderWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) m_pressed = false;
    QWidget::mouseReleaseEvent(event);
}
