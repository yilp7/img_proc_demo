#include "controller/scancontroller.h"
#include "controller/tcucontroller.h"
#include "controller/devicemanager.h"
#include "port/tcu.h"

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <cmath>

ScanController::ScanController(TCUController *tcu_ctrl, DeviceManager *device_mgr,
                               QObject *parent)
    : QObject(parent),
      m_tcu_ctrl(tcu_ctrl),
      m_device_mgr(device_mgr),
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

void ScanController::init(QString *save_location, int *w, int *h)
{
    m_save_location = save_location;
    m_w = w;
    m_h = h;

    connect(this, SIGNAL(update_scan(bool)), SLOT(enable_scan_options(bool)), Qt::QueuedConnection);
}

void ScanController::on_SCAN_BUTTON_clicked()
{
    bool start_scan = !scan;

    if (start_scan) {
        const bool panorama_scan = (m_params.ptz_scan_mode == ScanParams::PTZ_SCAN_PANORAMA);
        if (panorama_scan) update_panorama_fov_from_zoom(m_params);
        scan_3d = cv::Mat::zeros(*m_h, *m_w, CV_64F);
        scan_sum = cv::Mat::zeros(*m_h, *m_w, CV_64F);

        {
            QMutexLocker lk(&m_scan_mutex);
            scan_idx = 0;
            scan_ptz_idx = -1;
            scan_tcu_idx = -1;
            scan_ptz_route = panorama_scan ? get_panorama_route(m_params) : get_ptz_route(m_params);
            if (panorama_scan) {
                scan_tcu_route.clear();
                scan_step = 0;
            } else {
                scan_tcu_route = get_tcu_route(m_params);
                scan_step = m_params.step_size_delay * m_tcu_ctrl->get_dist_ns();
            }
        }

        if (!panorama_scan) {
            m_tcu_ctrl->set_rep_freq(m_params.rep_freq);
            m_tcu_ctrl->set_delay_dist(m_params.starting_delay * m_tcu_ctrl->get_dist_ns());
        }

        QString name = QString(panorama_scan ? "panorama_" : "scan_")
            + QDateTime::currentDateTime().toString("MMdd_hhmmss");
        { QMutexLocker lk(&m_scan_mutex); scan_name = name; }
        if (!QDir(*m_save_location + "/" + name).exists()) {
            QDir().mkdir(*m_save_location + "/" + name);
        }

        if (m_params.capture_scan_ori || m_params.capture_scan_res) {
            QFile params(*m_save_location + "/" + name + "/scan_params");
            params.open(QIODevice::WriteOnly);
            if (panorama_scan) {
                const QString ending = m_params.panorama_use_end_h
                    ? QString::number(m_params.panorama_end_h, 'f', 2) + " deg"
                    : QString("disabled");
                params.write(QString("mode:               panorama 360\n"
                                     "starting H:         %1 deg\n"
                                     "ending H:           %2\n"
                                     "fixed V:            %3 deg\n"
                                     "positions:          %4\n"
                                     "step H:             %5 deg\n"
                                     "wait time:          %6 s\n"
                                     "images per profile: %7\n"
                                     "zoom:               %8 x\n"
                                     "FOV H:              %9 deg\n"
                                     "FOV V:              %10 deg\n"
                                     "fusion:             %11")
                                 .arg(m_params.panorama_start_h, 0, 'f', 2)
                                 .arg(ending)
                                 .arg(m_params.panorama_fixed_v, 0, 'f', 2)
                                 .arg(m_params.panorama_count)
                                 .arg(m_params.panorama_step_h, 0, 'f', 2)
                                 .arg(m_params.panorama_wait_time)
                                 .arg(m_params.num_single_pos)
                                 .arg(m_params.panorama_zoom_level, 0, 'f', 2)
                                 .arg(m_params.panorama_fov_h, 0, 'f', 2)
                                 .arg(m_params.panorama_fov_v, 0, 'f', 2)
                                 .arg(get_panorama_fusion_name(m_params.panorama_fusion_mode))
                                 .toUtf8());
            } else {
                params.write(QString::asprintf("starting delay:     %06d ns\n"
                                               "ending delay:       %06d ns\n"
                                               "frames count:       %06d\n"
                                               "stepping size:      %.2f ns\n"
                                               "repeated frequency: %06d Hz\n"
                                               "laser width:        %06d ns\n"
                                               "gate width:         %06d ns\n"
                                               "MCP:                %04d",
                                               m_params.starting_delay, m_params.ending_delay, m_params.frame_count, m_params.step_size_delay,
                                               (int)(m_tcu_ctrl->get_rep_freq() * 1000), (int)m_tcu_ctrl->get_laser_width(), std::round(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A)), std::round(m_device_mgr->tcu()->get(TCU::MCP))).toUtf8());
                qDebug() << "scan param" << m_device_mgr->tcu()->get(TCU::DELAY_A) << std::round(m_device_mgr->tcu()->get(TCU::DELAY_A));
                qDebug() << "scan param" << m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A) << std::round(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A));
            }
            params.close();
        }

        on_CONTINUE_SCAN_BUTTON_clicked();
    }
    else {
        emit update_scan(false);
        { QMutexLocker lk(&m_scan_mutex); scan = false; }
        emit scan_button_text_changed(tr("Scan"));
    }
}

void ScanController::on_CONTINUE_SCAN_BUTTON_clicked()
{
    { QMutexLocker lk(&m_scan_mutex); scan = true; }
    emit update_scan(true);
    emit scan_button_text_changed(tr("Pause"));
}

void ScanController::on_RESTART_SCAN_BUTTON_clicked()
{
    { QMutexLocker lk(&m_scan_mutex); scan = true; }
    emit update_scan(true);
    m_tcu_ctrl->set_delay_dist(200);

    on_CONTINUE_SCAN_BUTTON_clicked();
}

void ScanController::on_SCAN_CONFIG_BTN_clicked()
{
    // No-op post-S22: scan config is now embedded in the SCAN overlay
    // (always visible when the SCAN tab is active). The SCAN_CONFIG_BTN
    // itself is hidden in userpanel.cpp; this slot only remains to
    // preserve the existing signal connection on the (hidden) button.
}

void ScanController::enable_scan_options(bool show)
{
    emit scan_options_changed(!is_scanning(), false);
}

void ScanController::auto_scan_for_target()
{
    m_tcu_ctrl->set_rep_freq(30);
    m_tcu_ctrl->set_delay_dist(1000);
    { QMutexLocker lk(&m_scan_mutex); scan_stopping_delay = 6400; scan_step = 100; }
    m_tcu_ctrl->set_depth_of_view(150);
}
