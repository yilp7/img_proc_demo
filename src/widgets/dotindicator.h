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

    // Per-state color override; defaults ship as neutral placeholders so
    // Sprint 20 QSS can replace them without touching this class.
    void setColorForState(State s, const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    State  m_state;
    QColor m_colors[4];
};

#endif // DOTINDICATOR_H
