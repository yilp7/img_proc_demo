#ifndef UNLOCKBAR_H
#define UNLOCKBAR_H

#include <QColor>
#include <QWidget>

class QMouseEvent;
class QPaintEvent;
class QTimer;

// Striped safety slider. User must drag the thumb across most of the width
// to unlock. Gates the laser-ON transition only — turning the laser OFF is
// the caller's responsibility and bypasses this widget.
class UnlockBar : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor trackBackgroundColor READ trackBackgroundColor WRITE setTrackBackgroundColor)
    Q_PROPERTY(QColor stripeColor READ stripeColor WRITE setStripeColor)
    Q_PROPERTY(QColor trackEdgeColor READ trackEdgeColor WRITE setTrackEdgeColor)
    Q_PROPERTY(QColor thumbLockedColor READ thumbLockedColor WRITE setThumbLockedColor)
    Q_PROPERTY(QColor thumbUnlockedColor READ thumbUnlockedColor WRITE setThumbUnlockedColor)
    Q_PROPERTY(QColor thumbEdgeArmedColor READ thumbEdgeArmedColor WRITE setThumbEdgeArmedColor)
    Q_PROPERTY(QColor thumbEdgePassColor READ thumbEdgePassColor WRITE setThumbEdgePassColor)
public:
    explicit UnlockBar(QWidget *parent = nullptr);

    // Unlock threshold as a fraction of the track width. Default 0.9.
    // Clamped to (0, 1].
    void   setUnlockThreshold(double fraction);
    double unlockThreshold() const { return m_threshold; }

    // Auto-relock timeout in ms. Default 3000. Set to 0 to disable auto-
    // relock (in which case the caller is responsible for calling relock()).
    void setRelockMs(int ms);
    int  relockMs() const { return m_relock_ms; }

    bool isUnlocked() const { return m_unlocked; }

    QColor trackBackgroundColor() const { return m_track_bg_color; }
    QColor stripeColor() const { return m_stripe_color; }
    QColor trackEdgeColor() const { return m_track_edge_color; }
    QColor thumbLockedColor() const { return m_thumb_locked_color; }
    QColor thumbUnlockedColor() const { return m_thumb_unlocked_color; }
    QColor thumbEdgeArmedColor() const { return m_thumb_edge_armed_color; }
    QColor thumbEdgePassColor() const { return m_thumb_edge_pass_color; }

    void setTrackBackgroundColor(const QColor &color);
    void setStripeColor(const QColor &color);
    void setTrackEdgeColor(const QColor &color);
    void setThumbLockedColor(const QColor &color);
    void setThumbUnlockedColor(const QColor &color);
    void setThumbEdgeArmedColor(const QColor &color);
    void setThumbEdgePassColor(const QColor &color);

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
    void onRelockTimeout();

private:
    int  trackLeft()   const;
    int  trackRight()  const;
    int  trackWidth()  const;
    int  thumbWidth()  const;

    void setThumbX(int x);
    void springBack();

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
