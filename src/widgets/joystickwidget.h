#ifndef JOYSTICKWIDGET_H
#define JOYSTICKWIDGET_H

#include <QWidget>
#include <QPointF>

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
public:
    explicit JoystickWidget(QWidget *parent = nullptr);

    // Deadzone as a fraction of the base radius. Inside this radius, the
    // reported direction snaps to (0, 0) so small jitter doesn't spam
    // consumers. Default 0.1. Clamped to [0, 1).
    void   setDeadzone(double fraction);
    double deadzone() const { return m_deadzone_frac; }

    QPointF direction() const { return m_direction; }

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
};

#endif // JOYSTICKWIDGET_H
