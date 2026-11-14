#ifndef JOYSTICKWIDGET_H
#define JOYSTICKWIDGET_H

#include <QColor>
#include <QPointF>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;

// Virtual joystick of the modern optics panel: a circular base with a stick that
// springs back to the centre. Emits a continuous (dx, dy) in [-1, +1] per axis,
// never a discrete direction.
class JoystickWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor baseEdgeColor READ get_base_edge_color WRITE set_base_edge_color)
    Q_PROPERTY(QColor baseDarkColor READ get_base_dark_color WRITE set_base_dark_color)
    Q_PROPERTY(QColor baseLightColor READ get_base_light_color WRITE set_base_light_color)
    Q_PROPERTY(QColor baseGrooveColor READ get_base_groove_color WRITE set_base_groove_color)
    Q_PROPERTY(QColor glyphColor READ get_glyph_color WRITE set_glyph_color)
    Q_PROPERTY(QColor handleEdgeColor READ get_handle_edge_color WRITE set_handle_edge_color)
    Q_PROPERTY(QColor domeDarkColor READ get_dome_dark_color WRITE set_dome_dark_color)
    Q_PROPERTY(QColor domeLightColor READ get_dome_light_color WRITE set_dome_light_color)
public:
    explicit JoystickWidget(QWidget *parent = nullptr);

    // Deadzone as a fraction of the base radius. Inside this radius, the
    // reported direction snaps to (0, 0) so small jitter doesn't spam
    // consumers. Default 0.1. Clamped to [0, 1).
    void   set_deadzone(double fraction);
    double get_deadzone() const { return m_deadzone_frac; }

    QPointF get_direction() const { return m_direction; }

    QColor get_base_edge_color() const { return m_base_edge_color; }
    QColor get_base_dark_color() const { return m_base_dark_color; }
    QColor get_base_light_color() const { return m_base_light_color; }
    QColor get_base_groove_color() const { return m_base_groove_color; }
    QColor get_glyph_color() const { return m_glyph_color; }
    QColor get_handle_edge_color() const { return m_handle_edge_color; }
    QColor get_dome_dark_color() const { return m_dome_dark_color; }
    QColor get_dome_light_color() const { return m_dome_light_color; }

    void set_base_edge_color(const QColor &color);
    void set_base_dark_color(const QColor &color);
    void set_base_light_color(const QColor &color);
    void set_base_groove_color(const QColor &color);
    void set_glyph_color(const QColor &color);
    void set_handle_edge_color(const QColor &color);
    void set_dome_dark_color(const QColor &color);
    void set_dome_light_color(const QColor &color);

    // Pure helpers, testable without a widget. Clamps a vector's length to radius.
    static QPointF clamp_to_circle(QPointF v, double radius);
    // Compute normalized direction in [-1, +1] from a stick position in
    // widget-local coords. Returns (0, 0) when inside the deadzone.
    static QPointF compute_direction(QPointF stick_local,
                                     QPointF center,
                                     double  radius,
                                     double  deadzone_frac);

signals:
    void direction_changed(double dx, double dy);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPointF base_center() const;
    double  base_radius() const;

    void    update_stick(const QPointF &stick_local);
    void    emit_direction(const QPointF &dir);

    QPointF m_stick_local;   // stick center in widget coords
    QPointF m_direction;     // last emitted (dx, dy)
    double  m_deadzone_frac;
    bool    m_pressed;
    QColor  m_base_edge_color;
    QColor  m_base_dark_color;
    QColor  m_base_light_color;
    QColor  m_base_groove_color;
    QColor  m_glyph_color;
    QColor  m_handle_edge_color;
    QColor  m_dome_dark_color;
    QColor  m_dome_light_color;
};

#endif // JOYSTICKWIDGET_H
