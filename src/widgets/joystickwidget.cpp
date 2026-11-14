#include "joystickwidget.h"

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QRadialGradient>
#include <QtMath>

namespace {
const double STICK_RADIUS_FRAC = 0.25; // stick cap radius as fraction of base radius
const double DEFAULT_DEADZONE = 0.1;

bool update_color(QColor &slot, const QColor &color)
{
    if (slot == color) return false;
    slot = color;
    return true;
}
}

JoystickWidget::JoystickWidget(QWidget *parent) :
    QWidget(parent),
    m_stick_local(),
    m_direction(0.0, 0.0),
    m_deadzone_frac(DEFAULT_DEADZONE),
    m_pressed(false),
    m_base_edge_color(0x22, 0x22, 0x22),
    m_base_dark_color(0x08, 0x08, 0x08),
    m_base_light_color(0x1E, 0x1E, 0x1E),
    m_base_groove_color(0x25, 0x25, 0x25),
    m_glyph_color(0x5C, 0x4A, 0x3D),
    m_handle_edge_color(0xE0, 0x92, 0x4A),
    m_dome_dark_color(0x08, 0x08, 0x08),
    m_dome_light_color(0x2A, 0x2A, 0x2A)
{
    setObjectName("JoystickWidget");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumSize(80, 80);
    setCursor(Qt::OpenHandCursor);
    m_stick_local = base_center();
}

void JoystickWidget::set_base_edge_color(const QColor &color)
{
    if (update_color(m_base_edge_color, color)) update();
}

void JoystickWidget::set_base_dark_color(const QColor &color)
{
    if (update_color(m_base_dark_color, color)) update();
}

void JoystickWidget::set_base_light_color(const QColor &color)
{
    if (update_color(m_base_light_color, color)) update();
}

void JoystickWidget::set_base_groove_color(const QColor &color)
{
    if (update_color(m_base_groove_color, color)) update();
}

void JoystickWidget::set_glyph_color(const QColor &color)
{
    if (update_color(m_glyph_color, color)) update();
}

void JoystickWidget::set_handle_edge_color(const QColor &color)
{
    if (update_color(m_handle_edge_color, color)) update();
}

void JoystickWidget::set_dome_dark_color(const QColor &color)
{
    if (update_color(m_dome_dark_color, color)) update();
}

void JoystickWidget::set_dome_light_color(const QColor &color)
{
    if (update_color(m_dome_light_color, color)) update();
}

void JoystickWidget::set_deadzone(double fraction)
{
    if (fraction < 0.0) fraction = 0.0;
    if (fraction >= 1.0) fraction = 0.999;
    m_deadzone_frac = fraction;
}

QPointF JoystickWidget::clamp_to_circle(QPointF v, double radius)
{
    if (radius <= 0.0) return QPointF(0.0, 0.0);
    const double len_sq = v.x() * v.x() + v.y() * v.y();
    if (len_sq <= radius * radius) return v;
    const double len = std::sqrt(len_sq);
    return v * (radius / len);
}

QPointF JoystickWidget::compute_direction(QPointF stick_local,
                                        QPointF center,
                                        double  radius,
                                        double  deadzone_frac)
{
    if (radius <= 0.0) return QPointF(0.0, 0.0);

    QPointF v = stick_local - center;
    // Magnitude in pixel space.
    const double len = std::sqrt(v.x() * v.x() + v.y() * v.y());

    // Deadzone test in pixel-space (fraction of radius).
    const double dz = deadzone_frac * radius;
    if (len <= dz) return QPointF(0.0, 0.0);

    // Clamp magnitude to radius, then normalize to [-1, +1].
    if (len > radius) v *= (radius / len);
    return QPointF(v.x() / radius, v.y() / radius);
}

QPointF JoystickWidget::base_center() const
{
    return QPointF(width() / 2.0, height() / 2.0);
}

double JoystickWidget::base_radius() const
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

    const QPointF c = base_center();
    const double  r = base_radius();
    if (r <= 0.0) return;

    // Base fill: radial gradient (highlight at upper-left, dark at lower-right).
    QRadialGradient base_g(c.x() - r * 0.16, c.y() - r * 0.24, r * 1.4);
    base_g.setColorAt(0.0, m_base_light_color);
    base_g.setColorAt(1.0, m_base_dark_color);
    p.setPen(QPen(m_base_edge_color, 2.0));
    p.setBrush(base_g);
    p.drawEllipse(c, r, r);

    // three concentric inset grooves
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(m_base_groove_color, 1.0));
    for (double frac : {0.22, 0.42, 0.62}) {
        p.drawEllipse(c, r * frac, r * frac);
    }

    // Direction glyphs at N/S/W/E.
    QFont gf = p.font();
    gf.setPointSizeF(qMax(6.0, r * 0.10));
    p.setFont(gf);
    p.setPen(m_glyph_color);
    const double gpad = qMax(7.0, r * 0.10);
    auto draw_glyph = [&](const QString &g, QPointF at) {
        const QRectF box(at.x() - 8, at.y() - 8, 16, 16);
        p.drawText(box, Qt::AlignCenter, g);
    };
    draw_glyph(QString::fromUtf8("\xe2\x96\xb2"), QPointF(c.x(),      c.y() - r + gpad)); // up triangle
    draw_glyph(QString::fromUtf8("\xe2\x96\xbc"), QPointF(c.x(),      c.y() + r - gpad)); // down triangle
    draw_glyph(QString::fromUtf8("\xe2\x97\x84"), QPointF(c.x() - r + gpad, c.y()));      // left triangle
    draw_glyph(QString::fromUtf8("\xe2\x96\xba"), QPointF(c.x() + r - gpad, c.y()));      // right triangle

    // Stick: hollow accent ring + inner dome (radial gradient).
    const double stick_r = r * (STICK_RADIUS_FRAC + 0.21); // about 46% of the base radius
    const double dome_r  = stick_r * (40.0 / 56.0);

    p.setPen(QPen(m_handle_edge_color, 2.0));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(m_stick_local, stick_r, stick_r);

    QRadialGradient dome_g(m_stick_local.x() - dome_r * 0.24,
                           m_stick_local.y() - dome_r * 0.32, dome_r * 1.4);
    dome_g.setColorAt(0.0, m_dome_light_color);
    dome_g.setColorAt(1.0, m_dome_dark_color);
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
    update_stick(event->pos());
    QWidget::mousePressEvent(event);
}

void JoystickWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed) update_stick(event->pos());
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
    m_stick_local = base_center();
    emit_direction(QPointF(0.0, 0.0));
    update();
    QWidget::mouseReleaseEvent(event);
}

void JoystickWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // The constructor sets m_stick_local before the first layout, when the size is
    // still 0. Re-centre on every resize while idle so the handle follows the centre.
    if (!m_pressed) m_stick_local = base_center();
}

void JoystickWidget::update_stick(const QPointF &stick_local)
{
    const QPointF c = base_center();
    const double  r = base_radius();
    // Clamp stick visual position to within the base circle.
    const QPointF clamped = c + clamp_to_circle(stick_local - c, r);
    m_stick_local = clamped;

    const QPointF dir = compute_direction(stick_local, c, r, m_deadzone_frac);
    emit_direction(dir);
    update();
}

void JoystickWidget::emit_direction(const QPointF &dir)
{
    if (m_direction == dir) return;
    m_direction = dir;
    emit direction_changed(dir.x(), dir.y());
}
