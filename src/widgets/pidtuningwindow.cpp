#include "widgets/pidtuningwindow.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "controller/ptzfollowcontroller.h"
#include "util/config.h"

namespace {

// Fixed width so the form's fields line up in a column instead of each
// stretching to the dialog width.
constexpr int FieldWidth = 96;

QDoubleSpinBox *makeSpin(double min, double max, double step, int decimals)
{
    auto *spin = new QDoubleSpinBox();
    spin->setRange(min, max);
    spin->setSingleStep(step);
    spin->setDecimals(decimals);
    spin->setFixedWidth(FieldWidth);
    return spin;
}

} // namespace

PidTuningWindow::PidTuningWindow(Config *config, PtzFollowController *controller, QWidget *parent)
    : QDialog(parent),
      m_config(config),
      m_controller(controller)
{
    setObjectName(QStringLiteral("PID_TUNING_WINDOW"));
    setModal(false);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(460, 560);

    setupUi();
    retranslateUi();
    reloadFromConfig();

    if (m_controller) {
        connect(m_controller, &PtzFollowController::telemetry,
                this, &PidTuningWindow::showTelemetry);
    }
}

void PidTuningWindow::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    m_gains_group = new QGroupBox(this);
    auto *gains = m_gains_group;
    auto *gains_form = new QFormLayout(gains);
    gains_form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    m_kp_h = makeSpin(0.0, 20.0, 0.05, 3);
    m_ki_h = makeSpin(0.0, 20.0, 0.01, 3);
    m_kd_h = makeSpin(0.0, 20.0, 0.01, 3);
    m_kp_v = makeSpin(0.0, 20.0, 0.05, 3);
    m_ki_v = makeSpin(0.0, 20.0, 0.01, 3);
    m_kd_v = makeSpin(0.0, 20.0, 0.01, 3);
    gains_form->addRow(tr("Kp horizontal"), m_kp_h);
    gains_form->addRow(tr("Ki horizontal"), m_ki_h);
    gains_form->addRow(tr("Kd horizontal"), m_kd_h);
    gains_form->addRow(tr("Kp vertical"), m_kp_v);
    gains_form->addRow(tr("Ki vertical"), m_ki_v);
    gains_form->addRow(tr("Kd vertical"), m_kd_v);
    root->addWidget(gains);

    m_limits_group = new QGroupBox(this);
    auto *limits = m_limits_group;
    auto *limits_form = new QFormLayout(limits);
    limits_form->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    m_deadband = makeSpin(0.0, 5.0, 0.05, 3);
    // 60 deg/s is the protocol's manual-search ceiling.
    m_max_velocity = makeSpin(0.1, 60.0, 1.0, 2);
    m_integral_limit = makeSpin(0.0, 60.0, 0.5, 2);
    m_derivative_filter = makeSpin(0.0, 1.0, 0.05, 2);
    m_coast_ms = new QSpinBox();
    m_coast_ms->setRange(0, 5000);
    m_coast_ms->setSingleStep(50);
    m_coast_ms->setFixedWidth(FieldWidth);
    m_compensate = new QCheckBox();
    limits_form->addRow(tr("Deadband (deg)"), m_deadband);
    limits_form->addRow(tr("Max velocity (deg/s)"), m_max_velocity);
    limits_form->addRow(tr("Integral limit (deg/s)"), m_integral_limit);
    limits_form->addRow(tr("Derivative filter"), m_derivative_filter);
    limits_form->addRow(tr("Coast on loss (ms)"), m_coast_ms);
    limits_form->addRow(tr("Compensate turret motion"), m_compensate);
    root->addWidget(limits);

    m_live_group = new QGroupBox(this);
    auto *live = m_live_group;
    auto *live_form = new QFormLayout(live);
    m_state_label = new QLabel(this);
    m_error_label = new QLabel(this);
    m_output_label = new QLabel(this);
    live_form->addRow(tr("State"), m_state_label);
    live_form->addRow(tr("Error (deg)"), m_error_label);
    live_form->addRow(tr("Output (deg/s)"), m_output_label);
    root->addWidget(live);

    root->addStretch(1);

    auto *buttons = new QHBoxLayout();
    m_reload_button = new QPushButton(this);
    m_apply_button = new QPushButton(this);
    m_save_button = new QPushButton(this);
    buttons->addStretch(1);
    buttons->addWidget(m_reload_button);
    buttons->addWidget(m_apply_button);
    buttons->addWidget(m_save_button);
    root->addLayout(buttons);

    connect(m_reload_button, &QPushButton::clicked, this, &PidTuningWindow::reloadFromConfig);
    connect(m_apply_button, &QPushButton::clicked, this, &PidTuningWindow::applyToController);
    connect(m_save_button, &QPushButton::clicked, this, &PidTuningWindow::saveToConfig);

    // Live apply: tuning means watching the response change as a value moves,
    // and a mandatory Apply click between every nudge makes that unusable.
    const QList<QDoubleSpinBox*> spins = {m_kp_h, m_ki_h, m_kd_h, m_kp_v, m_ki_v, m_kd_v,
                                          m_deadband, m_max_velocity, m_integral_limit,
                                          m_derivative_filter};
    for (QDoubleSpinBox *spin : spins) {
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this](double) { applyToController(); });
    }
    connect(m_coast_ms, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { applyToController(); });
    connect(m_compensate, &QCheckBox::toggled, this, [this](bool) { applyToController(); });
}

