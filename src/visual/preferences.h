#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "util/util.h"
#include "util/config.h"

namespace Ui {
class Preferences;
}

class Preferences : public QDialog
{
    Q_OBJECT

public:
    explicit Preferences(QWidget *parent = nullptr);
    ~Preferences();

    void init();
    void set_config(Config* config) { m_config = config; }

    void data_exchange(bool read);
    void config_ip(bool read, int ip = 0, int gateway = 0, int nic_address = 0); // ip and gateway will only be used when reading ip
    void enable_ip_editing(bool enable);
    void set_pixel_format(int idx);
    void update_distance_display();
    void display_baudrate(int id, int baudrate);
    void switch_language(bool en, QTranslator *trans);
    void set_ptz_type_enabled(bool enabled);

protected:
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    // ui
    void jump_to_content(int pos);

    // device
    void update_device_list(int cmd, QStringList dev_list = QStringList()); // 0: switch status, 1: update available devices

    // serial port
    void send_cmd(QString str);

    // TCU
    void toggle_laser(int id, bool on);

signals:
    // device
    void search_for_devices();
    void query_dev_ip();
    void set_dev_ip(int ip, int gateway);
    void change_pixel_format(int format);
    void rotate_image(int angle);
    void device_underwater(bool underwater);

    // serial comm.
    void get_port_info(int com_idx);
    void change_baudrate(int idx, int baudrate);
    void set_tcp_status(int idx, bool use_tcp);
    void share_tcu_port(bool share);
    void com_write(int idx, QByteArray data);

    // tcu config
    void tcu_type_changed(int idx);
    void set_auto_rep_freq(bool _auto);
    void set_auto_mcp(bool _auto);
    void set_ab_lock(bool _auto);
    void rep_freq_unit_changed(int idx);
    void base_unit_changed(int idx);
    void max_dist_changed(float dist);
    void max_dov_changed(float dov);
    void max_laser_changed(float laser_width);
    void delay_offset_changed(float dist);
    void gate_width_offset_changed(float dov);
    void laser_offset_changed(float laser_width);
    void laser_toggled(int config);
    void ps_config_updated(bool read, int idx, uint val);

    // image process
    void lower_3d_thresh_updated();
    void query_tcu_param();

public:
    Ui::Preferences* ui;
    Config*          m_config = nullptr;

    // ui-control
    bool             pressed;
    QPoint           prev_pos;

    // runtime state (not in Config)
    bool             split;
    bool             cameralink;
    int              port_idx;
    bool             use_tcp;
    float            dist_ns;
    QButtonGroup*    laser_grp;

};

#endif // PREFERENCES_H
