#ifndef TCUCONTROLLER_H
#define TCUCONTROLLER_H

#include <QObject>
#include <QLabel>
#include <QLineEdit>

#include "port/tcu.h"
#include "util/config.h"

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class Preferences;
class DeviceManager;
class ScanConfig;
class Aliasing;

class TCUController : public QObject
{
    Q_OBJECT

public:
    explicit TCUController(Config *config, DeviceManager *device_mgr, QObject *parent = nullptr);

    void init(Ui::UserPanel *ui, Preferences *pref, ScanConfig *scan_config, Aliasing *aliasing);

    // Getters for state accessed by other components
    float get_stepping() const      { return stepping; }
    int   get_hz_unit() const       { return hz_unit; }
    int   get_base_unit() const     { return base_unit; }
    float get_rep_freq() const      { return rep_freq; }
    float get_laser_width() const   { return laser_width; }
    float get_delay_dist() const    { return delay_dist; }
    float get_depth_of_view() const { return depth_of_view; }
    int   get_mcp_max() const       { return mcp_max; }
    bool  get_aliasing_mode() const { return aliasing_mode; }
    int   get_aliasing_level() const{ return aliasing_level; }
    float get_c() const             { return c; }
    float get_dist_ns() const       { return dist_ns; }
    bool  get_auto_mcp() const      { return auto_mcp; }
    bool  get_frame_a_3d() const    { return frame_a_3d; }
    int   get_distance() const      { return distance; }

    // Setters used by other components
    void set_stepping(float v)      { stepping = v; }
    void set_rep_freq(float v)      { rep_freq = v; }
    void set_laser_width(float v)   { laser_width = v; }
    void set_delay_dist(float v)    { delay_dist = v; }
    void set_depth_of_view(float v) { depth_of_view = v; }
    void set_frame_a_3d(bool v)     { frame_a_3d = v; }
    void set_auto_mcp(bool v)       { auto_mcp = v; }
    void set_distance(int v)        { distance = v; }

    // Convert data to TCU hex buffer
    QByteArray convert_to_send_tcu(uchar num, unsigned int send);

public slots:
    // Signaled by Preferences
    void update_light_speed(bool uw);
    void setup_hz(int hz_unit);
    void setup_stepping(int base_unit);
    void setup_max_dist(float max_dist);
    void setup_max_dov(float max_dov);
    void update_delay_offset(float dist_offset);
    void update_gate_width_offset(float dov_offset);
    void update_laser_offset(float laser_offset);
    void setup_laser(int laser_on);
    void set_tcu_type(int idx);
    void update_ps_config(bool read, int idx, uint val);
    void set_auto_mcp_slot(bool auto_mcp);

    // Signaled by Aliasing
    void set_distance_set(int id);

    // Signaled by DeviceManager (forwarded from TCU port)
    void update_tcu_params(qint32 tcu_param);

    // Slider-controlled
    void change_mcp(int val);
    void change_delay(int val);
    void change_gatewidth(int val);

    // Called by UserPanel::on_DIST_BTN_clicked after obtaining distance
    void apply_distance(int dist);

    // UI button slots
    void on_SWITCH_TCU_UI_BTN_clicked();
    void on_AUTO_MCP_CHK_clicked();
    void on_SIMPLE_LASER_CHK_clicked();

    // Internal update
    void update_delay();
    void update_gate_width();
    void update_laser_width();
    void update_current();

signals:
    void send_double_tcu_msg(qint32 tcu_param, double val);
    void send_uint_tcu_msg(qint32 tcu_param, uint val);
    void send_laser_msg(QString msg);

    // For UserPanel to forward to scan_config
    void dist_ns_changed(float dist_ns);

private:
    void update_tcu_param_pos(int dir, QLabel *u_unit, QLineEdit *n_input, QLabel *n_unit, QLineEdit *p_input);
    void split_value_by_unit(float val, uint &us, uint &ns, uint &ps, int idx = -1);

    Config*         m_config;
    DeviceManager*  m_device_mgr;
    Ui::UserPanel*  m_ui;
    Preferences*    m_pref;
    ScanConfig*     m_scan_config;
    Aliasing*       m_aliasing;

    float           stepping;
    int             hz_unit;
    int             base_unit;
    float           rep_freq;
    float           laser_width;
    float           delay_dist;
    float           depth_of_view;
    int             mcp_max;
    bool            aliasing_mode;
    int             aliasing_level;
    float           c;
    float           dist_ns;
    bool            auto_mcp;
    bool            frame_a_3d;
    int             distance;
};

#endif // TCUCONTROLLER_H
