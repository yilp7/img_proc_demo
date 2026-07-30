#include "scanoverlay.h"

#include "controller/scancontroller.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>

namespace {

const int kMaxPanoramaStops = 2000;

int clamp_int(int value, int lo, int hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

} // namespace

ScanOverlay::ScanOverlay(QWidget *parent)
    : QFrame(parent),
      m_scroll(nullptr),
      m_cards_container(nullptr),
      m_cards_layout(nullptr),
      m_preview_sidebar(nullptr),
      m_preview_layout(nullptr),
      m_ctrl(nullptr)
{
    setObjectName("SCAN_OVERLAY");

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // Left: horizontally scrollable cards row
    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("SCAN_SCROLL");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_cards_container = new QWidget(m_scroll);
    m_cards_layout = new QHBoxLayout(m_cards_container);
    m_cards_layout->setContentsMargins(0, 0, 0, 0);
    m_cards_layout->setSpacing(8);
    // Callers insert cards then addStretch(1) — no trailing stretch here.
    m_scroll->setWidget(m_cards_container);

    // Right: fixed preview sidebar
    m_preview_sidebar = new QWidget(this);
    m_preview_sidebar->setObjectName("SCAN_PREVIEW_SIDEBAR");
    m_preview_sidebar->setAttribute(Qt::WA_StyledBackground, true);
    m_preview_sidebar->setFixedWidth(220);
    m_preview_layout = new QVBoxLayout(m_preview_sidebar);
    m_preview_layout->setContentsMargins(0, 0, 0, 0);
    m_preview_layout->setSpacing(6);
    auto *ph = new QLabel("Display Previews\n—\npopulated in 19.5", m_preview_sidebar);
    ph->setObjectName("SCAN_PREVIEW_PH");
    ph->setAlignment(Qt::AlignCenter);
    m_preview_layout->addWidget(ph);
    m_preview_layout->addStretch(1);

    root->addWidget(m_scroll, 1);
    root->addWidget(m_preview_sidebar);
}

// ─────────────────────────────────────────────────────────────────────────────
// S22: ScanConfig replacement — bind widgets + controller, set up connections,
// implement the same widget-sync slots ScanConfig used to provide.
// ─────────────────────────────────────────────────────────────────────────────

void ScanOverlay::bind(ScanController *ctrl, const Widgets &w)
{
    m_ctrl = ctrl;
    m_w = w;

    auto tcu_edits = {
        m_w.start_delay_u, m_w.start_delay_n, m_w.start_delay_m,
        m_w.end_delay_u,   m_w.end_delay_n,   m_w.end_delay_m,
        m_w.start_gw_u,    m_w.start_gw_n,    m_w.start_gw_m,
        m_w.end_gw_u,      m_w.end_gw_n,      m_w.end_gw_m,
        m_w.frame_count,   m_w.step_delay,    m_w.step_gw
    };
    for (auto *e : tcu_edits)
        if (e) connect(e, &QLineEdit::editingFinished, this, &ScanOverlay::on_tcu_edit);

    auto ptz_edits = {
        m_w.start_h, m_w.start_v, m_w.end_h, m_w.end_v,
        m_w.step_h,  m_w.step_v,  m_w.frame_h, m_w.frame_v
    };
    for (auto *e : ptz_edits)
        if (e) connect(e, &QLineEdit::editingFinished, this, &ScanOverlay::on_ptz_edit);

    if (m_w.capture_ori) connect(m_w.capture_ori, &QCheckBox::stateChanged, this, &ScanOverlay::on_capture_ori);
    if (m_w.capture_res) connect(m_w.capture_res, &QCheckBox::stateChanged, this, &ScanOverlay::on_capture_res);
    if (m_w.record_ori)  connect(m_w.record_ori,  &QCheckBox::stateChanged, this, &ScanOverlay::on_record_ori);
    if (m_w.record_res)  connect(m_w.record_res,  &QCheckBox::stateChanged, this, &ScanOverlay::on_record_res);

    if (m_w.dir_n) connect(m_w.dir_n, &QRadioButton::clicked, this, &ScanOverlay::on_dir_n);
    if (m_w.dir_z) connect(m_w.dir_z, &QRadioButton::clicked, this, &ScanOverlay::on_dir_z);
    if (m_w.dir_n) m_w.dir_n->setChecked(true);

    if (m_w.ptz_wait)       connect(m_w.ptz_wait,       &QLineEdit::editingFinished, this, &ScanOverlay::on_ptz_wait_edit);
    if (m_w.num_single_pos) connect(m_w.num_single_pos, &QLineEdit::editingFinished, this, &ScanOverlay::on_num_single_pos_edit);

    if (m_w.mode_grid)     connect(m_w.mode_grid,     &QRadioButton::clicked, this, &ScanOverlay::on_mode_grid);
    if (m_w.mode_panorama) connect(m_w.mode_panorama, &QRadioButton::clicked, this, &ScanOverlay::on_mode_panorama);
    if (m_w.enable_delay) connect(m_w.enable_delay, &QCheckBox::toggled, this, &ScanOverlay::on_grid_section_toggled);
    if (m_w.enable_gate)  connect(m_w.enable_gate,  &QCheckBox::toggled, this, &ScanOverlay::on_grid_section_toggled);
    if (m_w.enable_ptz)   connect(m_w.enable_ptz,   &QCheckBox::toggled, this, &ScanOverlay::on_grid_section_toggled);

    auto panorama_edits = {
        m_w.panorama_start_h, m_w.panorama_end_h, m_w.panorama_fixed_v,
        m_w.panorama_step_h,  m_w.panorama_count,
        m_w.panorama_wait,    m_w.panorama_zoom
    };
    for (auto *e : panorama_edits)
        if (e) connect(e, &QLineEdit::editingFinished, this, &ScanOverlay::on_panorama_edit);
    if (m_w.panorama_use_end_h) {
        connect(m_w.panorama_use_end_h, &QCheckBox::stateChanged,
                this, [this](int) { on_panorama_edit(); });
    }

    if (m_w.panorama_cw)  connect(m_w.panorama_cw,  &QRadioButton::clicked, this, &ScanOverlay::on_panorama_cw);
    if (m_w.panorama_ccw) connect(m_w.panorama_ccw, &QRadioButton::clicked, this, &ScanOverlay::on_panorama_ccw);
    if (m_w.panorama_fusion) {
        connect(m_w.panorama_fusion, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int) { on_panorama_edit(); });
    }

