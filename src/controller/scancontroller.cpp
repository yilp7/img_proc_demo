#include "controller/scancontroller.h"
#include "controller/tcucontroller.h"
#include "controller/devicemanager.h"
#include "visual/scanconfig.h"
#include "ui_user_panel.h"
#include "port/tcu.h"

#include <QDir>
#include <QFile>
#include <QDateTime>

ScanController::ScanController(TCUController *tcu_ctrl, DeviceManager *device_mgr,
                               QObject *parent)
    : QObject(parent),
      m_tcu_ctrl(tcu_ctrl),
      m_device_mgr(device_mgr),
      m_ui(nullptr),
      m_scan_config(nullptr),
      m_save_location(nullptr),
      m_w(nullptr),
      m_h(nullptr),
      auto_scan_mode(true),
      scan(false),
      scan_distance(200),
      scan_step(0),
      scan_stopping_delay(0),
      scan_idx(0),
      scan_ptz_idx(0),
      scan_tcu_idx(0)
{
}

void ScanController::init(Ui::UserPanel *ui, ScanConfig *scan_config,
                           QString *save_location, int *w, int *h)
{
    m_ui = ui;
    m_scan_config = scan_config;
    m_save_location = save_location;
    m_w = w;
    m_h = h;

    connect(this, SIGNAL(update_scan(bool)), SLOT(enable_scan_options(bool)), Qt::QueuedConnection);
}

void ScanController::on_SCAN_BUTTON_clicked()
{
    bool start_scan = m_ui->SCAN_BUTTON->text() == tr("Scan");

    if (start_scan) {
        scan_3d = cv::Mat::zeros(*m_h, *m_w, CV_64F);
        scan_sum = cv::Mat::zeros(*m_h, *m_w, CV_64F);
        scan_idx = 0;

        scan_ptz_idx = -1;
        scan_tcu_idx = -1;
        scan_ptz_route = m_scan_config->get_ptz_route();
        scan_tcu_route = m_scan_config->get_tcu_route();

        m_tcu_ctrl->set_rep_freq(m_scan_config->rep_freq);
        m_tcu_ctrl->set_delay_dist(m_scan_config->starting_delay * m_tcu_ctrl->get_dist_ns());
        scan_step = m_scan_config->step_size_delay * m_tcu_ctrl->get_dist_ns();

        if (m_scan_config->capture_scan_ori || m_scan_config->capture_scan_res) {
            scan_name = "scan_" + QDateTime::currentDateTime().toString("MMdd_hhmmss");
            if (!QDir(*m_save_location + "/" + scan_name).exists()) {
                QDir().mkdir(*m_save_location + "/" + scan_name);
            }

            QFile params(*m_save_location + "/" + scan_name + "/scan_params");
            params.open(QIODevice::WriteOnly);
            params.write(QString::asprintf("starting delay:     %06d ns\n"
                                           "ending delay:       %06d ns\n"
                                           "frames count:       %06d\n"
                                           "stepping size:      %.2f ns\n"
                                           "repeated frequency: %06d Hz\n"
                                           "laser width:        %06d ns\n"
                                           "gate width:         %06d ns\n"
                                           "MCP:                %04d",
                                           m_scan_config->starting_delay, m_scan_config->ending_delay, m_scan_config->frame_count, m_scan_config->step_size_delay,
                                           (int)(m_tcu_ctrl->get_rep_freq() * 1000), (int)m_tcu_ctrl->get_laser_width(), std::round(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A)), std::round(m_device_mgr->tcu()->get(TCU::MCP))).toUtf8());
            qDebug() << "scan param" << m_device_mgr->tcu()->get(TCU::DELAY_A) << std::round(m_device_mgr->tcu()->get(TCU::DELAY_A));
            qDebug() << "scan param" << m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A) << std::round(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A));
            params.close();
        }

        on_CONTINUE_SCAN_BUTTON_clicked();
    }
    else {
        emit update_scan(false);
        scan = false;
        m_ui->SCAN_BUTTON->setText(tr("Scan"));
    }
}

void ScanController::on_CONTINUE_SCAN_BUTTON_clicked()
{
    scan = true;
    emit update_scan(true);
    m_ui->SCAN_BUTTON->setText(tr("Pause"));
}

void ScanController::on_RESTART_SCAN_BUTTON_clicked()
{
    scan = true;
    emit update_scan(true);
    m_tcu_ctrl->set_delay_dist(200);

    on_CONTINUE_SCAN_BUTTON_clicked();
}

void ScanController::on_SCAN_CONFIG_BTN_clicked()
{
    if (m_scan_config->isVisible()) {
        m_scan_config->hide();
    }
    else {
        m_scan_config->show();
        m_scan_config->raise();
    }
}

void ScanController::enable_scan_options(bool show)
{
    m_ui->CONTINUE_SCAN_BUTTON->setEnabled(!scan);
    m_ui->RESTART_SCAN_BUTTON->setEnabled(false);
}

void ScanController::auto_scan_for_target()
{
    m_tcu_ctrl->set_rep_freq(30);
    m_tcu_ctrl->set_delay_dist(1000);
    scan_stopping_delay = 6400;
    scan_step = 100;
    m_tcu_ctrl->set_depth_of_view(150);
}
