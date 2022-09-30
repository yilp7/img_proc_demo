#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "utils.h"

namespace Ui {
class Preferences;
}

class Preferences : public QDialog
{
    Q_OBJECT

public:
    explicit Preferences(QWidget *parent = nullptr);
    ~Preferences();

    void data_exchange(bool read);
    void config_ip(bool read, int ip = 0, int gateway = 0); // ip and gateway will only be used when reading ip
    void enable_ip_editing(bool enable);
    void set_pixel_format(int idx);
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
    void jump_to_content(int pos);
    void send_cmd(QString str);
    void toggle_laser(int id, bool on);

signals:
    // device
    void set_dev_ip(int ip, int gateway);
    void change_pixel_format(int format);

    // serial comm.
    void get_baudrate(int com_idx);
    void change_baudrate(int idx, int baudrate);
    void connect_tcp_btn_clicked();
    void com_write(int idx, QByteArray data);

    // tcu config
    void rep_freq_unit_changed(int idx);
    void base_unit_changed(int idx);
    void max_dist_changed(int num);
    void laser_toggled(int config);

public:
    Ui::Preferences *ui;

    // ui-control
    bool            pressed;
    QPoint          prev_pos;

    // device
    int             symmetry;
    int             pixel_type;
    bool            cameralink;

    // serial comm.
    int             port_idx;
    bool            share_port;
    bool            use_tcp;

    // tcu config
    float           dist_ns;
    bool            auto_rep_freq;
    bool            auto_mcp;
    int             hz_unit;   // 0: kHz, 1:Hz
    int             base_unit; // 0: ns, 1: μs, 2: m
    float           max_dist;
    bool            laser_enable;
    QButtonGroup    *laser_grp;
    int             laser_on; // does not rely on laser_enable

    // img proc
    float           accu_base;
    float           gamma;
    float           low_in;
    float           high_in;
    float           low_out;
    float           high_out;
    float           dehaze_pct;
    float           sky_tolerance;
    int             fast_gf;
    bool            fishnet_recog;
    float           fishnet_thresh;

};

#endif // PREFERENCES_H
