#ifndef LASERCONTROLLER_H
#define LASERCONTROLLER_H

#include <QObject>

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
class QDataStream;
QT_END_NAMESPACE

class DeviceManager;
class LensController;
class TCUController;
class Preferences;

class LaserController : public QObject
{
    Q_OBJECT

public:
    explicit LaserController(DeviceManager *device_mgr, LensController *lens_ctrl,
                             TCUController *tcu_ctrl, QObject *parent = nullptr);

    void init(Ui::UserPanel *ui, Preferences *pref);

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

private:
    void set_laser_preset_target(int *pos);

    DeviceManager*  m_device_mgr;
    LensController* m_lens_ctrl;
    TCUController*  m_tcu_ctrl;
    Ui::UserPanel*  m_ui;
    Preferences*    m_pref;

    char            curr_laser_idx;
};

#endif // LASERCONTROLLER_H
