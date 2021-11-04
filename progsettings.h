#ifndef PROGSETTINGS_H
#define PROGSETTINGS_H

#include <QDialog>
#include <QKeyEvent>

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

private slots:
    void update_scan();

    void on_SIMPLIFY_STEP_CHK_stateChanged(int arg1);
    void on_AUTO_REP_FREQ_CHK_stateChanged(int arg1);

    void on_SAVE_SCAN_CHK_stateChanged(int arg1);

    void on_MAX_DIST_EDT_editingFinished();

signals:
    void simplify_step_chk_clicked(bool);
    void max_dist_changed(int);

public:
    Ui::ProgSettings *ui;

    bool   pressed;
    QPoint prev_pos;

    int    start_pos;
    int    end_pos;
    int    frame_count;
    float  step_size;
    float  rep_freq;
    bool   save_scan;

    int    kernel;
    float  gamma;
    float  log;
    float  low_in;
    float  high_in;
    float  low_out;
    float  high_out;

    bool   auto_rep_freq;
    bool   simplify_step;
    int    max_dist;

};

#endif // PROGSETTINGS_H
