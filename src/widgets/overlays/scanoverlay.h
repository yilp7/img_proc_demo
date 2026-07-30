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

// Full-window SCAN-mode overlay. A single horizontal row of config group cards
// inside a QScrollArea (left) plus a fixed preview sidebar (right, ~220 px).
//
// Owns the widget→params sync that used to live on ScanConfig (S22):
// `bind()` plugs in pointers to the re-parented widgets + the ScanController;
// `setup_hz` / `setup_unit` swap the μs↔ns↔m display modes; the
// `update_scan_tcu` / `update_scan_ptz` slots run derivation rules on edit;
// `loadFromParams` pushes m_params back into the widgets after a preset load.
//
// Hidden by default; UserPanel toggles on TopBar::modeChanged(Scan).
class ScanOverlay : public QFrame
{
    Q_OBJECT
public:
    explicit ScanOverlay(QWidget *parent = nullptr);

    // The QWidget whose layout holds the cards row (pass as parent_w to addWidget).
    QWidget     *contentWidget()  const { return m_cards_container; }

    // QHBoxLayout of the cards row — callers addWidget() cards here, then
    // addStretch(1) once all cards are inserted.
    QHBoxLayout *cardsLayout()    const { return m_cards_layout; }

    // QVBoxLayout of the right preview sidebar (task 19.5).
    QVBoxLayout *previewLayout()  const { return m_preview_layout; }

    // ── S22: ScanConfig replacement ─────────────────────────────────────────
    // Widget pointers UserPanel hands over after re-parenting them into the
    // SCAN overlay cards. All optional — leave fields null for widgets that
    // don't exist in a given layout (none currently).
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

    // Plug in the controller + widget references and wire all signal/slot
    // connections that used to be set up in ScanConfig's constructor.
    // After this returns, widget edits propagate into ScanController::params().
    void bind(ScanController *ctrl, const Widgets &w);

    // Push ScanController::params() values back to the widgets. Used after
    // preset LOAD (replaces ScanConfig::data_exchange(false)).
    void loadFromParams();

    // Switch frequency display unit (kHz / Hz). Stores hz_unit in params,
    // updates rep_freq edit + label. Replaces ScanConfig::setup_hz.
    void setup_hz(int idx);

    // Switch base unit (0 ns, 1 μs, 2 m). Shows/hides μs/ns vs m widget
    // groups, updates step-unit labels, rewrites step edits.
    // Replaces ScanConfig::setup_unit.
    void setup_unit(int idx);

private slots:
    // Widget edits → ScanController::params() (with derivation rules).
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
