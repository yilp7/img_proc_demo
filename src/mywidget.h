#ifndef MYWIDGET_H
#define MYWIDGET_H

//#include "progsettings.h"
#include "preferences.h"
#include "scanconfig.h"
#include "utils.h"

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

signals:
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
};

class Ruler : public QLabel
{
    Q_OBJECT
public:
    explicit Ruler(QWidget *parent = 0);

private:
    void draw_mark(QPainter *painter);

protected:
    // @override
    void paintEvent(QPaintEvent *event);

public:
    bool   vertical;
    int    interval;
    QLabel *valueLabel;
    QLabel *handleLabel;

};

class InfoLabel : public QLabel
{
    Q_OBJECT
public:
    explicit InfoLabel(QWidget *parent);

};

class TitleButton : public QPushButton
{
    Q_OBJECT
public:
    explicit TitleButton(QString icon, QWidget *parent);

protected:
    void mouseMoveEvent(QMouseEvent *event);

};

class TitleBar : public QFrame
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = 0);
    void setup(QObject *ptr);

public slots:
    void process_maximize();

protected:
    // @override
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

public:
    InfoLabel*   icon;
    InfoLabel*   title;
    TitleButton* url;
    TitleButton* settings;
    TitleButton* capture;
    TitleButton* cls;
    TitleButton* lang;
    TitleButton* theme;
    TitleButton* min;
    TitleButton* max;
    TitleButton* exit;

    bool         is_maximized;
    QPoint       prev_pos;
    QRect        normal_stat;

    bool         pressed;

    QObject      *signal_receiver;

//    ProgSettings *prog_settings;
    Preferences  *preferences;
    ScanConfig   *scan_config;

};

class AnimationLabel : public QLabel
{
    Q_OBJECT
public:
    explicit AnimationLabel(QWidget *parent = 0, QString path = "", QSize size = QSize(), int interval = 100);
    void setup_animation(QString path, QSize size, int interval);

private slots:
    void draw();

private:
    QPixmap source;
    QSize   partition_size;
    QTimer  t;
    int     count;

    QPoint  curr_pos;
    int     curr_idx;
};

class Tools : public QRadioButton
{
    Q_OBJECT
public:
    explicit Tools(QWidget *parent = 0);
};

class Coordinate : public QFrame
{
    Q_OBJECT
public:
    explicit Coordinate(QWidget *parent = 0);
    void setup(QString name);

public:
    void display_pos(QPoint p);

public:
    QPoint     pair;

    InfoLabel* set_name;
    InfoLabel* coord_x;
    InfoLabel* coord_y;
};

class IndexLabel : public QLabel
{
    Q_OBJECT
public:
    explicit IndexLabel(QWidget *parent);
    void setup(int idx, int pos_y);

protected:
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void index_label_clicked(int pos_y);

private:
    int idx;
    int pos_y;
};

class StatusIcon : public QFrame
{
    Q_OBJECT
public:
    enum STATUS {
        NONE          = 0,
        NOT_CONNECTED = 1,
        DISCONNECTED  = 2,
        RECONNECTING  = 3,
        CONNECTED     = 4,
    };
    explicit StatusIcon(QWidget *parent);

    void setup(QPixmap img);
    void setup(QString str);

    void update_status(StatusIcon::STATUS status);

signals:
    void set_pixmap(QPixmap status_block_image);
    void change_status(QPixmap status_dot_image);
    void set_text(QString status_block_text);

private:
    const QPixmap get_status_dot(StatusIcon::STATUS status);

    QLabel* status_block;
    QLabel* status_dot;
    STATUS  status;
};

class StatusBar : public QFrame
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent);

public:
    StatusIcon* cam_status;
    StatusIcon* tcu_status;
    StatusIcon* lens_status;
    StatusIcon* laser_status;
    StatusIcon* ptz_status;
    StatusIcon* img_pixel_format;
    StatusIcon* img_pixel_depth;
    StatusIcon* img_resolution;
    StatusIcon* result_cam_fps;
};

#endif // MYWIDGET_H
