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
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

signals:
    void set_pixmap(const QPixmap& pm);
    void widget_resized();   // emitted after resize; UserPanel re-snapshots ProcessingParams
    void view_changed();     // emitted after zoom/pan/selection changes
    // idx: 0: curr_pos, 1: start_pos, 2: shape_size, 3: ptz_target
    void updated_pos(int idx, QPoint pos);
    void curr_pos(QPoint pos);
    void start_pos(QPoint pos);
    void shape_size(QPoint size);
    void ptz_target(QPoint pos, bool closed_loop);
    // Alt+drag in PTZ mode: the region to hand the follow tracker, in widget
    // coordinates. Empty when the drag was too small to be a deliberate gate.
    void gate_selected(QRect gate);
    // Plain single click in PTZ mode: drop the current gate.
    void gate_cleared();
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

    // Follow gate: the rubber band while dragging, and the locked gate the
    // tracker is holding. Both drawn over the pixmap by paintEvent.
    void set_follow_gate(const QRect &gate);
    QRect    gate_rubber_band;
    QRect    follow_gate;
    bool     gate_dragging = false;
    QPoint   gate_origin;

    QRect    reserved_geometry;
    bool     is_fullscreen = false;
};

#endif // DISPLAY_H
