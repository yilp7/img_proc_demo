#ifndef SCANCONTROLLER_H
#define SCANCONTROLLER_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <opencv2/core.hpp>
#include <vector>
#include <utility>

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class ScanConfig;
class TCUController;
class DeviceManager;

class ScanController : public QObject
{
    Q_OBJECT

public:
    explicit ScanController(TCUController *tcu_ctrl, DeviceManager *device_mgr,
                            QObject *parent = nullptr);

    void init(Ui::UserPanel *ui, ScanConfig *scan_config,
              QString *save_location, int *w, int *h);

    // Getters (mutex-protected for cross-thread access)
    bool is_scanning() const { QMutexLocker lk(&m_scan_mutex); return scan; }
    bool is_auto_scan_mode() const { QMutexLocker lk(&m_scan_mutex); return auto_scan_mode; }
    float get_scan_distance() const { QMutexLocker lk(&m_scan_mutex); return scan_distance; }
    float get_scan_step() const { QMutexLocker lk(&m_scan_mutex); return scan_step; }
    float get_scan_stopping_delay() const { QMutexLocker lk(&m_scan_mutex); return scan_stopping_delay; }
    QString get_scan_name() const { QMutexLocker lk(&m_scan_mutex); return scan_name; }
    cv::Mat& get_scan_3d() { return scan_3d; }
    cv::Mat& get_scan_sum() { return scan_sum; }
    int get_scan_idx() const { QMutexLocker lk(&m_scan_mutex); return scan_idx; }
    std::vector<std::pair<float,float>> get_scan_ptz_route() const { QMutexLocker lk(&m_scan_mutex); return scan_ptz_route; }
    std::vector<float> get_scan_tcu_route() const { QMutexLocker lk(&m_scan_mutex); return scan_tcu_route; }
    int get_scan_ptz_idx() const { QMutexLocker lk(&m_scan_mutex); return scan_ptz_idx; }
    int get_scan_tcu_idx() const { QMutexLocker lk(&m_scan_mutex); return scan_tcu_idx; }

    // Setters (mutex-protected for cross-thread access)
    void set_scanning(bool v) { QMutexLocker lk(&m_scan_mutex); scan = v; }
    void set_auto_scan_mode(bool v) { QMutexLocker lk(&m_scan_mutex); auto_scan_mode = v; }
    void set_scan_idx(int v) { QMutexLocker lk(&m_scan_mutex); scan_idx = v; }
    void set_scan_ptz_idx(int v) { QMutexLocker lk(&m_scan_mutex); scan_ptz_idx = v; }
    void set_scan_tcu_idx(int v) { QMutexLocker lk(&m_scan_mutex); scan_tcu_idx = v; }
    void set_scan_step(float v) { QMutexLocker lk(&m_scan_mutex); scan_step = v; }
    void set_scan_stopping_delay(float v) { QMutexLocker lk(&m_scan_mutex); scan_stopping_delay = v; }

public slots:
    void on_SCAN_BUTTON_clicked();
    void on_CONTINUE_SCAN_BUTTON_clicked();
    void on_RESTART_SCAN_BUTTON_clicked();
    void on_SCAN_CONFIG_BTN_clicked();
    void enable_scan_options(bool show);
    void auto_scan_for_target();

signals:
    void update_scan(bool show);
    void finish_scan();

private:
    mutable QMutex m_scan_mutex;

    Ui::UserPanel* m_ui;
    ScanConfig*    m_scan_config;
    TCUController* m_tcu_ctrl;
    DeviceManager* m_device_mgr;
    QString*       m_save_location;
    int*           m_w;
    int*           m_h;

    bool  auto_scan_mode;
    bool  scan;
    float scan_distance;
    float scan_step;
    float scan_stopping_delay;
    QString scan_name;
    cv::Mat scan_3d;
    cv::Mat scan_sum;
    int   scan_idx;
    std::vector<std::pair<float,float>> scan_ptz_route;
    std::vector<float> scan_tcu_route;
    int   scan_ptz_idx;
    int   scan_tcu_idx;
};

#endif // SCANCONTROLLER_H
