#include "controller/lasercontroller.h"
#include "controller/devicemanager.h"
#include "controller/lenscontroller.h"
#include "controller/tcucontroller.h"

#include <QTimer>

LaserController::LaserController(DeviceManager *device_mgr, LensController *lens_ctrl,
                                 TCUController *tcu_ctrl, QObject *parent)
    : QObject(parent),
      m_device_mgr(device_mgr),
      m_lens_ctrl(lens_ctrl),
      m_tcu_ctrl(tcu_ctrl),
      laser_on(false),
      firing(false),
      curr_laser_idx(-1)
{
}

void LaserController::on_LASER_BTN_clicked()
{
#ifdef LVTONG
    if (!laser_on) {
        emit laser_btn_update(false, "");
        uchar buf_laser_on[] = {0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99};
        m_device_mgr->tcu()->emit send_data(QByteArray((char*)buf_laser_on, 7), 0, 0);
        QTimer::singleShot(4000, this, SLOT(start_laser()));
    }
    else {
        if (firing) on_FIRE_LASER_BTN_clicked();
        uchar buf_laser_off[] = {0x88, 0x08, 0x00, 0x00, 0x00, 0x02, 0x99};
        m_device_mgr->tcu()->send_data(QByteArray((char*)buf_laser_off, 7), 0, 0);

        laser_on = false;
        emit laser_btn_update(true, tr("ON"));
        emit current_edit_enabled(false);
        emit fire_btn_update(false, "");
        emit simple_laser_chk_update(false, -1);
    }
#else
    emit laser_enable_requested(!laser_on);
    laser_on = !laser_on;
    emit laser_btn_update(true, laser_on ? tr("OFF") : tr("ON"));
#endif
}

void LaserController::start_laser()
{
    uchar buf_laser_on[] = {0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99};
    m_device_mgr->tcu()->emit send_data(QByteArray((char*)buf_laser_on, 7), 0, 0);

    QTimer::singleShot(1000, this, SLOT(init_laser()));
}

void LaserController::init_laser()
{
    laser_on = true;
    emit laser_btn_update(true, tr("OFF"));
    emit current_edit_enabled(true);

    emit send_laser_msg("MODE:RMT 1\r");
    qDebug() << QString("MODE:RMT 1\r");

    emit send_laser_msg("ON\r");
    qDebug() << QString("ON\r");

    emit send_laser_msg("QSW:PRF 0\r");
    qDebug() << QString("QSW:PRF 0\r");

    emit update_current_requested();

    emit simple_laser_chk_update(true, 2); // 2 = click
    emit fire_btn_update(true, "");
}

void LaserController::on_FIRE_LASER_BTN_clicked()
{
    if (!firing) {
        emit send_laser_msg("ON\r");
        qDebug() << QString("ON\r");

        emit laser_enable_requested(true);

        firing = true;
        emit fire_btn_update(true, tr("STOP"));
        emit simple_laser_chk_update(true, 1); // 1 = checked
    } else {
        emit send_laser_msg("OFF\r");
        qDebug() << QString("OFF\r");

        emit laser_enable_requested(false);

        firing = false;
        emit fire_btn_update(true, tr("FIRE"));
        emit simple_laser_chk_update(true, 0); // 0 = unchecked
    }
}

void LaserController::laser_preset_reached()
{
    m_lens_ctrl->lens_stop();
    curr_laser_idx = -curr_laser_idx;
}

inline void LaserController::set_laser_preset_target(int *pos)
{
    delete[] pos;
}

void LaserController::goto_laser_preset(char target)
{
    // check sum = sum(byte1 to byte5) & 0xFF
    if (curr_laser_idx < 0) {
        m_lens_ctrl->lens_stop();
        set_laser_preset_target(new int[4]{0});
        target = 1;
        goto quick_break;
    }
    if (target > curr_laser_idx) {
        switch(curr_laser_idx | target) {
        case 0x3:
            set_laser_preset_target(new int[4]{1008, 1008, 1008, 1008});
            goto quick_break;
        case 0x5:
            set_laser_preset_target(new int[4]{2016, 2016, 2016, 2016});
            goto quick_break;
        case 0x9:
            set_laser_preset_target(new int[4]{3024, 3024, 3024, 3024});
            goto quick_break;
        case 0x6:
            set_laser_preset_target(new int[4]{2016, 2016, 2016, 2016});
            goto quick_break;
        case 0xA:
            set_laser_preset_target(new int[4]{3024, 3024, 3024, 3024});
            goto quick_break;
        case 0xC:
            set_laser_preset_target(new int[4]{3024, 3024, 3024, 3024});
            goto quick_break;
        default: return;
        }
    }
    else {
        switch(curr_laser_idx | target) {
        case 0x3:
            set_laser_preset_target(new int[4]{0000, 0000, 0000, 0000});
            goto quick_break;
        case 0x5:
            set_laser_preset_target(new int[4]{0000, 0000, 0000, 0000});
            goto quick_break;
        case 0x9:
            set_laser_preset_target(new int[4]{0000, 0000, 0000, 0000});
            goto quick_break;
        case 0x6:
            set_laser_preset_target(new int[4]{1008, 1008, 1008, 1008});
            goto quick_break;
        case 0xA:
            set_laser_preset_target(new int[4]{1008, 1008, 1008, 1008});
            goto quick_break;
        case 0xC:
            set_laser_preset_target(new int[4]{2016, 2016, 2016, 2016});
            goto quick_break;
        default: return;
        }
    }
quick_break:
    curr_laser_idx = -target;
    QTimer::singleShot(3000, this, SLOT(laser_preset_reached()));
}
