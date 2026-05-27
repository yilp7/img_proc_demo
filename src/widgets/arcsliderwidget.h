#ifndef ARCSLIDERWIDGET_H
#define ARCSLIDERWIDGET_H

#include <QColor>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;

// Vertical slider whose track is rendered as a curved arc. Used in triplicate
// for zoom / focus / iris in PTZ Modern mode.
//
// Convention: top of track = maximum, bottom = minimum (matches QSlider with
// Qt::Vertical + invertedAppearance=false). Value↔pixel mapping is 1D and
// linear — the arc curvature is purely cosmetic, handled in paintEvent.
class ArcSliderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ArcSliderWidget(QWidget *parent = nullptr);

    void setRange(int min, int max);
    int  minimum() const { return m_min; }
    int  maximum() const { return m_max; }

    void setValue(int value);
    int  value() const { return m_value; }

    // Switch to speed/joystick mode: thumb anchors to midpoint, drag emits
    // signed speed scaled exponentially from drag distance, quantized to a
    // step of 4 (min |speed| = 4 once off midpoint), released → snaps to 0.
    // Calls setRange(-maxSpeed, +maxSpeed) and setValue(0) internally.
    void setSpeedMode(int maxSpeed);
    bool isSpeedMode() const { return m_speed_mode; }

    // Cosmetic only: color used for the filled portion and thumb fill.
    // Does not affect value mapping or signals.
    void   setActiveColor(const QColor &c);
    QColor activeColor() const { return m_active_color; }

    // Pure math helpers — testable without constructing a widget.
    // y=0 is top (=max); y=trackHeight is bottom (=min).
    static int pixelToValue(int y, int trackHeight, int min, int max);
    static int valueToPixel(int value, int trackHeight, int min, int max);

signals:
    void valueChanged(int value);
    // Speed-mode only: emitted when the quantized signed speed changes.
    // sign > 0 = up-direction, sign < 0 = down-direction, 0 = midpoint/stop.
    void speedChanged(int signedSpeed);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int  trackHeight() const;
    int  trackTop()    const;
    void setValueFromPixel(int local_y);
    int  pixelToSignedSpeed(int local_y) const;
    void applySpeedFromPixel(int local_y);

    int    m_min;
    int    m_max;
    int    m_value;
    bool   m_pressed;
    bool   m_speed_mode;
    int    m_max_speed;
    int    m_last_signed_speed;
    QColor m_active_color;
};

#endif // ARCSLIDERWIDGET_H
