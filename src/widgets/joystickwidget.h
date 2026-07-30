#ifndef JOYSTICKWIDGET_H
#define JOYSTICKWIDGET_H

#include <QColor>
#include <QPointF>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;

// Virtual joystick for PTZ Modern mode. Circular base with a draggable stick
// that snaps back to center on release. Emits continuous (dx, dy) normalized
// to [-1, +1] on each axis.
//
// This widget intentionally does NOT emit a discrete 8-direction enum — that
// concept belongs to Classic-mode PTZ direction buttons, not the joystick.
class JoystickWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor baseEdgeColor READ baseEdgeColor WRITE setBaseEdgeColor)
    Q_PROPERTY(QColor baseDarkColor READ baseDarkColor WRITE setBaseDarkColor)
    Q_PROPERTY(QColor baseLightColor READ baseLightColor WRITE setBaseLightColor)
    Q_PROPERTY(QColor baseGrooveColor READ baseGrooveColor WRITE setBaseGrooveColor)
    Q_PROPERTY(QColor glyphColor READ glyphColor WRITE setGlyphColor)
    Q_PROPERTY(QColor handleEdgeColor READ handleEdgeColor WRITE setHandleEdgeColor)
    Q_PROPERTY(QColor domeDarkColor READ domeDarkColor WRITE setDomeDarkColor)
    Q_PROPERTY(QColor domeLightColor READ domeLightColor WRITE setDomeLightColor)
public:
    explicit JoystickWidget(QWidget *parent = nullptr);

    // Deadzone as a fraction of the base radius. Inside this radius, the
    // reported direction snaps to (0, 0) so small jitter doesn't spam
    // consumers. Default 0.1. Clamped to [0, 1).
    void   setDeadzone(double fraction);
    double deadzone() const { return m_deadzone_frac; }

    QPointF direction() const { return m_direction; }

    QColor baseEdgeColor() const { return m_base_edge_color; }
    QColor baseDarkColor() const { return m_base_dark_color; }
    QColor baseLightColor() const { return m_base_light_color; }
    QColor baseGrooveColor() const { return m_base_groove_color; }
    QColor glyphColor() const { return m_glyph_color; }
    QColor handleEdgeColor() const { return m_handle_edge_color; }
    QColor domeDarkColor() const { return m_dome_dark_color; }
    QColor domeLightColor() const { return m_dome_light_color; }

    void setBaseEdgeColor(const QColor &color);
    void setBaseDarkColor(const QColor &color);
    void setBaseLightColor(const QColor &color);
    void setBaseGrooveColor(const QColor &color);
    void setGlyphColor(const QColor &color);
    void setHandleEdgeColor(const QColor &color);
    void setDomeDarkColor(const QColor &color);
    void setDomeLightColor(const QColor &color);

    // Pure math helpers — testable without constructing a widget.
    // Clamp a vector's magnitude to `radius`.
    static QPointF clampToCircle(QPointF v, double radius);
    // Compute normalized direction in [-1, +1] from a stick position in
    // widget-local coords. Returns (0, 0) when inside the deadzone.
    static QPointF computeDirection(QPointF stickLocal,
                                    QPointF center,
                                    double  radius,
                                    double  deadzoneFrac);

signals:
    void directionChanged(double dx, double dy);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPointF baseCenter() const;
    double  baseRadius() const;

    void    updateStick(const QPointF &stickLocal);
    void    emitDirection(const QPointF &dir);

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
