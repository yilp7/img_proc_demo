#ifndef PIDTUNINGWINDOW_H
#define PIDTUNINGWINDOW_H

#include <QDialog>

#include "util/config.h"

class Config;
class PtzFollowController;
class QCheckBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QSpinBox;

// Gains plus a live error/output readout. PID tuning is an iterative process
// against real hardware, so the values apply immediately rather than on
// restart, and saving is separate from applying.
class PidTuningWindow : public QDialog
{
    Q_OBJECT

public:
    PidTuningWindow(Config *config, PtzFollowController *controller, QWidget *parent = nullptr);

    void toggleVisible();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void applyToController();
    void saveToConfig();
    void reloadFromConfig();
    void showTelemetry(double errorH, double errorV, double outputH, double outputV, bool active);

private:
    void setupUi();
    void retranslateUi();
    Config::PidFollowSettings settingsFromUi() const;

    Config *m_config = nullptr;
    PtzFollowController *m_controller = nullptr;

    QDoubleSpinBox *m_kp_h = nullptr;
    QDoubleSpinBox *m_ki_h = nullptr;
    QDoubleSpinBox *m_kd_h = nullptr;
    QDoubleSpinBox *m_kp_v = nullptr;
    QDoubleSpinBox *m_ki_v = nullptr;
    QDoubleSpinBox *m_kd_v = nullptr;
    QDoubleSpinBox *m_deadband = nullptr;
    QDoubleSpinBox *m_max_velocity = nullptr;
    QDoubleSpinBox *m_integral_limit = nullptr;
    QDoubleSpinBox *m_derivative_filter = nullptr;
    QSpinBox *m_coast_ms = nullptr;
    QCheckBox *m_compensate = nullptr;

    QGroupBox *m_gains_group = nullptr;
    QGroupBox *m_limits_group = nullptr;
    QGroupBox *m_live_group = nullptr;

    QLabel *m_state_label = nullptr;
    QLabel *m_error_label = nullptr;
    QLabel *m_output_label = nullptr;
    QPushButton *m_apply_button = nullptr;
    QPushButton *m_save_button = nullptr;
    QPushButton *m_reload_button = nullptr;
};

#endif // PIDTUNINGWINDOW_H
