#ifndef MYWIDGET_H
#define MYWIDGET_H

#include "display.h"
#include "topbar/topbar.h"
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
    bool   m_vertical;
    int    m_interval;
    QLabel *m_value_label;
    QLabel *m_handle_label;

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
    QPixmap m_source;
    QSize   m_partition_size;
    QTimer  m_t;
    int     m_count;

    QPoint  m_curr_pos;
    int     m_curr_idx;
};

class Tools : public QRadioButton
{
    Q_OBJECT
public:
    explicit Tools(QWidget *parent = 0);
};

class InfoLabel : public QLabel
{
    Q_OBJECT
public:
    explicit InfoLabel(QWidget *parent) : QLabel(parent) {}
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
    QPoint     m_pair;

    InfoLabel *m_set_name;
    InfoLabel *m_coord_x;
    InfoLabel *m_coord_y;
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
    int m_idx;
    int m_pos_y;
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
