#ifndef SETUPOVERLAY_H
#define SETUPOVERLAY_H

#include <QByteArray>
#include <QFrame>
#include <QString>
#include <QStringList>

class QButtonGroup;
class QDialog;
class QEvent;
class QHBoxLayout;
class QObject;
class QScrollArea;
class QShowEvent;
class QTranslator;
class QVBoxLayout;
class QWidget;

class Config;

namespace Ui { class Preferences; }

// Full-window SETUP-mode overlay. Owns the widgets that used to live in the
// legacy Preferences dialog (instantiated here via Ui::Preferences::setupUi
// on a hidden form root, then re-parented into 3 horizontal rows of cards),
// plus a fixed preview sidebar on the right.
//
// The overlay is hidden by default. UserPanel toggles it on TopBar mode
// changes (Setup -> show, Monitor/Scan -> hide).
class SetupOverlay : public QFrame
{
    Q_OBJECT
public:
    explicit SetupOverlay(QWidget *parent = nullptr);
    ~SetupOverlay();

    // Build the widgets via Ui::Preferences::setupUi(form_root) and re-parent
    // the 5 top-level groups (SAVE_LOAD_GRP, SERIAL_PORTS_GRP, TCU_GRP,
    // DEVICES_GRP, IMG_PROC_GRP) into the row containers.
    void setup_form();

    // Wire all widget signal handlers (combo boxes, checkboxes, line edits)
    // to update Config and re-emit the SetupOverlay signals UserPanel listens
    // to. Must be called after setup_form() and set_config().
    void bind();

    void set_config(Config *config) { m_config = config; }
    Config *config() const { return m_config; }

    // Direct access to the generated widget struct so UserPanel can attach
    // additional behavior (icon, theming, custom signal hooks) to specific
    // widgets. Replaces the legacy `pref->ui->X` pattern.
    Ui::Preferences *w() const { return m_ui; }

    // Access the row containers / preview sidebar for cards we still inject
    // from UserPanel (PARAMS_STATIC, IMG_GRAB_STATIC, preview thumbnails).
    QWidget *contentRow(int idx) const;
    QHBoxLayout *rowLayout(int idx) const;
    QVBoxLayout *previewSidebarLayout() const { return m_preview_layout; }

    // Ported from Preferences --
    void data_exchange(bool read);
    void config_ip(bool read, int ip = 0, int gateway = 0, int nic_address = 0);
    void enable_ip_editing(bool enable);
    void set_pixel_format(int idx);
    void update_distance_display();
    void display_baudrate(int id, int baudrate);
    void switch_language(bool en, QTranslator *trans);
    void set_ptz_type_enabled(bool enabled);

    // State that used to live on Preferences --
    bool  split() const         { return m_split; }
    bool  cameralink() const    { return m_cameralink; }
    float dist_ns() const       { return m_dist_ns; }
    void  set_dist_ns(float v)  { m_dist_ns = v; }

public slots:
    // Mirrors Preferences::update_device_list -- DeviceManager signals into this.
    void update_device_list(int cmd, QStringList dev_list = QStringList());
    // Mirrors Preferences::toggle_laser (laser_grp buttonToggled).
    void toggle_laser(int id, bool on);

signals:
    // device
    void search_for_devices();
    void query_dev_ip();
    void set_dev_ip(int ip, int gateway);
    void change_pixel_format(int format);
    void rotate_image(int angle);
    void device_underwater(bool underwater);

    // serial comm.
    void get_port_info(int com_idx);
    void change_baudrate(int idx, int baudrate);
    void set_tcp_status(int idx, bool use_tcp);
    void share_tcu_port(bool share);
    void com_write(int idx, QByteArray data);

    // tcu config
    void tcu_type_changed(int idx);
    void set_auto_rep_freq(bool _auto);
    void set_auto_mcp(bool _auto);
    void set_ab_lock(bool _auto);
    void rep_freq_unit_changed(int idx);
    void base_unit_changed(int idx);
    void max_dist_changed(float dist);
    void max_dov_changed(float dov);
    void max_laser_changed(float laser_width);
    void delay_offset_changed(float dist);
    void gate_width_offset_changed(float dov);
    void laser_offset_changed(float laser_width);
    void laser_toggled(int config);
    void ps_config_updated(bool read, int idx, uint val);

    // image process
    void lower_3d_thresh_updated();
    void query_tcu_param();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void send_cmd(QString str);

    // Row / sidebar scaffolding
    QScrollArea *m_scroll;
    QWidget     *m_rows_container;
    QWidget     *m_row[3];
    QHBoxLayout *m_row_layout[3];
    QWidget     *m_preview_sidebar;
    QVBoxLayout *m_preview_layout;

    // Generated widgets (live under m_form_root, then re-parented into rows)
    QDialog          *m_form_root;
    Ui::Preferences  *m_ui;

    Config           *m_config;
    QButtonGroup     *m_laser_grp;

    // Runtime state previously held on Preferences
    bool   m_split;
    bool   m_cameralink;
    float  m_dist_ns;
};

#endif // SETUPOVERLAY_H
