#ifndef PTZRADARWIDGET_H
#define PTZRADARWIDGET_H

#include <QColor>
#include <QFrame>

class QPushButton;
class QTimer;

class PtzRadarWidget : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QColor radarBackgroundColor READ radarBackgroundColor WRITE setRadarBackgroundColor)
    Q_PROPERTY(QColor radarBorderColor READ radarBorderColor WRITE setRadarBorderColor)
    Q_PROPERTY(QColor radarGridColor READ radarGridColor WRITE setRadarGridColor)
    Q_PROPERTY(QColor radarGridSoftColor READ radarGridSoftColor WRITE setRadarGridSoftColor)
    Q_PROPERTY(QColor radarTextColor READ radarTextColor WRITE setRadarTextColor)
    Q_PROPERTY(QColor radarMutedColor READ radarMutedColor WRITE setRadarMutedColor)
    Q_PROPERTY(QColor radarAccentColor READ radarAccentColor WRITE setRadarAccentColor)
public:
    explicit PtzRadarWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    QColor radarBackgroundColor() const { return m_bg_color; }
    QColor radarBorderColor() const { return m_border_color; }
    QColor radarGridColor() const { return m_grid_color; }
    QColor radarGridSoftColor() const { return m_grid_soft_color; }
    QColor radarTextColor() const { return m_text_color; }
    QColor radarMutedColor() const { return m_muted_color; }
    QColor radarAccentColor() const { return m_accent_color; }

    void setRadarBackgroundColor(const QColor &color);
    void setRadarBorderColor(const QColor &color);
    void setRadarGridColor(const QColor &color);
    void setRadarGridSoftColor(const QColor &color);
    void setRadarTextColor(const QColor &color);
    void setRadarMutedColor(const QColor &color);
    void setRadarAccentColor(const QColor &color);

    bool isNorthReferenced() const;

public slots:
    void setAngle(float h, float v = 0.0f);
    void setZeroAngle(float h);
    // Heading reference, clockwise from true north, from whichever source is
    // feeding it. Only consulted while north reference is on; ptz_zero_angle_h
    // keeps applying either way. A reference that stops arriving expires.
    void setHeading(float heading_deg, bool valid);
    void setNorthReferenced(bool enabled);

signals:
    void northReferenceChanged(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    static float normalizeAngle(float angle);
    float displayedAngle() const;

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
