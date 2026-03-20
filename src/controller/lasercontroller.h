#ifndef LASERCONTROLLER_H
#define LASERCONTROLLER_H

#include <QObject>

class DeviceManager;
class LensController;
class TCUController;

class LaserController : public QObject
{
    Q_OBJECT

public:
    explicit LaserController(DeviceManager *device_mgr, LensController *lens_ctrl,
                             TCUController *tcu_ctrl, QObject *parent = nullptr);

    // Getters
    char get_curr_laser_idx() const { return curr_laser_idx; }

    // Setters
    void set_curr_laser_idx(char v) { curr_laser_idx = v; }

public slots:
    void on_LASER_BTN_clicked();
    void on_FIRE_LASER_BTN_clicked();
    void start_laser();
    void init_laser();
    void laser_preset_reached();

    // Laser preset system
    void goto_laser_preset(char target);

signals:
    void send_laser_msg(QString msg);

    // UI state signals
    void laser_btn_update(bool enabled, QString text);
    void fire_btn_update(bool enabled, QString text);
    void current_edit_enabled(bool enabled);
    void simple_laser_chk_update(bool enabled, int state); // state: -1=no change, 0=uncheck, 1=check, 2=click
    void laser_enable_requested(bool enable); // request to check/uncheck pref LASER_ENABLE_CHK
    void update_current_requested(); // request UserPanel to call tcu_ctrl->update_current(value)

private:
    void set_laser_preset_target(int *pos);

    DeviceManager*  m_device_mgr;
    LensController* m_lens_ctrl;
    TCUController*  m_tcu_ctrl;

    bool            laser_on;   // true when laser is active (button shows "OFF")
    bool            firing;     // true when laser is firing (button shows "STOP")
    char            curr_laser_idx;
};

#endif // LASERCONTROLLER_H
