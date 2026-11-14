#include "arcsliderwidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <cmath>

namespace {
const int MARGIN_TOP    = 10;
const int MARGIN_BOTTOM = 10;
const int THUMB_RADIUS  = 9;
const double ARC_BULGE  = 22.0; // horizontal bulge of the arc track in px
const double TRACK_WIDTH_PX = 5.0;

bool update_color(QColor &slot, const QColor &color)
{
    if (slot == color) return false;
    slot = color;
    return true;
}
}

ArcSliderWidget::ArcSliderWidget(QWidget *parent) :
    QWidget(parent),
    m_min(0),
    m_max(100),
    m_value(0),
    m_pressed(false),
    m_speed_mode(false),
    m_max_speed(0),
    m_last_signed_speed(0),
    m_active_color(0xE0, 0x92, 0x4A),
    m_inactive_track_color(0x5C, 0x4A, 0x3D),
    m_thumb_edge_color(0x12, 0x10, 0x0E)
{
    setObjectName("ArcSliderWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(32, 120);
    setCursor(Qt::PointingHandCursor);
}

void ArcSliderWidget::set_active_color(const QColor &c)
{
    if (update_color(m_active_color, c)) update();
}

void ArcSliderWidget::set_inactive_track_color(const QColor &c)
{
    if (update_color(m_inactive_track_color, c)) update();
}

void ArcSliderWidget::set_thumb_edge_color(const QColor &c)
{
    if (update_color(m_thumb_edge_color, c)) update();
}

void ArcSliderWidget::set_range(int min, int max)
{
    if (max < min) max = min;
    if (m_min == min && m_max == max) return;
    m_min = min;
    m_max = max;
    const int clamped = qBound(m_min, m_value, m_max);
    if (clamped != m_value) {
        m_value = clamped;
        emit value_changed(m_value);
    }
    update();
}

void ArcSliderWidget::set_value(int value)
{
    const int clamped = qBound(m_min, value, m_max);
    if (clamped == m_value) return;
    m_value = clamped;
    emit value_changed(m_value);
    update();
}

void ArcSliderWidget::set_speed_mode(int max_speed)
{
    m_speed_mode = true;
    m_max_speed = qMax(0, max_speed);
    set_range(-m_max_speed, m_max_speed);
    m_last_signed_speed = 0;
    set_value(0);
}

int ArcSliderWidget::pixel_to_signed_speed(int local_y) const
{
    const int th = track_height();
    if (th <= 0 || m_max_speed <= 0) return 0;

    const int track_y = local_y - track_top();
    const double half = th / 2.0;
    // delta > 0 when pointer is above midpoint (toward top of track).
    const double delta = half - double(track_y);
    if (delta == 0.0) return 0;

    const double frac = qBound(-1.0, delta / half, 1.0);
    // quadratic curve: fine control near the midpoint, fast ramp near the ends
    const double raw = frac * frac * double(m_max_speed);
    int mag = int(std::round(raw / 4.0)) * 4;
    if (mag < 4) mag = 4;            // off-midpoint floor; speed 0 only at midpoint
    if (mag > m_max_speed) mag = m_max_speed;
    return (frac > 0.0) ? mag : -mag;
}

void ArcSliderWidget::apply_speed_from_pixel(int local_y)
{
    const int speed = pixel_to_signed_speed(local_y);
    if (speed != m_last_signed_speed) {
        m_last_signed_speed = speed;
        emit speed_changed(speed);
    }
    // Drive m_value too so paintEvent shows the thumb at the dragged position.
    if (m_value != speed) {
        m_value = speed;
        emit value_changed(m_value);
        update();
    }
}

int ArcSliderWidget::pixel_to_value(int y, int track_height, int min, int max)
{
    if (track_height <= 0 || min >= max) return min;
    const int clamped_y = qBound(0, y, track_height);
    const double frac = 1.0 - double(clamped_y) / double(track_height);
    return min + int(std::round(frac * double(max - min)));
}

int ArcSliderWidget::value_to_pixel(int value, int track_height, int min, int max)
{
    if (track_height <= 0 || min >= max) return 0;
    const int clamped = qBound(min, value, max);
    const double frac = double(clamped - min) / double(max - min);
    return int(std::round((1.0 - frac) * double(track_height)));
}

int ArcSliderWidget::track_height() const
{
    return qMax(0, height() - MARGIN_TOP - MARGIN_BOTTOM);
}

int ArcSliderWidget::track_top() const
{
    return MARGIN_TOP;
}

void ArcSliderWidget::set_value_from_pixel(int local_y)
{
    const int track_y = local_y - track_top();
    set_value(pixel_to_value(track_y, track_height(), m_min, m_max));
}

void ArcSliderWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int th = track_height();
    if (th <= 0) return;

    const double cx = width() / 2.0;
    const double top    = track_top();
    const double bottom = top + th;

    // Cubic Bezier track from (cx, top) to (cx, bottom) with both control points at
    // cx + bulge. For this curve the x offset has a closed form, used below to put
    // the thumb exactly on the arc.
    QPainterPath path;
    path.moveTo(cx, top);
    path.cubicTo(cx + ARC_BULGE, top + th * 0.25,
                 cx + ARC_BULGE, top + th * 0.75,
                 cx, bottom);

    const double thumb_y = track_top() + value_to_pixel(m_value, th, m_min, m_max);
    const double t = th > 0 ? (thumb_y - top) / double(th) : 0.0;
    const double thumb_x = cx + 3.0 * t * (1.0 - t) * ARC_BULGE;

    // Inactive (full path): visible neutral.
    p.setPen(QPen(m_inactive_track_color, TRACK_WIDTH_PX, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(path);

    // Active (from bottom up to thumb): per-slider color.
    p.save();
    p.setClipRect(QRectF(0, thumb_y, double(width()), bottom - thumb_y + TRACK_WIDTH_PX));
    p.setPen(QPen(m_active_color, TRACK_WIDTH_PX, Qt::SolidLine, Qt::RoundCap));
    p.drawPath(path);
    p.restore();

    // Thumb positioned on the arc.
    p.setPen(QPen(m_thumb_edge_color, 1.5));
    p.setBrush(m_active_color);
    p.drawEllipse(QPointF(thumb_x, thumb_y), THUMB_RADIUS, THUMB_RADIUS);
}

void ArcSliderWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = true;
    if (m_speed_mode) apply_speed_from_pixel(event->pos().y());
    else              set_value_from_pixel(event->pos().y());
    QWidget::mousePressEvent(event);
}

void ArcSliderWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed) {
        if (m_speed_mode) apply_speed_from_pixel(event->pos().y());
        else              set_value_from_pixel(event->pos().y());
    }
    QWidget::mouseMoveEvent(event);
}

void ArcSliderWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = false;
        if (m_speed_mode) {
            // Snap to midpoint; emit speed_changed(0) and value_changed(0).
            if (m_last_signed_speed != 0) {
                m_last_signed_speed = 0;
                emit speed_changed(0);
            }
            if (m_value != 0) {
                m_value = 0;
                emit value_changed(0);
                update();
            }
        }
    }
    QWidget::mouseReleaseEvent(event);
}
