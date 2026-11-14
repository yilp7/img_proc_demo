#ifndef DOTINDICATOR_H
#define DOTINDICATOR_H

#include <QWidget>
#include <QColor>

class QPaintEvent;

// Small colored dot with soft glow. Used in TopBar for connection status
// (TCU, camera, laser, lens, PTZ, rangefinder).
class DotIndicator : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(State state READ get_state WRITE set_state)
    Q_PROPERTY(QColor offColor READ get_off_color WRITE set_off_color)
    Q_PROPERTY(QColor okColor READ get_ok_color WRITE set_ok_color)
    Q_PROPERTY(QColor warnColor READ get_warn_color WRITE set_warn_color)
    Q_PROPERTY(QColor errColor READ get_err_color WRITE set_err_color)
public:
    enum State {
        OFF  = 0,
        OK   = 1,
        WARN = 2,
        ERR  = 3
    };
    Q_ENUM(State)

    explicit DotIndicator(QWidget *parent = nullptr);

    State get_state() const { return m_state; }
    void  set_state(State s);

    QColor get_off_color() const { return m_colors[OFF]; }
    QColor get_ok_color() const { return m_colors[OK]; }
    QColor get_warn_color() const { return m_colors[WARN]; }
    QColor get_err_color() const { return m_colors[ERR]; }

    void set_off_color(const QColor &color);
    void set_ok_color(const QColor &color);
    void set_warn_color(const QColor &color);
    void set_err_color(const QColor &color);

    // Per-state colour override, so the stylesheet can set the colours without touching this class.
    void set_color_for_state(State s, const QColor &color);

public slots:
    // Maps the port status values 0-4 to a State so ControlPortThread can drive the dot:
    // 0 none: OFF, 1 not connected and 2 disconnected: ERR, 3 reconnecting: WARN, 4 connected: OK.
    void update_status(int status);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    State  m_state;
    QColor m_colors[4];
};

#endif // DOTINDICATOR_H
