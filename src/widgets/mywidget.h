#ifndef MYWIDGET_H
#define MYWIDGET_H

#include "display.h"
#include "titlebar.h"
#include "statusbar.h"
#include "floatingwindow.h"

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
    //@override
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void index_label_clicked(int pos_y);

private:
    int idx;
    int pos_y;
};

class MiscSelection : public QComboBox
{
    Q_OBJECT
public:
    explicit MiscSelection(QWidget *parent);

protected:
    //@override
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *event);

    // TODO make this popup grow upwards with no animation
//    void showPopup();

signals:
    void selected();
};

#endif // MYWIDGET_H
