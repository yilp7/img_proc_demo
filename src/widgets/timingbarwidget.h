#ifndef TIMINGBARWIDGET_H
#define TIMINGBARWIDGET_H

#include <QWidget>
#include <QColor>
#include <QString>
#include <QtGlobal>

class QMouseEvent;
class QPaintEvent;

// Four stacked horizontal bars for the TCU panel's normal mode. Each bar shows one
// value on its own scale from 0; only the right edge can be dragged. Units are
// labels only. The offset rules between bars live in TCUController.
class TimingBarWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor barBackgroundColor READ get_bar_background_color WRITE set_bar_background_color)
    Q_PROPERTY(QColor barTrackBackgroundColor READ get_bar_track_background_color WRITE set_bar_track_background_color)
    Q_PROPERTY(QColor barTrackBorderColor READ get_bar_track_border_color WRITE set_bar_track_border_color)
    Q_PROPERTY(QColor barLabelColor READ get_bar_label_color WRITE set_bar_label_color)
    Q_PROPERTY(QColor barValueColor READ get_bar_value_color WRITE set_bar_value_color)
    Q_PROPERTY(QColor laserColor READ get_laser_color WRITE set_laser_color)
    Q_PROPERTY(QColor distanceColor READ get_distance_color WRITE set_distance_color)
    Q_PROPERTY(QColor dovColor READ get_dov_color WRITE set_dov_color)
    Q_PROPERTY(QColor mcpColor READ get_mcp_color WRITE set_mcp_color)
    Q_PROPERTY(int barGradientStartFactor READ get_bar_gradient_start_factor WRITE set_bar_gradient_start_factor)
    Q_PROPERTY(int barGradientEndFactor READ get_bar_gradient_end_factor WRITE set_bar_gradient_end_factor)
public:
    enum BarId {
        LASER_WIDTH = 0,  // ns
        DISTANCE    = 1,  // m
        DOV         = 2,  // m (depth-of-view)
        MCP         = 3,  // gain (unitless)
        BAR_COUNT
    };
    Q_ENUM(BarId)

    explicit TimingBarWidget(QWidget *parent = nullptr);

    void    set_bar_value(BarId id, quint64 value);
    quint64 get_bar_value(BarId id) const;

    void    set_bar_scale(BarId id, quint64 max_value);
    quint64 get_bar_scale(BarId id) const;

    void    set_bar_label(BarId id, const QString &label);
    void    set_bar_unit(BarId id,  const QString &unit);
    void    set_bar_color(BarId id, const QColor  &color);

    QColor get_bar_background_color() const { return m_bg_color; }
    QColor get_bar_track_background_color() const { return m_track_bg_color; }
    QColor get_bar_track_border_color() const { return m_track_edge_color; }
    QColor get_bar_label_color() const { return m_label_color; }
    QColor get_bar_value_color() const { return m_value_color; }
    QColor get_laser_color() const { return m_bars[LASER_WIDTH].color; }
    QColor get_distance_color() const { return m_bars[DISTANCE].color; }
    QColor get_dov_color() const { return m_bars[DOV].color; }
    QColor get_mcp_color() const { return m_bars[MCP].color; }
    int get_bar_gradient_start_factor() const { return m_gradient_start_factor; }
    int get_bar_gradient_end_factor() const { return m_gradient_end_factor; }

    void set_bar_background_color(const QColor &color);
    void set_bar_track_background_color(const QColor &color);
    void set_bar_track_border_color(const QColor &color);
    void set_bar_label_color(const QColor &color);
    void set_bar_value_color(const QColor &color);
    void set_laser_color(const QColor &color);
    void set_distance_color(const QColor &color);
    void set_dov_color(const QColor &color);
    void set_mcp_color(const QColor &color);
    void set_bar_gradient_start_factor(int factor);
    void set_bar_gradient_end_factor(int factor);

    // Pure helpers, testable without a widget.
    static quint64 pixel_to_value(int px, int width_px, quint64 scale_max);
    static int     value_to_pixel(quint64 value, int width_px, quint64 scale_max);

signals:
    void bar_value_changed(TimingBarWidget::BarId id, quint64 value);

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
    int  end_handle_at(const QPoint &pos) const;  // returns BarId or -1

    int  row_height() const;
    int  track_left() const;
    int  track_width() const;
    int  row_top(int idx) const;

    Bar m_bars[BAR_COUNT];
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