void PidTuningWindow::retranslateUi()
{
    setWindowTitle(tr("PTZ Follow Tuning"));
    if (m_gains_group) m_gains_group->setTitle(tr("Gains"));
    if (m_limits_group) m_limits_group->setTitle(tr("Limits and loss handling"));
    if (m_live_group) m_live_group->setTitle(tr("Live"));
    if (m_reload_button) {
        m_reload_button->setText(tr("Reload"));
    }
    if (m_apply_button) m_apply_button->setText(tr("Apply"));
    if (m_save_button) m_save_button->setText(tr("Save"));
    if (m_compensate) m_compensate->setToolTip(
                tr("Subtracts the turret's own commanded motion from the measured error, so "
                   "the loop closes on target motion rather than its own lag."));
}

void PidTuningWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) retranslateUi();
    QDialog::changeEvent(event);
}

void PidTuningWindow::toggleVisible()
{
    if (isVisible()) {
        hide();
        return;
    }
    reloadFromConfig();
    show();
    raise();
    activateWindow();
}

Config::PidFollowSettings PidTuningWindow::settingsFromUi() const
{
    Config::PidFollowSettings settings;
    settings.kp_h = m_kp_h->value();
    settings.ki_h = m_ki_h->value();
    settings.kd_h = m_kd_h->value();
    settings.kp_v = m_kp_v->value();
    settings.ki_v = m_ki_v->value();
    settings.kd_v = m_kd_v->value();
    settings.deadband_deg = m_deadband->value();
    settings.max_velocity_deg_s = m_max_velocity->value();
    settings.integral_limit_deg_s = m_integral_limit->value();
    settings.derivative_filter = m_derivative_filter->value();
    settings.coast_ms = m_coast_ms->value();
    settings.compensate_turret_motion = m_compensate->isChecked();
    return settings;
}

void PidTuningWindow::applyToController()
{
    if (m_controller) m_controller->setSettings(settingsFromUi());
}

void PidTuningWindow::saveToConfig()
{
    if (!m_config) return;
    m_config->get_data().pid_follow = settingsFromUi();
    m_config->auto_save();
    applyToController();
}

void PidTuningWindow::reloadFromConfig()
{
    if (!m_config) return;
    const Config::PidFollowSettings &settings = m_config->get_data().pid_follow;

    // Blocked so repopulating does not fire the live-apply handlers once per
    // field on the way through.
    const QList<QWidget*> widgets = {m_kp_h, m_ki_h, m_kd_h, m_kp_v, m_ki_v, m_kd_v,
                                     m_deadband, m_max_velocity, m_integral_limit,
                                     m_derivative_filter, m_coast_ms, m_compensate};
    for (QWidget *w : widgets) w->blockSignals(true);

    m_kp_h->setValue(settings.kp_h);
    m_ki_h->setValue(settings.ki_h);
    m_kd_h->setValue(settings.kd_h);
    m_kp_v->setValue(settings.kp_v);
    m_ki_v->setValue(settings.ki_v);
    m_kd_v->setValue(settings.kd_v);
    m_deadband->setValue(settings.deadband_deg);
    m_max_velocity->setValue(settings.max_velocity_deg_s);
    m_integral_limit->setValue(settings.integral_limit_deg_s);
    m_derivative_filter->setValue(settings.derivative_filter);
    m_coast_ms->setValue(settings.coast_ms);
    m_compensate->setChecked(settings.compensate_turret_motion);

    for (QWidget *w : widgets) w->blockSignals(false);
    applyToController();
}

void PidTuningWindow::showTelemetry(double errorH, double errorV,
                                    double outputH, double outputV, bool active)
{
    if (m_state_label) m_state_label->setText(active ? tr("following") : tr("idle"));
    if (m_error_label) m_error_label->setText(QStringLiteral("H %1    V %2")
                                              .arg(errorH, 0, 'f', 3).arg(errorV, 0, 'f', 3));
    if (m_output_label) m_output_label->setText(QStringLiteral("H %1    V %2")
                                                .arg(outputH, 0, 'f', 2).arg(outputV, 0, 'f', 2));
}
