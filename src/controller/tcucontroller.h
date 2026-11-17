#ifndef TCUCONTROLLER_H
#define TCUCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <atomic>

#include "port/tcu.h"
#include "util/config.h"

class DeviceManager;
class Aliasing;

class TCUController : public QObject
{
    Q_OBJECT

public:
    explicit TCUController(Config *config, DeviceManager *device_mgr, QObject *parent = nullptr);

    void init(Aliasing *aliasing);

    float get_stepping() const      { return m_stepping; }
    int   get_hz_unit() const       { return m_hz_unit; }
    int   get_base_unit() const     { return m_base_unit; }
    float get_rep_freq() const      { return m_rep_freq; }
    float get_laser_width() const   { return m_laser_width; }
    float get_delay_dist() const    { return m_delay_dist.load(); }
    float get_depth_of_view() const { return m_depth_of_view; }
    int   get_mcp_max() const       { return m_mcp_max; }
    bool  get_aliasing_mode() const { return m_aliasing_mode; }
    int   get_aliasing_level() const{ return m_aliasing_level; }
    float get_c() const             { return m_c; }
    float get_dist_ns() const       { return m_dist_ns; }
    // The delay floor in metres at the current light speed. The config keeps
    // it in ns because it is a timing limit of the TCU, not a range.
    float min_delay_dist() const    { return m_config->get_data().tcu.min_delay * m_dist_ns; }
    bool  get_auto_mcp() const      { return m_auto_mcp; }
    int   get_distance() const      { return m_distance; }

    void set_stepping(float v)      { m_stepping = v; }
    void set_rep_freq(float v)      { m_rep_freq = v; }
    void set_laser_width(float v)   { m_laser_width = v; }
    void set_delay_dist(float v)    { m_delay_dist.store(v); emit pipeline_inputs_changed(); }
    void set_depth_of_view(float v) { m_depth_of_view = v;   emit pipeline_inputs_changed(); }
    void set_auto_mcp(bool v)       { m_auto_mcp = v;        emit pipeline_inputs_changed(); }
    void set_distance(int v)        { m_distance = v; }

    QByteArray convert_to_send_tcu(uchar num, unsigned int send);

public slots:
    // signalled by the setup overlay
    void update_light_speed(bool uw);
    void setup_hz(int hz_unit);
    void setup_stepping(int base_unit);
    void setup_max_dist(float max_dist);
    void setup_max_dov(float max_dov);
    void setup_min_delay();
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

    // External controls: the UI follows at once and values are clamped to
    // the configured limits.
    void apply_laser_width(float val);
    void apply_delay(int val);
    void apply_gatewidth(int val);

    // Called by UserPanel::on_DIST_BTN_clicked after obtaining distance
    void apply_distance(int dist);

    // A scan step. Unlike the operator's controls it may go below the delay floor: a scan
    // asks for its delays on purpose. The upper limit still holds.
    void apply_scan_delay(float delay_dist);

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

    // UserPanel copies it into ScanController::get_params().dist_ns
    void dist_ns_changed(float dist_ns);
    void update_distance_display_requested();
    // Base unit, delay, gate width, dist_ns or auto MCP may have changed: the values the
    // frame path works with. UserPanel refreshes the pipeline's copy.
    void pipeline_inputs_changed();

    // UI update signals
    void freq_display_updated(QString unit_text, QString value_text);
    void stepping_display_updated(QString unit_text, QString value_text);
    void delay_slider_max_changed(int max);
    void delay_slider_min_changed(int min);
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

    // Layout changes. diff marks a delay / gate-width reflow between 0 and non-0.
    // laser_diff marks the same for LASER_WIDTH, shown only at the 40 ps step (idx 2).
    void tcu_type_layout_changed(int idx, int diff, int laser_diff);
    void tcu_diff_view_toggled(bool show_diff);

    // setup overlay forwarding
    void ps_config_display_updated(QString stepping_text, QString max_step_text);
    void auto_mcp_chk_click_requested();
    void fire_laser_btn_click_requested();
    void laser_chk_click_requested();

private:
    void split_value_by_unit(float val, uint &us, uint &ns, uint &ps, int idx = -1);

    // One-deep mailbox per continuously driven command. The flow control it
    // implements is described above request() in the .cpp.
    struct Mailbox {
        double desired   = 0;      // latest requested value
        double sent      = -1;     // last dispatched value; < 0 = none since readback
        bool   pending   = false;  // a request arrived while a command was in flight
        bool   in_flight = false;  // dispatched, echo not yet seen
        QTimer watchdog;
    };
    void request(Mailbox &box, qint32 param, double val);
    void dispatch(Mailbox &box, qint32 param);
    bool accept_echo(Mailbox &box, qint32 param, double echoed);
    void echo_timeout(Mailbox &box, qint32 param);
    void resync(Mailbox &box);

    Config         *m_config;
    DeviceManager  *m_device_mgr;
    Aliasing       *m_aliasing;

    float           m_stepping;
    int             m_hz_unit;
    int             m_base_unit;
    float           m_rep_freq;
    float           m_laser_width;
    std::atomic<float> m_delay_dist{0.0f};
    float           m_depth_of_view;
    int             m_mcp_max;
    bool            m_aliasing_mode;
    int             m_aliasing_level;
    float           m_c;
    float           m_dist_ns;
    bool            m_auto_mcp;
    int             m_distance;

    Mailbox         m_delay_box;
    Mailbox         m_dov_box;
};

#endif // TCUCONTROLLER_H