    loadFromParams();
    setup_hz(0);
    setup_unit(0);
}

void ScanOverlay::loadFromParams()
{
    if (!m_ctrl) return;
    ScanParams &p = m_ctrl->params();
    update_panorama_fov_from_zoom(p);

    if (!p.scan_delay_enabled && !p.scan_gate_width_enabled && !p.scan_ptz_enabled)
        p.scan_ptz_enabled = true;

    auto setT = [](QLineEdit *e, const QString &s) { if (e) e->setText(s); };
    auto setI = [&setT](QLineEdit *e, int v)       { setT(e, QString::number(v)); };
    auto setF = [&setT](QLineEdit *e, double v, int prec = 2) {
        setT(e, QString::number(v, 'f', prec));
    };

    switch (p.base_unit) {
    case 0: case 1: // ns / μs
        setI(m_w.start_delay_u, p.starting_delay / 1000);
        setI(m_w.start_delay_n, p.starting_delay % 1000);
        setI(m_w.end_delay_u,   p.ending_delay / 1000);
        setI(m_w.end_delay_n,   p.ending_delay % 1000);
        setI(m_w.start_gw_u,    p.starting_gw / 1000);
        setI(m_w.start_gw_n,    p.starting_gw % 1000);
        setI(m_w.end_gw_u,      p.ending_gw / 1000);
        setI(m_w.end_gw_n,      p.ending_gw % 1000);
        break;
    case 2: // m
        // legacy quirk preserved: M edits get gw values overwriting delay values
        setI(m_w.start_gw_m, (int)(p.starting_gw * p.dist_ns));
        setI(m_w.end_gw_m,   (int)(p.ending_gw   * p.dist_ns));
        break;
    }
    setI(m_w.frame_count, p.frame_count);
    setF(m_w.step_delay,  p.step_size_delay);
    setF(m_w.step_gw,     p.step_size_gw);
    if (m_w.rep_freq) m_w.rep_freq->setText(QString::number(p.rep_freq));

    if (m_w.capture_ori) m_w.capture_ori->setChecked(p.capture_scan_ori);
    if (m_w.capture_res) m_w.capture_res->setChecked(p.capture_scan_res);
    if (m_w.record_ori)  m_w.record_ori ->setChecked(p.record_scan_ori);
    if (m_w.record_res)  m_w.record_res ->setChecked(p.record_scan_res);

    auto setCheckedBlocked = [](QCheckBox *box, bool checked) {
        if (!box) return;
        QSignalBlocker blocker(box);
        box->setChecked(checked);
    };
    setCheckedBlocked(m_w.enable_delay, p.scan_delay_enabled);
    setCheckedBlocked(m_w.enable_gate,  p.scan_gate_width_enabled);
    setCheckedBlocked(m_w.enable_ptz,   p.scan_ptz_enabled);

    setF(m_w.start_h, p.starting_h);
    setF(m_w.start_v, p.starting_v);
    setF(m_w.end_h,   p.ending_h);
    setF(m_w.end_v,   p.ending_v);
    setF(m_w.step_h,  p.step_h);
    setF(m_w.step_v,  p.step_v);
    setI(m_w.frame_h, p.count_h);
    setI(m_w.frame_v, p.count_v);
    setI(m_w.ptz_wait,       p.ptz_wait_time);
    setI(m_w.num_single_pos, p.num_single_pos);

    if (m_w.mode_grid)     m_w.mode_grid->setChecked(p.ptz_scan_mode == ScanParams::PTZ_SCAN_GRID);
    if (m_w.mode_panorama) m_w.mode_panorama->setChecked(p.ptz_scan_mode == ScanParams::PTZ_SCAN_PANORAMA);
    setF(m_w.panorama_start_h, p.panorama_start_h);
    if (m_w.panorama_use_end_h) m_w.panorama_use_end_h->setChecked(p.panorama_use_end_h);
    setF(m_w.panorama_end_h,   p.panorama_end_h);
    setF(m_w.panorama_fixed_v, p.panorama_fixed_v);
    setF(m_w.panorama_step_h,  p.panorama_step_h);
    setI(m_w.panorama_count,   p.panorama_count);
    setI(m_w.panorama_wait,    p.panorama_wait_time);
    setF(m_w.panorama_zoom,    p.panorama_zoom_level, 2);
    if (m_w.panorama_fusion) {
        QSignalBlocker block(m_w.panorama_fusion);
        m_w.panorama_fusion->setCurrentIndex(clamp_int(p.panorama_fusion_mode, 0, 2));
    }
    if (m_w.panorama_end_h) m_w.panorama_end_h->setEnabled(p.panorama_use_end_h);
    if (m_w.panorama_cw)  m_w.panorama_cw->setChecked(p.panorama_direction == 0);
    if (m_w.panorama_ccw) m_w.panorama_ccw->setChecked(p.panorama_direction != 0);
    update_mode_enabled();
}

