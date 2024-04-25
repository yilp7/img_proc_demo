#ifndef LASERCONTROL_H
#define LASERCONTROL_H

#include "port/lens.h"
#include "util/util.h"

namespace Ui {
class LaserControl;
}

class LaserControl : public QDialog
{
    Q_OBJECT

public:
    explicit LaserControl(QWidget *parent = nullptr, Lens *p_lens = nullptr);
    ~LaserControl();

    void set_theme();

signals:
    void send_lens_msg_addr(qint32 lens, uchar addr, uchar speed);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private slots:
//    void adjustment_btn_pressed(int id);
//    void adjustment_btn_released(int id);

    void btn_control_pressed(int id);
    void btn_control_released(int id);

signals:
    void com_write(int id, QByteArray cmd);

private:
    Ui::LaserControl *ui;

    bool   pressed;
    QPoint prev_pos;

    Lens         *p_lens;
    QButtonGroup *laser_control;
    uchar        speed[4];
};

#endif // LASERCONTROL_H
