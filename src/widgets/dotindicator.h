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
    Q_PROPERTY(State state READ state WRITE setState)
    Q_PROPERTY(QColor offColor READ offColor WRITE setOffColor)
    Q_PROPERTY(QColor okColor READ okColor WRITE setOkColor)
    Q_PROPERTY(QColor warnColor READ warnColor WRITE setWarnColor)
    Q_PROPERTY(QColor errColor READ errColor WRITE setErrColor)
public:
    enum State {
        Off  = 0,
        Ok   = 1,
        Warn = 2,
        Err  = 3
    };
    Q_ENUM(State)

    explicit DotIndicator(QWidget *parent = nullptr);

    State state() const { return m_state; }
    void  setState(State s);

    QColor offColor() const { return m_colors[Off]; }
    QColor okColor() const { return m_colors[Ok]; }
    QColor warnColor() const { return m_colors[Warn]; }
    QColor errColor() const { return m_colors[Err]; }

    void setOffColor(const QColor &color);
    void setOkColor(const QColor &color);
    void setWarnColor(const QColor &color);
    void setErrColor(const QColor &color);

    // Per-state color override; defaults ship as neutral placeholders so
    // Sprint 20 QSS can replace them without touching this class.
    void setColorForState(State s, const QColor &color);

public slots:
    // Maps StatusIcon::STATUS int values (0–4) → State so ControlPortThread can drive this dot.
    // 0:NONE→Off  1:NOT_CONNECTED→Err  2:DISCONNECTED→Err  3:RECONNECTING→Warn  4:CONNECTED→Ok
    void update_status(int status);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    State  m_state;
    QColor m_colors[4];
};

#endif // DOTINDICATOR_H