void ScanOverlay::setup_hz(int idx)
{
    if (!m_ctrl) return;
    ScanParams &p = m_ctrl->params();
    p.hz_unit = idx;
    switch (idx) {
    case 0: if (m_w.rep_freq)  m_w.rep_freq->setText(QString::number((int)p.rep_freq));
            if (m_w.freq_unit) m_w.freq_unit->setText("kHz"); break;
    case 1: if (m_w.rep_freq)  m_w.rep_freq->setText(QString::number((int)(p.rep_freq * 1000)));
            if (m_w.freq_unit) m_w.freq_unit->setText("Hz");  break;
    default: break;
    }
}

void ScanOverlay::setup_unit(int idx)
{
    if (!m_ctrl) return;
    ScanParams &p = m_ctrl->params();
    p.base_unit = idx;

    auto sv = [](QWidget *w, bool v) { if (w) w->setVisible(v); };

    const bool m_mode = (idx == 2);
    sv(m_w.start_delay_m, m_mode); sv(m_w.end_delay_m, m_mode);
    sv(m_w.start_gw_m,    m_mode); sv(m_w.end_gw_m,    m_mode);
    sv(m_w.unit_m_1, m_mode); sv(m_w.unit_m_2, m_mode);
    sv(m_w.unit_m_3, m_mode); sv(m_w.unit_m_4, m_mode);
    sv(m_w.start_delay_u, !m_mode); sv(m_w.start_delay_n, !m_mode);
    sv(m_w.end_delay_u,   !m_mode); sv(m_w.end_delay_n,   !m_mode);
    sv(m_w.start_gw_u,    !m_mode); sv(m_w.start_gw_n,    !m_mode);
    sv(m_w.end_gw_u,      !m_mode); sv(m_w.end_gw_n,      !m_mode);
    sv(m_w.unit_us_1, !m_mode); sv(m_w.unit_us_2, !m_mode);
    sv(m_w.unit_us_3, !m_mode); sv(m_w.unit_us_4, !m_mode);
    sv(m_w.unit_ns_1, !m_mode); sv(m_w.unit_ns_2, !m_mode);
    sv(m_w.unit_ns_3, !m_mode); sv(m_w.unit_ns_4, !m_mode);

    if (m_w.step_delay) m_w.step_delay->setText(QString::number(
        m_mode ? p.step_size_delay * p.dist_ns : p.step_size_delay, 'f', 2));
    if (m_w.step_gw)    m_w.step_gw->setText(QString::number(
        m_mode ? p.step_size_gw * p.dist_ns : p.step_size_gw, 'f', 2));
    if (m_w.unit_1) m_w.unit_1->setText(m_mode ? "m" : "ns");
    if (m_w.unit_2) m_w.unit_2->setText(m_mode ? "m" : "ns");
}

