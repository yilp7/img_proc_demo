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
};

#endif // TIMINGBARWIDGET_H
