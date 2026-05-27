#include "joystickwidget.h"

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QRadialGradient>
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
    // Theme tokens — match prototype v3 / style_dark.qss.
    static const QColor kBaseEdge   (0x22, 0x22, 0x22);  // hw / dark ring edge
    static const QColor kBaseDark   (0x08, 0x08, 0x08);  // inner gradient end
    static const QColor kBaseLight  (0x1E, 0x1E, 0x1E);  // inner gradient start
    static const QColor kBaseGroove (0x25, 0x25, 0x25);  // concentric ring stroke
    static const QColor kGlyph      (0x5C, 0x4A, 0x3D);  // joy-dir (text-dim)
    static const QColor kHandleEdge (0xE0, 0x92, 0x4A);  // accent (joy-handle)
    static const QColor kDomeDark   (0x08, 0x08, 0x08);  // dome gradient end
    static const QColor kDomeLight  (0x2A, 0x2A, 0x2A);  // dome gradient start

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPointF c = baseCenter();
    const double  r = baseRadius();
    if (r <= 0.0) return;

    // Base fill: radial gradient (highlight at upper-left, dark at lower-right).
    QRadialGradient base_g(c.x() - r * 0.16, c.y() - r * 0.24, r * 1.4);
    base_g.setColorAt(0.0, kBaseLight);
    base_g.setColorAt(1.0, kBaseDark);
    p.setPen(QPen(kBaseEdge, 2.0));
    p.setBrush(base_g);
    p.drawEllipse(c, r, r);

    // Concentric inset grooves (3) — match prototype radial-gradient rings.
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(kBaseGroove, 1.0));
    for (double frac : {0.22, 0.42, 0.62}) {
        p.drawEllipse(c, r * frac, r * frac);
    }

    // Direction glyphs at N/S/W/E.
    QFont gf = p.font();
    gf.setPointSizeF(qMax(6.0, r * 0.10));
    p.setFont(gf);
    p.setPen(kGlyph);
    const double gpad = qMax(7.0, r * 0.10);
    auto drawGlyph = [&](const QString &g, QPointF at) {
        const QRectF box(at.x() - 8, at.y() - 8, 16, 16);
        p.drawText(box, Qt::AlignCenter, g);
    };
    drawGlyph(QString::fromUtf8("\xe2\x96\xb2"), QPointF(c.x(),      c.y() - r + gpad)); // ▲
    drawGlyph(QString::fromUtf8("\xe2\x96\xbc"), QPointF(c.x(),      c.y() + r - gpad)); // ▼
    drawGlyph(QString::fromUtf8("\xe2\x97\x84"), QPointF(c.x() - r + gpad, c.y()));      // ◄
    drawGlyph(QString::fromUtf8("\xe2\x96\xba"), QPointF(c.x() + r - gpad, c.y()));      // ►

    // Stick: hollow accent ring + inner dome (radial gradient).
    const double stick_r = r * (kStickRadiusFrac + 0.21); // ~46% — matches 56/120 in proto
    const double dome_r  = stick_r * (40.0 / 56.0);

    p.setPen(QPen(kHandleEdge, 2.0));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(m_stick_local, stick_r, stick_r);

    QRadialGradient dome_g(m_stick_local.x() - dome_r * 0.24,
                           m_stick_local.y() - dome_r * 0.32, dome_r * 1.4);
    dome_g.setColorAt(0.0, kDomeLight);
    dome_g.setColorAt(1.0, kDomeDark);
    p.setPen(Qt::NoPen);
    p.setBrush(dome_g);
    p.drawEllipse(m_stick_local, dome_r, dome_r);
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

void JoystickWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Constructor sets m_stick_local = baseCenter() before layout, when
    // width()/height() are still 0 — handle would render at (0, 0).
    // Re-snap on every resize when idle so the handle tracks the true center.
    if (!m_pressed) m_stick_local = baseCenter();
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
