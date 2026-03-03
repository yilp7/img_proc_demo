#ifndef DISPLAY_H
#define DISPLAY_H

#include "util/util.h"

class Display : public QLabel
{
    Q_OBJECT
public:
    explicit Display(QWidget *parent = 0);

public:
    void update_roi(QPoint pos);

public slots:
    void clear();

protected:
    // @override
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

signals:
    void set_pixmap(const QPixmap& pm);
    // idx: 0: curr_pos, 1: start_pos, 2: shape_size, 3: ptz_target
    void updated_pos(int idx, QPoint pos);
    void curr_pos(QPoint pos);
    void start_pos(QPoint pos);
    void shape_size(QPoint size);
    void ptz_target(QPoint pos);
    void add_roi(cv::Point p1, cv::Point p2);
    void clear_roi();

public:
    QPoint    lefttop;
    QPoint    center;
    QPoint    prev_pos; // start position of mouse when image is dragged
    QPoint    ori_pos;  // start position of roi when image is dragged
    bool      is_grabbing;
    int       mode; // 0: zoom mode, 1: selection mode, 2: ptz mode
    int       curr_scale;
    float     scale[5];
    cv::Rect  display_region;
    cv::Point selection_v1;
    cv::Point selection_v2;

    bool     pressed;

    QRect    reserved_geometry;
};

#endif // DISPLAY_H
