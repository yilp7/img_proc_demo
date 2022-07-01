#ifndef MYWIDGET_H
#define MYWIDGET_H

#include "progsettings.h"
#include "utils.h"

class Display : public QLabel
{
    Q_OBJECT
public:
    explicit Display(QWidget *parent = 0);

public:
    void update_roi(QPoint pos);

protected:
    // @override
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

signals:
    void curr_pos(QPoint pos);
    void start_pos(QPoint pos);
    void shape_size(QPoint size);

public:
    QPoint    lefttop;
    QPoint    center;
    QPoint    prev_pos; // start position of mouse when image is dragged
    QPoint    ori_pos;  // start position of roi when image is dragged
    bool      grab;
    bool      drag;
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
    InfoLabel    *icon;
    InfoLabel    *title;
    TitleButton  *settings;
    TitleButton  *capture;
    TitleButton  *lang;
    TitleButton  *cls;
    TitleButton  *min;
    TitleButton  *max;
    TitleButton  *exit;

    bool         is_maximized;
    QPoint       prev_pos;
    QRect        normal_stat;

    bool         pressed;

    QObject      *signal_receiver;

    ProgSettings *prog_settings;

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

public slots:
    void display_pos(QPoint p);

public:
    InfoLabel *set_name;
    InfoLabel *coord_x;
    InfoLabel *coord_y;
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

#endif // MYWIDGET_H
