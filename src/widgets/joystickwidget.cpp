#include "joystickwidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>

namespace {
const double kStickRadiusFrac = 0.25; // stick cap radius as fraction of base radius
const double kDefaultDeadzone = 0.1;
}

JoystickWidget::JoystickWidget(QWidget *parent) :
    QWidget(parent),
    m_stick_local(),
    m_direction(0.0, 0.0),
    m_deadzone_frac(kDefaultDeadzone),
    m_pressed(false)
{
    setObjectName("JoystickWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(80, 80);
    setCursor(Qt::OpenHandCursor);
    m_stick_local = baseCenter();
}

void JoystickWidget::setDeadzone(double fraction)
{
    if (fraction < 0.0) fraction = 0.0;
    if (fraction >= 1.0) fraction = 0.999;
    m_deadzone_frac = fraction;
}

QPointF JoystickWidget::clampToCircle(QPointF v, double radius)
{
    if (radius <= 0.0) return QPointF(0.0, 0.0);
    const double len_sq = v.x() * v.x() + v.y() * v.y();
    if (len_sq <= radius * radius) return v;
    const double len = std::sqrt(len_sq);
    return v * (radius / len);
}

QPointF JoystickWidget::computeDirection(QPointF stickLocal,
                                        QPointF center,
                                        double  radius,
                                        double  deadzoneFrac)
{
    if (radius <= 0.0) return QPointF(0.0, 0.0);

    QPointF v = stickLocal - center;
    // Magnitude in pixel space.
    const double len = std::sqrt(v.x() * v.x() + v.y() * v.y());

    // Deadzone test in pixel-space (fraction of radius).
    const double dz = deadzoneFrac * radius;
    if (len <= dz) return QPointF(0.0, 0.0);

    // Clamp magnitude to radius, then normalize to [-1, +1].
    if (len > radius) v *= (radius / len);
    return QPointF(v.x() / radius, v.y() / radius);
}

QPointF JoystickWidget::baseCenter() const
{
    return QPointF(width() / 2.0, height() / 2.0);
}

double JoystickWidget::baseRadius() const
{
    // Reserve 2 px on each side for the ring stroke so the stick doesn't
    // clip at extremes.
    const double side = qMin(width(), height()) / 2.0 - 2.0;
    return qMax(0.0, side);
}

void JoystickWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPointF c = baseCenter();
    const double  r = baseRadius();
    if (r <= 0.0) return;

    // Base ring.
    p.setPen(QPen(QColor(0xff, 0xff, 0xff), 2.0));
    p.setBrush(QColor(0x22, 0x22, 0x22));
    p.drawEllipse(c, r, r);

    // Deadzone guide (faint).
    if (m_deadzone_frac > 0.0) {
        p.setPen(QPen(QColor(0x55, 0x55, 0x55), 1.0, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, r * m_deadzone_frac, r * m_deadzone_frac);
    }

    // Stick cap.
    const double stick_r = r * kStickRadiusFrac;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x3d, 0x8b, 0xff));
    p.drawEllipse(m_stick_local, stick_r, stick_r);
}

void JoystickWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = true;
    setCursor(Qt::ClosedHandCursor);
    updateStick(event->pos());
    QWidget::mousePressEvent(event);
}

void JoystickWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed) updateStick(event->pos());
    QWidget::mouseMoveEvent(event);
}

void JoystickWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_pressed = false;
    setCursor(Qt::OpenHandCursor);
    // Snap stick back to center and emit zero vector.
    m_stick_local = baseCenter();
    emitDirection(QPointF(0.0, 0.0));
    update();
    QWidget::mouseReleaseEvent(event);
}

void JoystickWidget::updateStick(const QPointF &stickLocal)
{
    const QPointF c = baseCenter();
    const double  r = baseRadius();
    // Clamp stick visual position to within the base circle.
    const QPointF clamped = c + clampToCircle(stickLocal - c, r);
    m_stick_local = clamped;

    const QPointF dir = computeDirection(stickLocal, c, r, m_deadzone_frac);
    emitDirection(dir);
    update();
}

void JoystickWidget::emitDirection(const QPointF &dir)
{
    if (m_direction == dir) return;
    m_direction = dir;
    emit directionChanged(dir.x(), dir.y());
}
