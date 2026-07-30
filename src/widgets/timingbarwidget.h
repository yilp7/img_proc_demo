#ifndef TIMINGBARWIDGET_H
#define TIMINGBARWIDGET_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QtGlobal>

class QMouseEvent;
class QPaintEvent;

// 4 stacked horizontal bars for TCU Normal mode. Each bar represents a
// single value on its own scale, with the bar's start fixed at 0 (the trigger
// origin). Only the right (end) edge is draggable.
//
// Per-bar config: label, unit string, scale (max value), colour. Units are
// cosmetic labels — the widget treats values as unsigned integers.
//
// This widget is dumb: Normal-mode B-follows-A offset rules live in
// TCUController, not here. The widget just accepts whatever values the
// controller writes back via setBarValue().
class TimingBarWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor barBackgroundColor READ barBackgroundColor WRITE setBarBackgroundColor)
    Q_PROPERTY(QColor barTrackBackgroundColor READ barTrackBackgroundColor WRITE setBarTrackBackgroundColor)
    Q_PROPERTY(QColor barTrackBorderColor READ barTrackBorderColor WRITE setBarTrackBorderColor)
    Q_PROPERTY(QColor barLabelColor READ barLabelColor WRITE setBarLabelColor)
    Q_PROPERTY(QColor barValueColor READ barValueColor WRITE setBarValueColor)
    Q_PROPERTY(QColor laserColor READ laserColor WRITE setLaserColor)
    Q_PROPERTY(QColor distanceColor READ distanceColor WRITE setDistanceColor)
    Q_PROPERTY(QColor dovColor READ dovColor WRITE setDovColor)
    Q_PROPERTY(QColor mcpColor READ mcpColor WRITE setMcpColor)
    Q_PROPERTY(int barGradientStartFactor READ barGradientStartFactor WRITE setBarGradientStartFactor)
    Q_PROPERTY(int barGradientEndFactor READ barGradientEndFactor WRITE setBarGradientEndFactor)
public:
    enum BarId {
        LaserWidth = 0,  // ns
        Distance   = 1,  // m
        Dov        = 2,  // m (depth-of-view)
        Mcp        = 3,  // gain (unitless)
        BarCount
    };
    Q_ENUM(BarId)

    explicit TimingBarWidget(QWidget *parent = nullptr);

    void    setBarValue(BarId id, quint64 value);
    quint64 barValue(BarId id) const;

    void    setBarScale(BarId id, quint64 maxValue);
    quint64 barScale(BarId id) const;

    void    setBarLabel(BarId id, const QString &label);
    void    setBarUnit(BarId id,  const QString &unit);
    void    setBarColor(BarId id, const QColor  &color);

    QColor barBackgroundColor() const { return m_bg_color; }
    QColor barTrackBackgroundColor() const { return m_track_bg_color; }
    QColor barTrackBorderColor() const { return m_track_edge_color; }
    QColor barLabelColor() const { return m_label_color; }
    QColor barValueColor() const { return m_value_color; }
    QColor laserColor() const { return m_bars[LaserWidth].color; }
    QColor distanceColor() const { return m_bars[Distance].color; }
    QColor dovColor() const { return m_bars[Dov].color; }
    QColor mcpColor() const { return m_bars[Mcp].color; }
    int barGradientStartFactor() const { return m_gradient_start_factor; }
    int barGradientEndFactor() const { return m_gradient_end_factor; }

    void setBarBackgroundColor(const QColor &color);
    void setBarTrackBackgroundColor(const QColor &color);
    void setBarTrackBorderColor(const QColor &color);
    void setBarLabelColor(const QColor &color);
    void setBarValueColor(const QColor &color);
    void setLaserColor(const QColor &color);
    void setDistanceColor(const QColor &color);
    void setDovColor(const QColor &color);
    void setMcpColor(const QColor &color);
    void setBarGradientStartFactor(int factor);
    void setBarGradientEndFactor(int factor);

    // Pure math helpers — testable without constructing a widget.
    static quint64 pixelToValue(int px, int widthPx, quint64 scaleMax);
    static int     valueToPixel(quint64 value, int widthPx, quint64 scaleMax);

signals:
    void barValueChanged(TimingBarWidget::BarId id, quint64 value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct Bar {
        quint64 value;
        quint64 scale;
        QString label;
        QString unit;
        QColor  color;
    };

    // Hit-test: is this point over the right edge of any bar?
    int  endHandleAt(const QPoint &pos) const;  // returns BarId or -1

    int  rowHeight() const;
    int  trackLeft() const;
    int  trackWidth() const;
    int  rowTop(int idx) const;

    Bar m_bars[BarCount];
    int m_drag_bar;   // -1 when not dragging
    QColor m_bg_color;
    QColor m_track_bg_color;
    QColor m_track_edge_color;
    QColor m_label_color;
    QColor m_value_color;
    int m_gradient_start_factor;
    int m_gradient_end_factor;
};

#endif // TIMINGBARWIDGET_H
