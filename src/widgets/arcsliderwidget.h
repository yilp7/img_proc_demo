#ifndef ARCSLIDERWIDGET_H
#define ARCSLIDERWIDGET_H

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

    // Pure math helpers — testable without constructing a widget.
    // y=0 is top (=max); y=trackHeight is bottom (=min).
    static int pixelToValue(int y, int trackHeight, int min, int max);
    static int valueToPixel(int value, int trackHeight, int min, int max);

signals:
    void valueChanged(int value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int  trackHeight() const;
    int  trackTop()    const;
    void setValueFromPixel(int local_y);

    int  m_min;
    int  m_max;
    int  m_value;
    bool m_pressed;
};

#endif // ARCSLIDERWIDGET_H
