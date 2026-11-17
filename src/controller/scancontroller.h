#ifndef SCANCONTROLLER_H
#define SCANCONTROLLER_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <opencv2/core.hpp>
#include <vector>
#include <utility>

#include "pipeline/scanparams.h"
#include "pipeline/scanjob.h"

class TCUController;
class DeviceManager;

class ScanController : public QObject
{
    Q_OBJECT

public:
    explicit ScanController(TCUController *tcu_ctrl, DeviceManager *device_mgr,
                            QObject *parent = nullptr);

    void init(QString *save_location, int *w, int *h);

    // Scan parameters, the single source of truth. ScanOverlay writes them from its
    // widgets. The frame path gets them inside the scan job, so handing out the writable
    // reference schedules a new job. GUI thread only.
    const ScanParams &get_params() const { return m_params; }
    ScanParams       &get_params()       { schedule_job(); return m_params; }
    void refresh_current_values();

    // mutex-protected for cross-thread access
    bool is_scanning() const { QMutexLocker lk(&m_scan_mutex); return m_scan; }
    bool is_auto_scan_mode() const { QMutexLocker lk(&m_scan_mutex); return m_auto_scan_mode; }
    float get_scan_distance() const { QMutexLocker lk(&m_scan_mutex); return m_scan_distance; }
    float get_scan_step() const { QMutexLocker lk(&m_scan_mutex); return m_scan_step; }
    float get_scan_stopping_delay() const { QMutexLocker lk(&m_scan_mutex); return m_scan_stopping_delay; }
    QString get_scan_name() const { QMutexLocker lk(&m_scan_mutex); return m_scan_name; }
    cv::Mat &get_scan_3d() { return m_scan_3d; }
    cv::Mat &get_scan_sum() { return m_scan_sum; }
    int get_scan_idx() const { QMutexLocker lk(&m_scan_mutex); return m_scan_idx; }
    std::vector<std::pair<float,float>> get_scan_ptz_route() const { QMutexLocker lk(&m_scan_mutex); return m_scan_ptz_route; }
    std::vector<float> get_scan_tcu_route() const { QMutexLocker lk(&m_scan_mutex); return m_scan_tcu_route; }
    std::vector<float> get_scan_gate_width_route() const { QMutexLocker lk(&m_scan_mutex); return m_scan_gate_width_route; }
    int get_scan_ptz_idx() const { QMutexLocker lk(&m_scan_mutex); return m_scan_ptz_idx; }
    int get_scan_tcu_idx() const { QMutexLocker lk(&m_scan_mutex); return m_scan_tcu_idx; }

    // mutex-protected for cross-thread access
    void set_scanning(bool v) { QMutexLocker lk(&m_scan_mutex); m_scan = v; }
    void set_auto_scan_mode(bool v) { QMutexLocker lk(&m_scan_mutex); m_auto_scan_mode = v; }
    void set_scan_idx(int v) { QMutexLocker lk(&m_scan_mutex); m_scan_idx = v; }
    void set_scan_ptz_idx(int v) { QMutexLocker lk(&m_scan_mutex); m_scan_ptz_idx = v; }
    void set_scan_tcu_idx(int v) { QMutexLocker lk(&m_scan_mutex); m_scan_tcu_idx = v; }
    void set_scan_step(float v) { QMutexLocker lk(&m_scan_mutex); m_scan_step = v; }
    void set_scan_stopping_delay(float v) { QMutexLocker lk(&m_scan_mutex); m_scan_stopping_delay = v; }

public slots:
    void on_SCAN_BUTTON_clicked();
    void on_CONTINUE_SCAN_BUTTON_clicked();
    void on_RESTART_SCAN_BUTTON_clicked();
    void on_SCAN_CONFIG_BTN_clicked();
    void enable_scan_options(bool show);
    void auto_scan_for_target();
    // From the frame thread, queued. A step moves the PTZ and sets delay and gate width;
    // the end of a scan stops it. Both name their scan by its generation.
    void apply_scan_step(const ScanStep &step);
    void end_scan(quint64 generation);

signals:
    void update_scan(bool show);
    void finish_scan();
    void scan_button_text_changed(QString text);
    void scan_options_changed(bool continue_enabled, bool restart_enabled);
    // The job has changed: its state, its routes or a parameter. UserPanel keeps the copy
    // the frame path reads.
    void scan_job_changed(const ScanJob &job);

private:
    void schedule_job();
    void publish_job();

    mutable QMutex m_scan_mutex;

    quint64        m_job_revision  = 0;
    quint64        m_generation    = 0;
    bool           m_job_scheduled = false;

    ScanParams     m_params;
    TCUController *m_tcu_ctrl;
    DeviceManager *m_device_mgr;
    QString       *m_save_location;
    int           *m_w;
    int           *m_h;

    bool  m_auto_scan_mode;
    bool  m_scan;
    float m_scan_distance;
    float m_scan_step;
    float m_scan_stopping_delay;
    QString m_scan_name;
    cv::Mat m_scan_3d;
    cv::Mat m_scan_sum;
    int   m_scan_idx;
    std::vector<std::pair<float,float>> m_scan_ptz_route;
    std::vector<float> m_scan_tcu_route;
    std::vector<float> m_scan_gate_width_route;
    int   m_scan_ptz_idx;
    int   m_scan_tcu_idx;
};

#endif // SCANCONTROLLER_H
