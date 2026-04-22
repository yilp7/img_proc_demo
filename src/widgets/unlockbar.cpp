#include "unlockbar.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

namespace {
const int    kMarginH          = 4;
const int    kMarginV          = 4;
const int    kDefaultRelockMs  = 3000;
const double kDefaultThreshold = 0.9;
}

UnlockBar::UnlockBar(QWidget *parent) :
    QWidget(parent),
    m_threshold(kDefaultThreshold),
    m_relock_ms(kDefaultRelockMs),
    m_unlocked(false),
    m_pressed(false),
    m_thumb_x(0),
    m_press_dx(0),
    m_relock_timer(new QTimer(this))
{
    setObjectName("UnlockBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(160, 32);
    setCursor(Qt::PointingHandCursor);

    m_relock_timer->setSingleShot(true);
    connect(m_relock_timer, &QTimer::timeout, this, &UnlockBar::onRelockTimeout);
}

void UnlockBar::setUnlockThreshold(double fraction)
{
    if (fraction <= 0.0)  fraction = 0.01;
    if (fraction >  1.0)  fraction = 1.0;
    m_threshold = fraction;
}

void UnlockBar::setRelockMs(int ms)
{
    if (ms < 0) ms = 0;
    m_relock_ms = ms;
    if (ms == 0) m_relock_timer->stop();
}

int UnlockBar::trackLeft() const  { return kMarginH; }
int UnlockBar::trackRight() const { return width() - kMarginH; }
int UnlockBar::trackWidth() const { return qMax(0, trackRight() - trackLeft()); }

int UnlockBar::thumbWidth() const
{
    // Thumb width grows with height but never exceeds half the track.
    const int h = height() - 2 * kMarginV;
    return qMax(16, qMin(trackWidth() / 2, h));
}

void UnlockBar::setThumbX(int x)
{
    const int min_x = trackLeft();
    const int max_x = trackRight() - thumbWidth();
    const int clamped = qBound(min_x, x, qMax(min_x, max_x));
    if (m_thumb_x == clamped) return;
    m_thumb_x = clamped;
    update();
}

void UnlockBar::springBack()
{
    setThumbX(trackLeft());
}

void UnlockBar::relock()
{
    m_relock_timer->stop();
    springBack();
    if (!m_unlocked) return;
    m_unlocked = false;
    emit locked();
}

void UnlockBar::onRelockTimeout()
{
    if (!m_unlocked) return;
    springBack();
    m_unlocked = false;
    emit locked();
}

void UnlockBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect track_rect(trackLeft(), kMarginV,
                           trackWidth(), height() - 2 * kMarginV);
    if (track_rect.isEmpty()) return;

    // Striped background (black + amber, 45° stripes).
    p.save();
    p.setClipRect(track_rect);
    p.fillRect(track_rect, QColor(0x1a, 0x1a, 0x1a));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xff, 0xc1, 0x07));
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

    // Thumb.
    const QRect thumb_rect(m_thumb_x, kMarginV,
                           thumbWidth(), height() - 2 * kMarginV);
    p.setPen(QPen(QColor(0xff, 0xff, 0xff), 1.5));
    p.setBrush(m_unlocked ? QColor(0x4c, 0xd9, 0x64) : QColor(0x2b, 0x2b, 0x2b));
    p.drawRoundedRect(thumb_rect, 4, 4);

    // Border around the track.
    p.setPen(QPen(QColor(0xff, 0xff, 0xff), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRect(track_rect);
}

void UnlockBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int x = event->pos().x();
    const QRect thumb_rect(m_thumb_x, kMarginV, thumbWidth(), height() - 2 * kMarginV);
    if (thumb_rect.contains(event->pos())) {
        m_pressed = true;
        m_press_dx = x - m_thumb_x;
    }
    QWidget::mousePressEvent(event);
}

void UnlockBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed) setThumbX(event->pos().x() - m_press_dx);
    QWidget::mouseMoveEvent(event);
}

void UnlockBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_pressed) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_pressed = false;

    const int tw = trackWidth();
    const int max_travel = qMax(1, tw - thumbWidth());
    const double frac = double(m_thumb_x - trackLeft()) / double(max_travel);

    if (frac >= m_threshold) {
        // Unlock: snap thumb to right end, emit once, start relock timer
        // (unless auto-relock is disabled or already unlocked — repeated
        // unlock gestures while unlocked do not re-fire the signal).
        setThumbX(trackLeft() + max_travel);
        if (!m_unlocked) {
            m_unlocked = true;
            emit unlocked();
        }
        if (m_relock_ms > 0) m_relock_timer->start(m_relock_ms);
    } else {
        // Short drag: spring back, no signal.
        springBack();
    }

    QWidget::mouseReleaseEvent(event);
}
