#include "controller/lasercontroller.h"
#include "controller/devicemanager.h"
#include "controller/lenscontroller.h"
#include "controller/tcucontroller.h"
#include "visual/preferences.h"
#include "ui_user_panel.h"
#include "ui_preferences.h"

#include <QTimer>

LaserController::LaserController(DeviceManager *device_mgr, LensController *lens_ctrl,
                                 TCUController *tcu_ctrl, QObject *parent)
    : QObject(parent),
      m_device_mgr(device_mgr),
      m_lens_ctrl(lens_ctrl),
      m_tcu_ctrl(tcu_ctrl),
      m_ui(nullptr),
      m_pref(nullptr),
      curr_laser_idx(-1)
{
}

void LaserController::init(Ui::UserPanel *ui, Preferences *pref)
{
    m_ui = ui;
    m_pref = pref;
}

void LaserController::on_LASER_BTN_clicked()
{
#ifdef LVTONG
    if (m_ui->LASER_BTN->text() == tr("ON")) {
        m_ui->LASER_BTN->setEnabled(false);
        uchar buf_laser_on[] = {0x88, 0x08, 0x00, 0x00, 0x00, 0x01, 0x99};
        m_device_mgr->tcu()->emit send_data(QByteArray((char*)buf_laser_on, 7), 0, 0);
        QTimer::singleShot(4000, this, SLOT(start_laser()));
    }
    else {
        if (m_ui->FIRE_LASER_BTN->text() == tr("STOP")) m_ui->FIRE_LASER_BTN->click();
        uchar buf_laser_off[] = {0x88, 0x08, 0x00, 0x00, 0x00, 0x02, 0x99};
        m_device_mgr->tcu()->send_data(QByteArray((char*)buf_laser_off, 7), 0, 0);

        m_ui->LASER_BTN->setText(tr("ON"));
        m_ui->CURRENT_EDIT->setEnabled(false);
        m_ui->FIRE_LASER_BTN->setEnabled(false);
        m_ui->SIMPLE_LASER_CHK->setEnabled(false);
    }
#else
    m_pref->ui->LASER_ENABLE_CHK->click();
    m_ui->LASER_BTN->setText(m_ui->LASER_BTN->text() == tr("ON") ? tr("OFF") : tr("ON"));
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
    m_ui->LASER_BTN->setEnabled(true);
    m_ui->LASER_BTN->setText(tr("OFF"));
    m_ui->CURRENT_EDIT->setEnabled(true);

    emit send_laser_msg("MODE:RMT 1\r");
    qDebug() << QString("MODE:RMT 1\r");

    emit send_laser_msg("ON\r");
    qDebug() << QString("ON\r");

    emit send_laser_msg("QSW:PRF 0\r");
    qDebug() << QString("QSW:PRF 0\r");

    m_tcu_ctrl->update_current();

    m_ui->SIMPLE_LASER_CHK->setEnabled(true);
    m_ui->FIRE_LASER_BTN->setEnabled(true);
    m_ui->SIMPLE_LASER_CHK->click();
}

void LaserController::on_FIRE_LASER_BTN_clicked()
{
    if (m_ui->FIRE_LASER_BTN->text() == tr("FIRE")) {
        emit send_laser_msg("ON\r");
        qDebug() << QString("ON\r");

        if (!m_pref->ui->LASER_ENABLE_CHK->isChecked()) m_pref->ui->LASER_ENABLE_CHK->click();

        m_ui->FIRE_LASER_BTN->setText(tr("STOP"));
        m_ui->SIMPLE_LASER_CHK->setChecked(true);
    } else {
        emit send_laser_msg("OFF\r");
        qDebug() << QString("OFF\r");

        if (m_pref->ui->LASER_ENABLE_CHK->isChecked()) m_pref->ui->LASER_ENABLE_CHK->click();

        m_ui->FIRE_LASER_BTN->setText(tr("FIRE"));
        m_ui->SIMPLE_LASER_CHK->setChecked(false);
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
