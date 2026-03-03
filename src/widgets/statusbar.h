#ifndef STATUSBAR_H
#define STATUSBAR_H

#include "util/util.h"

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
    StatusIcon* img_pixel_depth;
    StatusIcon* img_resolution;
    StatusIcon* packet_lost;
    StatusIcon* result_cam_fps;
};

#endif // STATUSBAR_H
