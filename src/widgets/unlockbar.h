#ifndef UNLOCKBAR_H
#define UNLOCKBAR_H

#include <QColor>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QTimer;

// Striped safety slider: the thumb has to be dragged across most of the width to
// unlock. It gates switching the laser on only; switching off bypasses it.
class UnlockBar : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor trackBackgroundColor READ get_track_background_color WRITE set_track_background_color)
    Q_PROPERTY(QColor stripeColor READ get_stripe_color WRITE set_stripe_color)
    Q_PROPERTY(QColor trackEdgeColor READ get_track_edge_color WRITE set_track_edge_color)
    Q_PROPERTY(QColor thumbLockedColor READ get_thumb_locked_color WRITE set_thumb_locked_color)
    Q_PROPERTY(QColor thumbUnlockedColor READ get_thumb_unlocked_color WRITE set_thumb_unlocked_color)
    Q_PROPERTY(QColor thumbEdgeArmedColor READ get_thumb_edge_armed_color WRITE set_thumb_edge_armed_color)
    Q_PROPERTY(QColor thumbEdgePassColor READ get_thumb_edge_pass_color WRITE set_thumb_edge_pass_color)
public:
    explicit UnlockBar(QWidget *parent = nullptr);

    // Unlock threshold as a fraction of the track width. Default 0.9.
    // Clamped to (0, 1].
    void   set_unlock_threshold(double fraction);
    double get_unlock_threshold() const { return m_threshold; }

    // Auto-relock timeout in ms. Default 3000. Set to 0 to disable auto-
    // relock (in which case the caller is responsible for calling relock()).
    void set_relock_ms(int ms);
    int  get_relock_ms() const { return m_relock_ms; }

    bool is_unlocked() const { return m_unlocked; }

    QColor get_track_background_color() const { return m_track_bg_color; }
    QColor get_stripe_color() const { return m_stripe_color; }
    QColor get_track_edge_color() const { return m_track_edge_color; }
    QColor get_thumb_locked_color() const { return m_thumb_locked_color; }
    QColor get_thumb_unlocked_color() const { return m_thumb_unlocked_color; }
    QColor get_thumb_edge_armed_color() const { return m_thumb_edge_armed_color; }
    QColor get_thumb_edge_pass_color() const { return m_thumb_edge_pass_color; }

    void set_track_background_color(const QColor &color);
    void set_stripe_color(const QColor &color);
    void set_track_edge_color(const QColor &color);
    void set_thumb_locked_color(const QColor &color);
    void set_thumb_unlocked_color(const QColor &color);
    void set_thumb_edge_armed_color(const QColor &color);
    void set_thumb_edge_pass_color(const QColor &color);

public slots:
    // Force the bar back to the locked state. Safe to call when already
    // locked (no signal). Clears any pending auto-relock timer.
    void relock();

signals:
    void unlocked();
    void locked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_relock_timeout();

private:
    int  track_left()    const;
    int  track_right()   const;
    int  track_width()   const;
    int  thumb_width()   const;

    void set_thumb_x(int x);
    void spring_back();

    double  m_threshold;
    int     m_relock_ms;
    bool    m_unlocked;
    bool    m_pressed;
    int     m_thumb_x;     // left edge of thumb, in widget-local coords
    int     m_press_dx;    // offset from thumb-left to press point
    QTimer *m_relock_timer;
    QColor  m_track_bg_color;
    QColor  m_stripe_color;
    QColor  m_track_edge_color;
    QColor  m_thumb_locked_color;
    QColor  m_thumb_unlocked_color;
    QColor  m_thumb_edge_armed_color;
    QColor  m_thumb_edge_pass_color;
};

#endif // UNLOCKBAR_H
