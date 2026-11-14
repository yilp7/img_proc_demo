#include "unlockbar.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

namespace {
const int    MARGIN_H          = 4;
const int    MARGIN_V          = 4;
const int    DEFAULT_RELOCK_MS = 3000;
const double DEFAULT_THRESHOLD = 0.9;

bool update_color(QColor &slot, const QColor &color)
{
    if (slot == color) return false;
    slot = color;
    return true;
}
}

UnlockBar::UnlockBar(QWidget *parent) :
    QWidget(parent),
    m_threshold(DEFAULT_THRESHOLD),
    m_relock_ms(DEFAULT_RELOCK_MS),
    m_unlocked(false),
    m_pressed(false),
    m_thumb_x(0),
    m_press_dx(0),
    m_relock_timer(new QTimer(this)),
    m_track_bg_color(0x12, 0x10, 0x0E),
    m_stripe_color(0xE6, 0xB3, 0x45),
    m_track_edge_color(0x38, 0x2D, 0x25),
    m_thumb_locked_color(0x1C, 0x18, 0x15),
    m_thumb_unlocked_color(0x1C, 0x18, 0x15),
    m_thumb_edge_armed_color(0x8F, 0x6B, 0x4D),
    m_thumb_edge_pass_color(0x8F, 0x6B, 0x4D)
{
    setObjectName("UnlockBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(160, 32);
    setCursor(Qt::PointingHandCursor);

    m_relock_timer->setSingleShot(true);
    connect(m_relock_timer, &QTimer::timeout, this, &UnlockBar::on_relock_timeout);
}

void UnlockBar::set_unlock_threshold(double fraction)
{
    if (fraction <= 0.0)  fraction = 0.01;
    if (fraction >  1.0)  fraction = 1.0;
    m_threshold = fraction;
}

void UnlockBar::set_relock_ms(int ms)
{
    if (ms < 0) ms = 0;
    m_relock_ms = ms;
    if (ms == 0) m_relock_timer->stop();
}

void UnlockBar::set_track_background_color(const QColor &color)
{
    if (update_color(m_track_bg_color, color)) update();
}

void UnlockBar::set_stripe_color(const QColor &color)
{
    if (update_color(m_stripe_color, color)) update();
}

void UnlockBar::set_track_edge_color(const QColor &color)
{
    if (update_color(m_track_edge_color, color)) update();
}

void UnlockBar::set_thumb_locked_color(const QColor &color)
{
    if (update_color(m_thumb_locked_color, color)) update();
}

void UnlockBar::set_thumb_unlocked_color(const QColor &color)
{
    if (update_color(m_thumb_unlocked_color, color)) update();
}

void UnlockBar::set_thumb_edge_armed_color(const QColor &color)
{
    if (update_color(m_thumb_edge_armed_color, color)) update();
}

void UnlockBar::set_thumb_edge_pass_color(const QColor &color)
{
    if (update_color(m_thumb_edge_pass_color, color)) update();
}

int UnlockBar::track_left() const  { return MARGIN_H; }
int UnlockBar::track_right() const { return width() - MARGIN_H; }
int UnlockBar::track_width() const { return qMax(0, track_right() - track_left()); }

int UnlockBar::thumb_width() const
{
    // Thumb width grows with height but never exceeds half the track.
    const int h = height() - 2 * MARGIN_V;
    return qMax(16, qMin(track_width() / 2, h));
}

void UnlockBar::set_thumb_x(int x)
{
    const int min_x = track_left();
    const int max_x = track_right() - thumb_width();
    const int clamped = qBound(min_x, x, qMax(min_x, max_x));
    if (m_thumb_x == clamped) return;
    m_thumb_x = clamped;
    update();
}

void UnlockBar::spring_back()
{
    set_thumb_x(track_left());
}

void UnlockBar::relock()
{
    m_relock_timer->stop();
    spring_back();
    if (!m_unlocked) return;
    m_unlocked = false;
    emit locked();
}

void UnlockBar::on_relock_timeout()
{
    if (!m_unlocked) return;
    spring_back();
    m_unlocked = false;
    emit locked();
}

void UnlockBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect track_rect(track_left(), MARGIN_V,
                           track_width(), height() - 2 * MARGIN_V);
    if (track_rect.isEmpty()) return;

    // Striped background (bg-raised + warning-amber, 45° stripes).
    p.save();
    p.setClipRect(track_rect);
    p.fillRect(track_rect, m_track_bg_color);
    p.setPen(Qt::NoPen);
    p.setBrush(m_stripe_color);
    const int stripe_w = 10;
    for (int x = track_rect.left() - track_rect.height();
         x < track_rect.right();
         x += stripe_w * 2) {
        QPolygon poly;
        poly << QPoint(x, track_rect.bottom())
             << QPoint(x + stripe_w, track_rect.bottom())
             << QPoint(x + stripe_w + track_rect.height(), track_rect.top())
             << QPoint(x + track_rect.height(), track_rect.top());
        p.drawPolygon(poly);
    }
    p.restore();

    // Border around the track.
    p.setPen(QPen(m_track_edge_color, 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRect(track_rect);

    // Thumb is drawn last so the track border and stripes never sit on top
    // of the draggable handle.
    const QRect thumb_rect(m_thumb_x, MARGIN_V,
                           thumb_width(), height() - 2 * MARGIN_V);
    p.setPen(QPen(m_unlocked ? m_thumb_edge_pass_color : m_thumb_edge_armed_color, 1.5));
    p.setBrush(m_unlocked ? m_thumb_unlocked_color : m_thumb_locked_color);
    p.drawRoundedRect(thumb_rect, 4, 4);
}

void UnlockBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int x = event->pos().x();
    const QRect thumb_rect(m_thumb_x, MARGIN_V, thumb_width(), height() - 2 * MARGIN_V);
    if (thumb_rect.contains(event->pos())) {
        m_pressed = true;
        m_press_dx = x - m_thumb_x;
    }
    QWidget::mousePressEvent(event);
}

void UnlockBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed) set_thumb_x(event->pos().x() - m_press_dx);
    QWidget::mouseMoveEvent(event);
}

void UnlockBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_pressed) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_pressed = false;

    const int tw = track_width();
    const int max_travel = qMax(1, tw - thumb_width());
    const double frac = double(m_thumb_x - track_left()) / double(max_travel);

    if (frac >= m_threshold) {
        // Unlock: snap the thumb to the right end, emit once and start the relock timer.
        // Unlock gestures while already unlocked do not emit again.
        set_thumb_x(track_left() + max_travel);
        if (!m_unlocked) {
            m_unlocked = true;
            emit unlocked();
        }
        if (m_relock_ms > 0) m_relock_timer->start(m_relock_ms);
    }
    else {
        // Short drag: spring back, no signal.
        spring_back();
    }

    QWidget::mouseReleaseEvent(event);
}