void ScanOverlay::on_tcu_edit()
{
    if (!m_ctrl) return;
    ScanParams &p = m_ctrl->params();

    auto txt = [](QLineEdit *e) { return e ? e->text().toInt()   : 0; };
    auto txtf= [](QLineEdit *e) { return e ? e->text().toFloat() : 0.f; };

    switch (p.base_unit) {
    case 0: case 1: // ns / μs
        p.starting_delay = txt(m_w.start_delay_n) > 999
            ? txt(m_w.start_delay_n)
            : txt(m_w.start_delay_u) * 1000 + txt(m_w.start_delay_n);
        p.ending_delay = txt(m_w.end_delay_n) > 999
            ? txt(m_w.end_delay_n)
            : txt(m_w.end_delay_u) * 1000 + txt(m_w.end_delay_n);
        p.starting_gw = txt(m_w.start_gw_n) > 999
            ? txt(m_w.start_gw_n)
            : txt(m_w.start_gw_u) * 1000 + txt(m_w.start_gw_n);
        p.ending_gw = txt(m_w.end_gw_n) > 999
            ? txt(m_w.end_gw_n)
            : txt(m_w.end_gw_u) * 1000 + txt(m_w.end_gw_n);
        break;
    case 2: // m  (legacy uses GW_EDIT_M for both delay and gw — quirk preserved)
        p.starting_delay = txt(m_w.start_gw_m);
        p.ending_delay   = txt(m_w.end_gw_m);
        p.starting_gw    = txt(m_w.start_gw_m);
        p.ending_gw      = txt(m_w.end_gw_m);
        break;
    }

    if (m_w.rep_freq) p.rep_freq = txtf(m_w.rep_freq);

    QLineEdit *src = qobject_cast<QLineEdit*>(sender());
    if (src == m_w.step_delay) {
        p.step_size_delay = txtf(m_w.step_delay);
        if (std::fabs(p.step_size_delay) < 1e-4f) {
            p.step_size_delay = 0;
            p.frame_count = 1;
        } else {
            p.step_size_gw  = 1.0f * (p.ending_gw - p.starting_gw) / (p.frame_count - 1);
            p.frame_count   = 1.0f * (p.ending_delay - p.starting_delay) / p.step_size_delay + 1;
        }
    }
    else if (src == m_w.step_gw) {
        p.step_size_gw = txtf(m_w.step_gw);
        if (std::fabs(p.step_size_gw) < 1e-4f) {
            p.frame_count = 1;
            p.step_size_gw = 0;
        } else {
            p.frame_count     = 1.0f * (p.ending_delay - p.starting_delay) / p.step_size_delay + 1;
            p.step_size_delay = 1.0f * (p.ending_delay - p.starting_delay) / (p.frame_count - 1);
        }
    }
    else {
        p.frame_count = txt(m_w.frame_count);
        if (p.frame_count <= 1) {
            p.frame_count = 1;
            p.step_size_delay = 0;
            p.step_size_gw    = 0;
        } else {
            p.step_size_delay = 1.0f * (p.ending_delay - p.starting_delay) / (p.frame_count - 1);
            p.step_size_gw    = 1.0f * (p.ending_gw    - p.starting_gw)    / (p.frame_count - 1);
            if (std::fabs(p.step_size_delay) < 1e-4f) p.step_size_delay = 1;
            if (std::fabs(p.step_size_gw)    < 1e-4f) p.step_size_gw    = 1;
        }
    }

    if (m_w.frame_count) m_w.frame_count->setText(QString::number(p.frame_count));
    const bool m_mode = (p.base_unit == 2);
    if (m_w.step_delay) m_w.step_delay->setText(QString::number(
        m_mode ? p.step_size_delay * p.dist_ns : p.step_size_delay, 'f', 2));
    if (m_w.step_gw)    m_w.step_gw->setText(QString::number(
        m_mode ? p.step_size_gw * p.dist_ns : p.step_size_gw, 'f', 2));
}

