#ifndef PROGSETTINGS_H
#define PROGSETTINGS_H

#include "utils.h"

namespace Ui {
class ProgSettings;
}

class ProgSettings : public QDialog
{
    Q_OBJECT

public:
    explicit ProgSettings(QWidget *parent = nullptr);
    ~ProgSettings();

    void data_exchange(bool read = false);
    void display_baudrate(int id, int baudrate);
    void enable_ip_editing(bool enable);
    void config_ip(bool read, int ip = 0, int gateway = 0); // ip and gateway will only be used when requesting ip

protected:
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    void showEvent(QShowEvent *event);

private:
    void send_cmd();

private slots:
    void update_scan();

    void on_SAVE_SCAN_ORI_CHK_stateChanged(int arg1);
    void on_SAVE_SCAN_RES_CHK_stateChanged(int arg1);

    void on_HZ_LIST_currentIndexChanged(int index);
    void on_UNIT_LIST_currentIndexChanged(int index);
    void on_AUTO_REP_FREQ_CHK_stateChanged(int arg1);

    void on_MAX_DIST_EDT_editingFinished();

    void toggle_laser(int id, bool on);

    void on_BAUDRATE_LIST_activated(const QString &arg1);
    void on_SHARE_CHK_stateChanged(int arg1);
    void on_COM_LIST_currentIndexChanged(int index);

    void on_AUTO_MCP_CHK_stateChanged(int arg1);

    void on_CAMERALINK_CHK_stateChanged(int arg1);

    void on_FAST_GF_EDIT_editingFinished();
    void on_CENTRAL_SYMM_CHK_stateChanged(int arg1);

    void on_LASER_ENERGY_LIST_currentIndexChanged(int index);

    void on_FILTER_CHK_stateChanged(int arg1);

signals:
    void rep_freq_unit_changed(int);
    void base_unit_changed(int);
    void max_dist_changed(int);
    void laser_toggled(int);
    void share_serial_port(bool share);
    void change_baudrate(int idx, int baudrate);
    void com_write(int, QByteArray);
    void get_baudrate(int);
    void auto_mcp(bool);
    void set_dev_ip(int ip, int gateway);

public:
    Ui::ProgSettings *ui;

    bool             pressed;
    QPoint           prev_pos;

    int              start_pos;
    int              end_pos;
    int              frame_count;
    float            step_size;
    float            rep_freq;
    bool             save_scan_ori;
    bool             save_scan_res;
    bool             filter_scan;

    int              kernel;
    float            gamma;
    float            log;
    float            accu_base;
    float            low_in;
    float            high_in;
    float            low_out;
    float            high_out;
    float            dehaze_pct;
    float            sky_tolerance;
    int              fast_gf;
    bool             central_symmetry;

    float            dist_ns;
    bool             auto_rep_freq;
    int              hz_unit; // 0: kHz, 1:Hz
    int              base_unit; // 0: ns, 1: μs, 2: m
    float            max_dist;
    QButtonGroup     *laser_grp;
    int              laser_on;

    int              com_idx;

    bool             cameralink;

};

#endif // PROGSETTINGS_H
