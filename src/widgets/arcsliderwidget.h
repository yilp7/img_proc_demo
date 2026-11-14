#ifndef ARCSLIDERWIDGET_H
#define ARCSLIDERWIDGET_H

#include <QColor>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;

// Vertical slider drawn as a curved arc, used for zoom, focus and iris in the
// modern optics panel. Top is maximum. The value mapping is linear; the curve is
// only drawn.
class ArcSliderWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor activeColor READ get_active_color WRITE set_active_color)
    Q_PROPERTY(QColor inactiveTrackColor READ get_inactive_track_color WRITE set_inactive_track_color)
    Q_PROPERTY(QColor thumbEdgeColor READ get_thumb_edge_color WRITE set_thumb_edge_color)
public:
    explicit ArcSliderWidget(QWidget *parent = nullptr);

    void set_range(int min, int max);
    int  get_minimum() const { return m_min; }
    int  get_maximum() const { return m_max; }

    void set_value(int value);
    int  get_value() const { return m_value; }

    // Speed mode: the thumb rests at the midpoint and a drag emits a signed speed,
    // growing with distance, in steps of 4. Release returns to 0. Sets the range to
    // +/- max_speed.
    void set_speed_mode(int max_speed);
    bool is_speed_mode() const { return m_speed_mode; }

    // Cosmetic only: color used for the filled portion and thumb fill.
    // Does not affect value mapping or signals.
    void   set_active_color(const QColor &c);
    QColor get_active_color() const { return m_active_color; }
    void   set_inactive_track_color(const QColor &c);
    QColor get_inactive_track_color() const { return m_inactive_track_color; }
    void   set_thumb_edge_color(const QColor &c);
    QColor get_thumb_edge_color() const { return m_thumb_edge_color; }

    // Pure helpers, testable without a widget. y = 0 is the top (maximum).
    static int pixel_to_value(int y, int track_height, int min, int max);
    static int value_to_pixel(int value, int track_height, int min, int max);

signals:
    void value_changed(int value);
    // Speed-mode only: emitted when the quantized signed speed changes.
    // sign > 0 = up-direction, sign < 0 = down-direction, 0 = midpoint/stop.
    void speed_changed(int signed_speed);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int  track_height() const;
    int  track_top()    const;
    void set_value_from_pixel(int local_y);
    int  pixel_to_signed_speed(int local_y) const;
    void apply_speed_from_pixel(int local_y);

    int    m_min;
    int    m_max;
    int    m_value;
    bool   m_pressed;
    bool   m_speed_mode;
    int    m_max_speed;
    int    m_last_signed_speed;
    QColor m_active_color;
    QColor m_inactive_track_color;
    QColor m_thumb_edge_color;
};

#endif // ARCSLIDERWIDGET_H