void ScanOverlay::on_ptz_edit()
{
    if (!m_ctrl) return;
    ScanParams &p = m_ctrl->params();
    auto txtf = [](QLineEdit *e) { return e ? e->text().toFloat() : 0.f; };
    auto txti = [](QLineEdit *e) { return e ? e->text().toInt()   : 0; };

    p.starting_h = txtf(m_w.start_h);
    p.starting_v = txtf(m_w.start_v);
    p.ending_h   = txtf(m_w.end_h);
    p.ending_v   = txtf(m_w.end_v);

    QLineEdit *src = qobject_cast<QLineEdit*>(sender());
    if (src == m_w.step_h) {
        p.step_h = txtf(m_w.step_h);
        if (p.step_h == 0) { p.count_h = 1; p.step_h = 0; }
        else {
            p.count_h = std::ceil(1.0f * (p.ending_h - p.starting_h) / p.step_h) + 1;
            p.step_h  = 1.0f * (p.ending_h - p.starting_h) / (p.count_h - 1);
        }
        if (m_w.step_h)  m_w.step_h->setText(QString::number(p.step_h, 'f', 2));
        if (m_w.frame_h) m_w.frame_h->setText(QString::number(p.count_h));
    }
    else if (src == m_w.step_v) {
        p.step_v = txtf(m_w.step_v);
        if (p.step_v == 0) { p.count_v = 1; p.step_v = 0; }
        else {
            p.count_v = std::ceil(1.0f * (p.ending_v - p.starting_v) / p.step_v) + 1;
            p.step_v  = 1.0f * (p.ending_v - p.starting_v) / (p.count_v - 1);
        }
        if (m_w.step_v)  m_w.step_v->setText(QString::number(p.step_v, 'f', 2));
        if (m_w.frame_v) m_w.frame_v->setText(QString::number(p.count_v));
    }
    else if (src == m_w.frame_h || src == m_w.start_h || src == m_w.end_h) {
        p.count_h = txti(m_w.frame_h);
        if (p.count_h <= 1) { p.count_h = 1; p.step_h = 0; }
        else { p.step_h = 1.0f * (p.ending_h - p.starting_h) / (p.count_h - 1); }
        if (m_w.step_h) m_w.step_h->setText(QString::number(p.step_h, 'f', 2));
    }
    else if (src == m_w.frame_v || src == m_w.start_v || src == m_w.end_v) {
        p.count_v = txti(m_w.frame_v);
        if (p.count_v <= 1) { p.count_v = 1; p.step_v = 0; }
        else { p.step_v = 1.0f * (p.ending_v - p.starting_v) / (p.count_v - 1); }
        if (m_w.step_v) m_w.step_v->setText(QString::number(p.step_v, 'f', 2));
    }
}

void ScanOverlay::on_capture_ori(int s) { if (m_ctrl) m_ctrl->params().capture_scan_ori = s; }
void ScanOverlay::on_capture_res(int s) { if (m_ctrl) m_ctrl->params().capture_scan_res = s; }
void ScanOverlay::on_record_ori (int s) { if (m_ctrl) m_ctrl->params().record_scan_ori  = s; }
void ScanOverlay::on_record_res (int s) { if (m_ctrl) m_ctrl->params().record_scan_res  = s; }

