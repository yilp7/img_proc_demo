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

    void apply_color_mode();

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
    Ui::LaserControl *m_ui;

    bool   m_pressed;
    QPoint m_prev_pos;

    Lens         *m_p_lens;
    QButtonGroup *m_laser_control;
    uchar        m_speed[4];
};

#endif // LASERCONTROL_H
