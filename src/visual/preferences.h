#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "util/util.h"

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

    void data_exchange(bool read);
    void config_ip(bool read, int ip = 0, int gateway = 0, int nic_address = 0); // ip and gateway will only be used when reading ip
    void enable_ip_editing(bool enable);
    void set_pixel_format(int idx);
    void update_distance_display();
    void display_baudrate(int id, int baudrate);
    void switch_language(bool en, QTranslator *trans);

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

    // ui-control
    bool             pressed;
    QPoint           prev_pos;

    // 1 device
    int              device_idx;
    int              symmetry;
    int              pixel_type;
    bool             ebus_cam;
    bool             cameralink;
    bool             split;

    // 2 serial comm.
    int              port_idx;
    bool             share_port;
    bool             use_tcp;

    // 3 tcu config
    float            dist_ns;
    bool             auto_rep_freq;
    bool             auto_mcp;
    int              hz_unit;   // 0: kHz, 1:Hz
    int              base_unit; // 0: ns, 1: μs, 2: m
    float            max_dist;
    float            delay_offset; // distance offset (from hardware)
    float            max_dov;
    float            gate_width_offset; // depth offset (from hardware)
    float            max_laser_width;
    float            laser_width_offset; // laser width offset (from hardware)
    uint             ps_step[4]; // ps-stepping in ps-TCU
    bool             laser_enable;
    QButtonGroup*    laser_grp;
    int              laser_on; // does not rely on laser_enable

    // 4 save options
    bool             save_info;
    bool             custom_topleft_info;
    bool             save_in_grayscale;
    bool             consecutive_capture;
    int              img_format; // 0: bmp/tiff, 1: jpg

    // 5 img proc
    float            accu_base;
    float            gamma;
    float            low_in;
    float            high_in;
    float            low_out;
    float            high_out;
    float            dehaze_pct;
    float            sky_tolerance;
    int              fast_gf;
    int              colormap;
    double           lower_3d_thresh;
    double           upper_3d_thresh;
    bool             truncate_3d;
    bool             custom_3d_param;
    float            custom_3d_delay;
    float            custom_3d_gate_width;
    int              model_idx;
    bool             fishnet_recog;
    float            fishnet_thresh;

};

#endif // PREFERENCES_H