void ScanOverlay::on_dir_n()
{
    if (m_w.dir_z) m_w.dir_z->setChecked(false);
    if (m_ctrl) m_ctrl->params().ptz_direction = 0;
}

void ScanOverlay::on_dir_z()
{
    if (m_w.dir_n) m_w.dir_n->setChecked(false);
    if (m_ctrl) m_ctrl->params().ptz_direction = 1;
}

void ScanOverlay::on_ptz_wait_edit()
{
    if (!m_ctrl || !m_w.ptz_wait) return;
    int v = m_w.ptz_wait->text().toInt();
    if (v < 0)  v = 0;
    if (v > 20) v = 20;
    m_ctrl->params().ptz_wait_time = v;
    m_w.ptz_wait->setText(QString::number(v));
}

void ScanOverlay::on_num_single_pos_edit()
{
    if (!m_ctrl || !m_w.num_single_pos) return;
    int v = m_w.num_single_pos->text().toInt();
    if (v < 1)  v = 1;
    if (v > 20) v = 20;
    m_ctrl->params().num_single_pos = v;
    m_w.num_single_pos->setText(QString::number(v));
}

void ScanOverlay::on_mode_grid()
{
    if (m_w.mode_panorama) m_w.mode_panorama->setChecked(false);
    if (m_ctrl) m_ctrl->params().ptz_scan_mode = ScanParams::PTZ_SCAN_GRID;
    update_mode_enabled();
}

void ScanOverlay::on_mode_panorama()
{
    if (m_w.mode_grid) m_w.mode_grid->setChecked(false);
    if (m_ctrl) m_ctrl->params().ptz_scan_mode = ScanParams::PTZ_SCAN_PANORAMA;
    update_mode_enabled();
}

void ScanOverlay::on_grid_section_toggled()
{
    if (!m_ctrl) return;

    bool delay_enabled = m_w.enable_delay && m_w.enable_delay->isChecked();
    bool gate_enabled = m_w.enable_gate && m_w.enable_gate->isChecked();
    bool ptz_enabled = m_w.enable_ptz && m_w.enable_ptz->isChecked();
    if (!delay_enabled && !gate_enabled && !ptz_enabled) {
        QCheckBox *changed = qobject_cast<QCheckBox*>(sender());
        if (changed) {
            QSignalBlocker blocker(changed);
            changed->setChecked(true);
        }
        delay_enabled = m_w.enable_delay && m_w.enable_delay->isChecked();
        gate_enabled = m_w.enable_gate && m_w.enable_gate->isChecked();
        ptz_enabled = m_w.enable_ptz && m_w.enable_ptz->isChecked();
    }

    ScanParams &p = m_ctrl->params();
    p.scan_delay_enabled = delay_enabled;
    p.scan_gate_width_enabled = gate_enabled;
    p.scan_ptz_enabled = ptz_enabled;
    update_mode_enabled();
}

