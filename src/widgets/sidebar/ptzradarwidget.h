#ifndef PTZRADARWIDGET_H
#define PTZRADARWIDGET_H

#include <QColor>
#include <QFrame>

class QPushButton;
class QTimer;

class PtzRadarWidget : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QColor radarBackgroundColor READ get_radar_background_color WRITE set_radar_background_color)
    Q_PROPERTY(QColor radarBorderColor READ get_radar_border_color WRITE set_radar_border_color)
    Q_PROPERTY(QColor radarGridColor READ get_radar_grid_color WRITE set_radar_grid_color)
    Q_PROPERTY(QColor radarGridSoftColor READ get_radar_grid_soft_color WRITE set_radar_grid_soft_color)
    Q_PROPERTY(QColor radarTextColor READ get_radar_text_color WRITE set_radar_text_color)
    Q_PROPERTY(QColor radarMutedColor READ get_radar_muted_color WRITE set_radar_muted_color)
    Q_PROPERTY(QColor radarAccentColor READ get_radar_accent_color WRITE set_radar_accent_color)
public:
    explicit PtzRadarWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    QColor get_radar_background_color() const { return m_bg_color; }
    QColor get_radar_border_color() const { return m_border_color; }
    QColor get_radar_grid_color() const { return m_grid_color; }
    QColor get_radar_grid_soft_color() const { return m_grid_soft_color; }
    QColor get_radar_text_color() const { return m_text_color; }
    QColor get_radar_muted_color() const { return m_muted_color; }
    QColor get_radar_accent_color() const { return m_accent_color; }

    void set_radar_background_color(const QColor &color);
    void set_radar_border_color(const QColor &color);
    void set_radar_grid_color(const QColor &color);
    void set_radar_grid_soft_color(const QColor &color);
    void set_radar_text_color(const QColor &color);
    void set_radar_muted_color(const QColor &color);
    void set_radar_accent_color(const QColor &color);

    bool is_north_referenced() const;

public slots:
    void set_angle(float h, float v = 0.0f);
    void set_zero_angle(float h);
    // Heading reference, clockwise from true north, from whichever source is
    // feeding it. Only consulted while north reference is on; ptz_zero_angle_h
    // keeps applying either way. A reference that stops arriving expires.
    void set_heading(float heading_deg, bool valid);
    void set_north_referenced(bool enabled);

signals:
    void north_reference_changed(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    static float normalize_angle(float angle);
    float displayed_angle() const;

    float m_angle_h;
    float m_angle_v;
    float m_zero_angle_h;
    float m_heading = 0.0f;
    bool  m_heading_valid = false;
    QTimer *m_heading_timeout = nullptr;
    QPushButton *m_north_button = nullptr;
    QColor m_bg_color;
    QColor m_border_color;
    QColor m_grid_color;
    QColor m_grid_soft_color;
    QColor m_text_color;
    QColor m_muted_color;
    QColor m_accent_color;
};

#endif // PTZRADARWIDGET_H
