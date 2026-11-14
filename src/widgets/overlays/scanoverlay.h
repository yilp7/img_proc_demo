#ifndef SCANOVERLAY_H
#define SCANOVERLAY_H

#include <QFrame>

class QHBoxLayout;
class QScrollArea;
class QVBoxLayout;
class QWidget;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QRadioButton;
class QLabel;
class ScanController;

// Full-window overlay of the scan mode: a scrollable row of setting cards and a
// fixed preview sidebar. Keeps the widgets in sync with ScanController::get_params():
// bind() connects them, load_from_params() pushes the values back after a preset load.
class ScanOverlay : public QFrame
{
    Q_OBJECT
public:
    explicit ScanOverlay(QWidget *parent = nullptr);

    // The QWidget whose layout holds the cards row (pass as parent_w to addWidget).
    QWidget     *get_content_widget()  const { return m_cards_container; }

    // the row's QHBoxLayout: callers add their cards, then one stretch
    QHBoxLayout *get_cards_layout()    const { return m_cards_layout; }

    // QVBoxLayout of the right preview sidebar
    QVBoxLayout *get_preview_layout()  const { return m_preview_layout; }

    // Widget pointers UserPanel hands over. All are optional: a null field means the
    // widget does not exist in the layout.
    struct Widgets {
        QLineEdit  *start_delay_u   = nullptr;
        QLineEdit  *start_delay_n   = nullptr;
        QLineEdit  *start_delay_m   = nullptr;
        QLineEdit  *end_delay_u     = nullptr;
        QLineEdit  *end_delay_n     = nullptr;
        QLineEdit  *end_delay_m     = nullptr;
        QLineEdit  *start_gw_u      = nullptr;
        QLineEdit  *start_gw_n      = nullptr;
        QLineEdit  *start_gw_m      = nullptr;
        QLineEdit  *end_gw_u        = nullptr;
        QLineEdit  *end_gw_n        = nullptr;
        QLineEdit  *end_gw_m        = nullptr;
        QLineEdit  *step_delay      = nullptr;
        QLineEdit  *step_gw         = nullptr;
        QLineEdit  *frame_count     = nullptr;
        QLineEdit  *rep_freq        = nullptr;
        QLineEdit  *start_h         = nullptr;
        QLineEdit  *start_v         = nullptr;
        QLineEdit  *end_h           = nullptr;
        QLineEdit  *end_v           = nullptr;
        QLineEdit  *step_h          = nullptr;
        QLineEdit  *step_v          = nullptr;
        QLineEdit  *frame_h         = nullptr;
        QLineEdit  *frame_v         = nullptr;
        QLineEdit  *ptz_wait        = nullptr;
        QLineEdit  *num_single_pos  = nullptr;
        QCheckBox  *capture_ori     = nullptr;
        QCheckBox  *capture_res     = nullptr;
        QCheckBox  *record_ori      = nullptr;
        QCheckBox  *record_res      = nullptr;
        QRadioButton *dir_n         = nullptr;
        QRadioButton *dir_z         = nullptr;
        QRadioButton *mode_grid     = nullptr;
        QRadioButton *mode_panorama = nullptr;
        QCheckBox  *enable_delay    = nullptr;
        QCheckBox  *enable_gate     = nullptr;
        QCheckBox  *enable_ptz      = nullptr;
        QWidget    *grid_delay_card = nullptr;
        QWidget    *grid_gate_card  = nullptr;
        QWidget    *grid_ptz_card   = nullptr;
        QWidget    *grid_delay_body = nullptr;
        QWidget    *grid_gate_body  = nullptr;
        QWidget    *grid_ptz_body   = nullptr;
        QWidget    *grid_capture_card = nullptr;
        QWidget    *panorama_card   = nullptr;
        QLineEdit  *panorama_start_h = nullptr;
        QCheckBox  *panorama_use_end_h = nullptr;
        QLineEdit  *panorama_end_h   = nullptr;
        QLineEdit  *panorama_fixed_v = nullptr;
        QLineEdit  *panorama_step_h  = nullptr;
        QLineEdit  *panorama_count   = nullptr;
        QLineEdit  *panorama_wait    = nullptr;
        QLineEdit  *panorama_zoom    = nullptr;
        QComboBox  *panorama_fusion  = nullptr;
        QRadioButton *panorama_cw   = nullptr;
        QRadioButton *panorama_ccw  = nullptr;
        QLabel     *unit_1          = nullptr;   // step-delay unit label (ns or m)
        QLabel     *unit_2          = nullptr;   // step-gw unit label
        QLabel     *freq_unit       = nullptr;   // kHz / Hz
        // μs/ns/m mode-toggle widgets (hidden in unsupported modes)
        QWidget    *unit_m_1        = nullptr;
        QWidget    *unit_m_2        = nullptr;
        QWidget    *unit_m_3        = nullptr;
        QWidget    *unit_m_4        = nullptr;
        QWidget    *unit_us_1       = nullptr;
        QWidget    *unit_us_2       = nullptr;
        QWidget    *unit_us_3       = nullptr;
        QWidget    *unit_us_4       = nullptr;
        QWidget    *unit_ns_1       = nullptr;
        QWidget    *unit_ns_2       = nullptr;
        QWidget    *unit_ns_3       = nullptr;
        QWidget    *unit_ns_4       = nullptr;
    };

    // Stores the controller and the widgets and makes the connections. After it
    // returns, widget edits go into ScanController::get_params().
    void bind(ScanController *ctrl, const Widgets &w);

    // Pushes ScanController::get_params() back into the widgets, after a preset load.
    void load_from_params();

    // Switches the frequency unit (kHz / Hz): stores hz_unit and updates the rep_freq edit and label.
    void setup_hz(int idx);

    // Switches the base unit (0 ns, 1 μs, 2 m): shows the matching widget group and
    // rewrites the step labels and edits.
    void setup_unit(int idx);

private slots:
    // widget edits to ScanController::get_params(), with the derivation rules
    void on_tcu_edit();
    void on_ptz_edit();
    void on_capture_ori(int s);
    void on_capture_res(int s);
    void on_record_ori(int s);
    void on_record_res(int s);
    void on_dir_n();
    void on_dir_z();
    void on_ptz_wait_edit();
    void on_num_single_pos_edit();
    void on_mode_grid();
    void on_mode_panorama();
    void on_grid_section_toggled();
    void on_panorama_edit();
    void on_panorama_cw();
    void on_panorama_ccw();

private:
    void update_mode_enabled();

    QScrollArea *m_scroll;
    QWidget     *m_cards_container;
    QHBoxLayout *m_cards_layout;
    QWidget     *m_preview_sidebar;
    QVBoxLayout *m_preview_layout;

    ScanController *m_ctrl;
    Widgets         m_w;
};

#endif // SCANOVERLAY_H
