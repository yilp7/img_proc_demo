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

protected:
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    bool eventFilter(QObject *obj, QEvent* event);

private slots:
    void update_scan();

    void on_SAVE_SCAN_ORI_CHK_stateChanged(int arg1);
    void on_SAVE_SCAN_RES_CHK_stateChanged(int arg1);

    void on_HZ_LIST_currentIndexChanged(int index);
    void on_UNIT_LIST_currentIndexChanged(int index);
    void on_AUTO_REP_FREQ_CHK_stateChanged(int arg1);

    void on_MAX_DIST_EDT_editingFinished();

    void toggle_laser(int id, bool on);

signals:
    void rep_freq_unit_changed(int);
    void base_unit_changed(int);
    void max_dist_changed(int);
    void laser_toggled(int);

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

    int              kernel;
    float            gamma;
    float            log;
    float            low_in;
    float            high_in;
    float            low_out;
    float            high_out;

    float            dist_ns;
    bool             auto_rep_freq;
    int              hz_unit; // 0: kHz, 1:Hz
    int              base_unit; // 0: ns, 1: μs, 2: m
    float            max_dist;
    QButtonGroup     *laser_grp;
    int              laser_on;

};

#endif // PROGSETTINGS_H
