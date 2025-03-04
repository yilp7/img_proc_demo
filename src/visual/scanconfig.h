#ifndef SCANCONFIG_H
#define SCANCONFIG_H

#include "util/util.h"

namespace Ui {
class ScanConfig;
}

class ScanConfig : public QDialog
{
    Q_OBJECT

public:
    explicit ScanConfig(QWidget *parent = nullptr);
    ~ScanConfig();

    void data_exchange(bool read);

    std::vector<std::pair<float, float>> get_ptz_route();
    std::vector<float> get_tcu_route();

public slots:
    void update_scan_tcu();
    void update_scan_ptz();
    void setup_hz(int idx);
    void setup_unit(int idx);

protected:
    void keyPressEvent(QKeyEvent *event);
    void showEvent(QShowEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    Ui::ScanConfig *ui;

    // ui-control
    bool            pressed;
    QPoint          prev_pos;

    // scan params
    float          dist_ns;
    int            hz_unit;// 0: kHz, 1: Hz
    int            base_unit;// 0: ns, 1: μs, 2: m
    int            starting_delay;
    int            ending_delay;
    int            starting_gw;
    int            ending_gw;
    float          step_size_delay;
    float          step_size_gw;
    int            frame_count;
    float          rep_freq;
    bool           capture_scan_ori;
    bool           capture_scan_res;
    // TODO implement record function in main
    bool           record_scan_ori;
    bool           record_scan_res;
    float          starting_h;
    float          ending_h;
    float          starting_v;
    float          ending_v;
    float          step_h;
    float          step_v;
    int            count_h;
    int            count_v;
    uint           ptz_direction;
    int            ptz_wait_time;
    int            num_single_pos;
};

#endif // SCANCONFIG_H