void ScanOverlay::on_panorama_edit()
{
    if (!m_ctrl) return;

    ScanParams &p = m_ctrl->params();
    auto txtf = [](QLineEdit *e) { return e ? e->text().toFloat() : 0.f; };
    auto txti = [](QLineEdit *e) { return e ? e->text().toInt()   : 0; };
    auto setF = [](QLineEdit *e, float v, int prec = 2) {
        if (e) e->setText(QString::number(v, 'f', prec));
    };

    p.panorama_start_h = txtf(m_w.panorama_start_h);
    p.panorama_use_end_h = m_w.panorama_use_end_h ? m_w.panorama_use_end_h->isChecked() : false;
    p.panorama_end_h = txtf(m_w.panorama_end_h);
    p.panorama_fixed_v = txtf(m_w.panorama_fixed_v);
    p.panorama_wait_time = clamp_int(txti(m_w.panorama_wait), 0, 20);
    p.panorama_zoom_level = txtf(m_w.panorama_zoom);
    if (p.panorama_zoom_level <= 0.0f) p.panorama_zoom_level = 1.0f;
    update_panorama_fov_from_zoom(p);
    p.panorama_fusion_mode = m_w.panorama_fusion
        ? clamp_int(m_w.panorama_fusion->currentIndex(), 0, 2)
        : ScanParams::PANORAMA_FUSION_GAIN;

    QLineEdit *src = qobject_cast<QLineEdit*>(sender());
    int count = txti(m_w.panorama_count);
    float step = txtf(m_w.panorama_step_h);

    if (src == m_w.panorama_step_h) {
        step = std::fabs(step);
        if (step < 1e-4f) step = p.panorama_fov_h > 0.0f ? p.panorama_fov_h : 360.0f;
        p.panorama_step_h = step;
        count = get_panorama_stop_count(p);
    } else if (src == m_w.panorama_count) {
        count = clamp_int(count, 1, kMaxPanoramaStops);
        const float sweep = get_panorama_sweep_degrees(p);
        const int segments = p.panorama_use_end_h ? qMax(1, count - 1) : count;
        p.panorama_step_h = sweep / static_cast<float>(segments);
    } else {
        if (src == m_w.panorama_zoom) {
            p.panorama_step_h = p.panorama_fov_h > 0.0f ? p.panorama_fov_h : p.panorama_step_h;
        } else {
            p.panorama_step_h = std::fabs(step);
            if (p.panorama_step_h < 1e-4f) {
                p.panorama_step_h = p.panorama_fov_h > 0.0f ? p.panorama_fov_h : 360.0f;
            }
        }
        count = get_panorama_stop_count(p);
    }

    p.panorama_count = count;

    setF(m_w.panorama_start_h, p.panorama_start_h);
    if (m_w.panorama_use_end_h) m_w.panorama_use_end_h->setChecked(p.panorama_use_end_h);
    setF(m_w.panorama_end_h, p.panorama_end_h);
    setF(m_w.panorama_fixed_v, p.panorama_fixed_v);
    setF(m_w.panorama_step_h, p.panorama_step_h);
    if (m_w.panorama_count) m_w.panorama_count->setText(QString::number(p.panorama_count));
    if (m_w.panorama_wait) m_w.panorama_wait->setText(QString::number(p.panorama_wait_time));
    setF(m_w.panorama_zoom, p.panorama_zoom_level, 2);
    if (m_w.panorama_fusion && m_w.panorama_fusion->currentIndex() != p.panorama_fusion_mode) {
        QSignalBlocker block(m_w.panorama_fusion);
        m_w.panorama_fusion->setCurrentIndex(p.panorama_fusion_mode);
    }
    if (m_w.panorama_end_h) m_w.panorama_end_h->setEnabled(p.panorama_use_end_h);
}

void ScanOverlay::on_panorama_cw()
{
    if (m_w.panorama_ccw) m_w.panorama_ccw->setChecked(false);
    if (m_ctrl) m_ctrl->params().panorama_direction = 0;
    on_panorama_edit();
}

void ScanOverlay::on_panorama_ccw()
{
    if (m_w.panorama_cw) m_w.panorama_cw->setChecked(false);
    if (m_ctrl) m_ctrl->params().panorama_direction = 1;
    on_panorama_edit();
}

void ScanOverlay::update_mode_enabled()
{
    if (!m_ctrl) return;

    const ScanParams &p = m_ctrl->params();
    const bool grid_mode = (p.ptz_scan_mode == ScanParams::PTZ_SCAN_GRID);
    const bool panorama_mode = !grid_mode;

    auto set_enabled = [](QWidget *w, bool enabled) {
        if (w) w->setEnabled(enabled);
    };

    set_enabled(m_w.grid_delay_card, grid_mode);
    set_enabled(m_w.grid_gate_card, grid_mode);
    set_enabled(m_w.grid_ptz_card, grid_mode);
    set_enabled(m_w.grid_capture_card, grid_mode);
    set_enabled(m_w.panorama_card, panorama_mode);
    set_enabled(m_w.grid_delay_body, grid_mode && p.scan_delay_enabled);
    set_enabled(m_w.grid_gate_body, grid_mode && p.scan_gate_width_enabled);
    set_enabled(m_w.grid_ptz_body, grid_mode && p.scan_ptz_enabled);

    if (m_w.mode_grid) m_w.mode_grid->setEnabled(true);
    if (m_w.mode_panorama) m_w.mode_panorama->setEnabled(true);
    if (m_w.panorama_end_h) m_w.panorama_end_h->setEnabled(panorama_mode && m_ctrl->params().panorama_use_end_h);
}
