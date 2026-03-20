#ifndef TCUCONTROLLER_H
#define TCUCONTROLLER_H

#include <QObject>
#include <atomic>

#include "port/tcu.h"
#include "util/config.h"

class DeviceManager;
class ScanConfig;
class Aliasing;

class TCUController : public QObject
{
    Q_OBJECT

public:
    explicit TCUController(Config *config, DeviceManager *device_mgr, QObject *parent = nullptr);

    void init(ScanConfig *scan_config, Aliasing *aliasing);

    // Getters for state accessed by other components
    float get_stepping() const      { return stepping; }
    int   get_hz_unit() const       { return hz_unit; }
    int   get_base_unit() const     { return base_unit; }
    float get_rep_freq() const      { return rep_freq; }
    float get_laser_width() const   { return laser_width; }
    float get_delay_dist() const    { return delay_dist.load(); }
    float get_depth_of_view() const { return depth_of_view; }
    int   get_mcp_max() const       { return mcp_max; }
    bool  get_aliasing_mode() const { return aliasing_mode; }
    int   get_aliasing_level() const{ return aliasing_level; }
    float get_c() const             { return c; }
    float get_dist_ns() const       { return dist_ns; }
    bool  get_auto_mcp() const      { return auto_mcp; }
    bool  get_frame_a_3d() const    { return frame_a_3d.load(); }
    int   get_distance() const      { return distance; }

    // Setters used by other components
    void set_stepping(float v)      { stepping = v; }
    void set_rep_freq(float v)      { rep_freq = v; }
    void set_laser_width(float v)   { laser_width = v; }
    void set_delay_dist(float v)    { delay_dist.store(v); }
    void set_depth_of_view(float v) { depth_of_view = v; }
    void set_frame_a_3d(bool v)     { frame_a_3d.store(v); }
    void toggle_frame_a_3d() {
        bool expected = frame_a_3d.load();
        while (!frame_a_3d.compare_exchange_weak(expected, !expected));
    }
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
    void update_current(float current_value);

signals:
    void send_double_tcu_msg(qint32 tcu_param, double val);
    void send_uint_tcu_msg(qint32 tcu_param, uint val);
    void send_laser_msg(QString msg);

    // For UserPanel to forward to scan_config / preferences
    void dist_ns_changed(float dist_ns);
    void update_distance_display_requested();

    // UI update signals
    void freq_display_updated(QString unit_text, QString value_text);
    void stepping_display_updated(QString unit_text, QString value_text);
    void delay_slider_max_changed(int max);
    void gw_slider_max_changed(int max);
    void mcp_slider_max_changed(int max);
    void mcp_display_updated(int slider_val, QString edit_text);
    void delay_slider_value_changed(int val);
    void gw_slider_value_changed(int val);
    void auto_mcp_chk_changed(bool checked);
    void distance_text_updated(QString text);
    void est_dist_text_updated(QString text);

    // TCU param display updates (grouped by section)
    void laser_width_display_updated(QString u, QString n, QString p);
    void delay_display_updated(QString au, QString an, QString ap,
                               QString bu, QString bn, QString bp);
    void gw_display_updated(QString au, QString an, QString ap,
                            QString bu, QString bn, QString bp,
                            QString gw_text);

    // Layout changes
    void tcu_type_layout_changed(int idx, int diff);
    void tcu_diff_view_toggled(bool show_diff);

    // Preferences forwarding
    void ps_config_display_updated(QString stepping_text, QString max_step_text);
    void auto_mcp_chk_click_requested();
    void fire_laser_btn_click_requested();
    void laser_chk_click_requested();

private:
    void split_value_by_unit(float val, uint &us, uint &ns, uint &ps, int idx = -1);

    Config*         m_config;
    DeviceManager*  m_device_mgr;
    ScanConfig*     m_scan_config;
    Aliasing*       m_aliasing;

    float           stepping;
    int             hz_unit;
    int             base_unit;
    float           rep_freq;
    float           laser_width;
    std::atomic<float> delay_dist{0.0f};
    float           depth_of_view;
    int             mcp_max;
    bool            aliasing_mode;
    int             aliasing_level;
    float           c;
    float           dist_ns;
    bool            auto_mcp;
    std::atomic<bool> frame_a_3d{false};
    int             distance;
};

#endif // TCUCONTROLLER_H
