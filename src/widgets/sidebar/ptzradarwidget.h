#ifndef PTZRADARWIDGET_H
#define PTZRADARWIDGET_H

#include <QFrame>

class PtzRadarWidget : public QFrame
{
    Q_OBJECT
public:
    explicit PtzRadarWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void setAngle(float h, float v = 0.0f);
    void setZeroAngle(float h);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static float normalizeAngle(float angle);

    float m_angle_h;
    float m_angle_v;
    float m_zero_angle_h;
};

#endif // PTZRADARWIDGET_H
