#include "controller/tcucontroller.h"
#include "controller/devicemanager.h"
#include "visual/preferences.h"
#include "visual/scanconfig.h"
#include "visual/aliasing.h"
#include "ui_user_panel.h"
#include "ui_preferences.h"
#include "util/constants.h"

#include <QElapsedTimer>
#include <QSlider>
#include <QInputDialog>
#include <cmath>

static bool throttle_check(QElapsedTimer &t)
{
    if (t.isValid() && t.elapsed() < THROTTLE_MS) return true;
    t.restart();
    return false;
}

TCUController::TCUController(Config *config, DeviceManager *device_mgr, QObject *parent)
    : QObject(parent),
      m_config(config),
      m_device_mgr(device_mgr),
      m_ui(nullptr),
      m_pref(nullptr),
      m_scan_config(nullptr),
      m_aliasing(nullptr),
      stepping(10),
      hz_unit(0),
      base_unit(0),
      rep_freq(30),
      laser_width(40),
      delay_dist(15),
      depth_of_view(6),
      mcp_max(MAX_MCP_8BIT),
      aliasing_mode(false),
      aliasing_level(0),
      c(3e8),
      dist_ns(0),
      auto_mcp(false),
      frame_a_3d(false),
      distance(0)
{
    dist_ns = c * 1e-9 / 2;
}

void TCUController::init(Ui::UserPanel *ui, Preferences *pref, ScanConfig *scan_config, Aliasing *aliasing)
{
    m_ui = ui;
    m_pref = pref;
    m_scan_config = scan_config;
    m_aliasing = aliasing;
}

// --- Preferences slots ---

void TCUController::setup_hz(int hz_unit)
{
    if (aliasing_mode) return;
    this->hz_unit = hz_unit;
    switch (hz_unit) {
    case 0: m_ui->FREQ_UNIT->setText("kHz"); m_ui->FREQ_EDIT->setText(QString::number(m_device_mgr->tcu()->get(TCU::REPEATED_FREQ), 'f', 2)); break;
    case 1: m_ui->FREQ_UNIT->setText("Hz"); m_ui->FREQ_EDIT->setText(QString::number((int)(m_device_mgr->tcu()->get(TCU::REPEATED_FREQ) * 1000))); break;
    default: break;
    }
}

void TCUController::setup_stepping(int base_unit)
{
    this->base_unit = base_unit;
    switch (base_unit) {
    case 0: m_ui->STEPPING_UNIT->setText("ns"); m_ui->STEPPING_EDIT->setText(QString::number(stepping, 'f', 2)); break;
    case 1: m_ui->STEPPING_UNIT->setText("μs"); m_ui->STEPPING_EDIT->setText(QString::number(stepping / 1000, 'f', 2)); break;
    case 2: m_ui->STEPPING_UNIT->setText("m"); m_ui->STEPPING_EDIT->setText(QString::number(stepping * dist_ns, 'f', 2)); break;
    default: break;
    }
}

void TCUController::setup_max_dist(float max_dist)
{
    m_ui->DELAY_SLIDER->setMaximum(max_dist);
}

void TCUController::setup_max_dov(float max_dov)
{
    m_ui->GW_SLIDER->setMaximum(max_dov);
}

void TCUController::update_delay_offset(float dist_offset)
{
    emit send_double_tcu_msg(TCU::OFFSET_DELAY, dist_offset / dist_ns);
}

void TCUController::update_gate_width_offset(float dov_offset)
{
    emit send_double_tcu_msg(TCU::OFFSET_GW, dov_offset / dist_ns);
}

void TCUController::update_laser_offset(float laser_offset)
{
    emit send_double_tcu_msg(TCU::OFFSET_LASER, laser_offset);
}

void TCUController::setup_laser(int laser_on)
{
    emit send_uint_tcu_msg(TCU::LASER_ON, laser_on);
}

void TCUController::set_tcu_type(int idx)
{
    int diff = 0;
    if (idx == 2) update_tcu_param_pos(1, m_ui->LASER_WIDTH_UNIT_U, m_ui->LASER_WIDTH_EDIT_N, m_ui->LASER_WIDTH_UNIT_N, m_ui->LASER_WIDTH_EDIT_P);
    if (m_device_mgr->tcu()->type() == 2) update_tcu_param_pos(-1, m_ui->LASER_WIDTH_UNIT_U, m_ui->LASER_WIDTH_EDIT_N, m_ui->LASER_WIDTH_UNIT_N, m_ui->LASER_WIDTH_EDIT_P);

    if (m_device_mgr->tcu()->type() == 0 && idx) diff = 1;
    if (m_device_mgr->tcu()->type() && idx == 0) diff = -1;
    m_device_mgr->tcu()->set_type(idx);
    m_ui->DELAY_A->resize(QSize(m_ui->DELAY_A->width() + 12 * diff, m_ui->DELAY_A->height()));
    m_ui->DELAY_B->resize(QSize(m_ui->DELAY_B->width() + 12 * diff, m_ui->DELAY_B->height()));
    m_ui->DELAY_GRP->move(idx ? -6 : 0, 100);
    m_ui->GATE_WIDTH_A->resize(QSize(m_ui->GATE_WIDTH_A->width() + 12 * diff, m_ui->GATE_WIDTH_A->height()));
    m_ui->GATE_WIDTH_B->resize(QSize(m_ui->GATE_WIDTH_B->width() + 12 * diff, m_ui->GATE_WIDTH_B->height()));
    m_ui->GATE_WIDTH_GRP->move(idx ? -6 : 0, 180);
    if (diff) update_tcu_param_pos(diff, m_ui->DELAY_A_UNIT_U, m_ui->DELAY_A_EDIT_N, m_ui->DELAY_A_UNIT_N, m_ui->DELAY_A_EDIT_P);
    if (diff) update_tcu_param_pos(diff, m_ui->DELAY_B_UNIT_U, m_ui->DELAY_B_EDIT_N, m_ui->DELAY_B_UNIT_N, m_ui->DELAY_B_EDIT_P);
    if (diff) update_tcu_param_pos(diff, m_ui->GATE_WIDTH_A_UNIT_U, m_ui->GATE_WIDTH_A_EDIT_N, m_ui->GATE_WIDTH_A_UNIT_N, m_ui->GATE_WIDTH_A_EDIT_P);
    if (diff) update_tcu_param_pos(diff, m_ui->GATE_WIDTH_B_UNIT_U, m_ui->GATE_WIDTH_B_EDIT_N, m_ui->GATE_WIDTH_B_UNIT_N, m_ui->GATE_WIDTH_B_EDIT_P);

    switch (idx) {
    case 0: mcp_max = MAX_MCP_8BIT; break;
    case 1: mcp_max = MAX_MCP_12BIT; break;
    case 2: mcp_max = MAX_MCP_12BIT; break;
    default: break;
    }
    m_ui->MCP_SLIDER->setMaximum(mcp_max);
}

void TCUController::update_ps_config(bool read, int idx, uint val)
{
    if (read) {
        m_pref->ui->PS_STEPPING_EDT->setText(QString::number(m_device_mgr->tcu()->get(TCU::PS_STEP_1 + idx)));
        m_pref->ui->MAX_PS_STEP_EDT->setText(QString::number(int(std::round(4000. / m_device_mgr->tcu()->get(TCU::PS_STEP_1 + idx)))));
    }
    else emit send_uint_tcu_msg(TCU::PS_STEP_1 + idx, val);
}

void TCUController::set_auto_mcp_slot(bool auto_mcp)
{
    this->auto_mcp = auto_mcp;
    m_ui->AUTO_MCP_CHK->setChecked(auto_mcp);
}

// --- Aliasing slot ---

void TCUController::set_distance_set(int id)
{
    // FIXME: aliasing parameter application temporary banned
    Q_UNUSED(id);
    return;

    aliasing_mode = true;
    struct AliasingData temp = m_aliasing->retrieve_data(id);
    aliasing_level = temp.num_period;
    emit send_double_tcu_msg(TCU::REPEATED_FREQ, temp.rep_freq);
    if (m_pref->ui->AUTO_REP_FREQ_CHK->isChecked()) m_pref->ui->AUTO_REP_FREQ_CHK->click();
    emit send_double_tcu_msg(TCU::EST_DIST, temp.distance);
    update_delay();
}

// --- TCU param update (from port feedback) ---

void TCUController::update_tcu_params(qint32 tcu_param)
{
    uint us, ns, ps;
    switch (tcu_param)
    {
        case TCU::LASER_USR:
            split_value_by_unit(m_device_mgr->tcu()->get(TCU::LASER_WIDTH), us, ns, ps);
            m_ui->LASER_WIDTH_EDIT_U->setText(QString::asprintf(  "%d", us));
            m_ui->LASER_WIDTH_EDIT_N->setText(QString::asprintf("%03d", ns));
            m_ui->LASER_WIDTH_EDIT_P->setText(QString::asprintf("%03d", ps));
            break;
        case TCU::EST_DIST:
            delay_dist.store(m_device_mgr->tcu()->get(TCU::EST_DIST));
            m_ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist.load()));
            rep_freq = m_device_mgr->tcu()->get(TCU::REPEATED_FREQ);
            setup_hz(hz_unit);
            split_value_by_unit(m_device_mgr->tcu()->get(TCU::DELAY_A), us, ns, ps, 1);
            m_ui->DELAY_A_EDIT_U->setText(QString::asprintf(  "%d", us));
            m_ui->DELAY_A_EDIT_N->setText(QString::asprintf("%03d", ns));
            m_ui->DELAY_A_EDIT_P->setText(QString::asprintf("%03d", ps));
            split_value_by_unit(m_device_mgr->tcu()->get(TCU::DELAY_B), us, ns, ps, 1);
            m_ui->DELAY_B_EDIT_U->setText(QString::asprintf(  "%d", us));
            m_ui->DELAY_B_EDIT_N->setText(QString::asprintf("%03d", ns));
            m_ui->DELAY_B_EDIT_P->setText(QString::asprintf("%03d", ps));
            m_ui->DELAY_SLIDER->setValue(delay_dist.load());
            break;
        case TCU::EST_DOV:
            m_ui->GATE_WIDTH->setText(QString::asprintf("%.2f m", depth_of_view = m_device_mgr->tcu()->get(TCU::EST_DOV)));
            split_value_by_unit(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A), us, ns, ps, 0);
            m_ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf(  "%d", us));
            m_ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%03d", ns));
            m_ui->GATE_WIDTH_A_EDIT_P->setText(QString::asprintf("%03d", ps));
            split_value_by_unit(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_B), us, ns, ps, 0);
            m_ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf(  "%d", us));
            m_ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%03d", ns));
            m_ui->GATE_WIDTH_B_EDIT_P->setText(QString::asprintf("%03d", ps));
            m_ui->GW_SLIDER->setValue(depth_of_view);
            break;
        case TCU::NO_PARAM:
            update_tcu_params(TCU::LASER_USR);
            update_tcu_params(TCU::EST_DIST);
            update_tcu_params(TCU::EST_DOV);
            m_ui->MCP_SLIDER->setValue(m_device_mgr->tcu()->get(TCU::MCP));
        default: break;
    }
}

// --- Slider-controlled ---

void TCUController::change_mcp(int val)
{
    if (val < 0) val = 0;
    if (val > mcp_max) val = mcp_max;
    m_ui->MCP_EDIT->setText(QString::number(val));
    m_ui->MCP_SLIDER->setValue(val);

    if (val == (int)std::round(m_device_mgr->tcu()->get(TCU::MCP))) return;

    emit send_uint_tcu_msg(TCU::MCP, val);
}

void TCUController::change_delay(int val)
{
    if (abs(delay_dist.load() - val) < 1) return;
    delay_dist.store(val);
    update_delay();
}

void TCUController::change_gatewidth(int val)
{
    if (abs(depth_of_view - val) < 1) return;
    depth_of_view = val;
    update_gate_width();
}

// --- apply_distance (called by UserPanel::on_DIST_BTN_clicked) ---

void TCUController::apply_distance(int dist)
{
    distance = dist;
    if (distance < 100) distance = 100;
    m_ui->DISTANCE->setText(QString::asprintf("%d m", distance));

    delay_dist.store(distance);
    update_delay();

    rep_freq = 1e6 / (delay_dist.load() / dist_ns + depth_of_view / dist_ns + 1000);
    if (rep_freq > 30) rep_freq = 30;
    if      (distance < 1000) depth_of_view =  500 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else if (distance < 3000) depth_of_view = 1000 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else if (distance < 6000) depth_of_view = 2000 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else                      depth_of_view = 3500 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));

    emit send_uint_tcu_msg(TCU::DIST, distance);
    m_ui->EST_DIST->setText(QString::asprintf("%.2f m", delay_dist.load()));

    setup_hz(hz_unit);
    uint us, ns, ps;
    split_value_by_unit(m_device_mgr->tcu()->get(TCU::LASER_WIDTH), us, ns, ps);
    m_ui->LASER_WIDTH_EDIT_U->setText(QString::asprintf(  "%d", us));
    m_ui->LASER_WIDTH_EDIT_N->setText(QString::asprintf("%03d", ns));
    m_ui->LASER_WIDTH_EDIT_P->setText(QString::asprintf("%03d", ps));
    split_value_by_unit(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A), us, ns, ps, 0);
    m_ui->GATE_WIDTH_A_EDIT_U->setText(QString::asprintf(  "%d", us));
    m_ui->GATE_WIDTH_A_EDIT_N->setText(QString::asprintf("%03d", ns));
    m_ui->GATE_WIDTH_A_EDIT_P->setText(QString::asprintf("%03d", ps));
    split_value_by_unit(m_device_mgr->tcu()->get(TCU::DELAY_A), us, ns, ps, 1);
    m_ui->DELAY_A_EDIT_U->setText(QString::asprintf(  "%d", us));
    m_ui->DELAY_A_EDIT_N->setText(QString::asprintf("%03d", ns));
    m_ui->DELAY_A_EDIT_P->setText(QString::asprintf("%03d", ps));
    split_value_by_unit(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_B), us, ns, ps, 2);
    m_ui->GATE_WIDTH_B_EDIT_U->setText(QString::asprintf(  "%d", us));
    m_ui->GATE_WIDTH_B_EDIT_N->setText(QString::asprintf("%03d", ns));
    m_ui->GATE_WIDTH_B_EDIT_P->setText(QString::asprintf("%03d", ps));
    split_value_by_unit(m_device_mgr->tcu()->get(TCU::DELAY_B), us, ns, ps, 3);
    m_ui->DELAY_B_EDIT_U->setText(QString::asprintf(  "%d", us));
    m_ui->DELAY_B_EDIT_N->setText(QString::asprintf("%03d", ns));
    m_ui->DELAY_B_EDIT_P->setText(QString::asprintf("%03d", ps));
}

// --- UI button slots ---

void TCUController::on_SWITCH_TCU_UI_BTN_clicked()
{
    static int show_diff = 1;
    if (show_diff) {
        m_ui->DELAY_B_GRP->hide();
        m_ui->GATE_WIDTH_B_GRP->hide();
        m_ui->DELAY_DIFF_GRP->show();
        m_ui->GATE_WIDTH_DIFF_GRP->show();
    }
    else {
        m_ui->DELAY_B_GRP->show();
        m_ui->GATE_WIDTH_B_GRP->show();
        m_ui->DELAY_DIFF_GRP->hide();
        m_ui->GATE_WIDTH_DIFF_GRP->hide();
    }
    show_diff ^= 1;
}

void TCUController::on_SIMPLE_LASER_CHK_clicked()
{
#ifdef LVTONG
    if (!qobject_cast<QCheckBox *>(sender())) return;
    m_ui->FIRE_LASER_BTN->click();
#else //LVTONG
    m_pref->ui->LASER_CHK_1->click();
#endif
}

void TCUController::on_AUTO_MCP_CHK_clicked()
{
    m_pref->ui->AUTO_MCP_CHK->click();
}

// --- Internal update ---

void TCUController::update_laser_width()
{
#ifndef LVTONG
    static QElapsedTimer t;
    if (throttle_check(t)) return;
#endif
    laser_width = qBound(0.0f, laser_width, m_config->get_data().tcu.max_laser_width);
    emit send_double_tcu_msg(TCU::LASER_USR, laser_width);
}

void TCUController::update_delay()
{
    static QElapsedTimer t;
    if (throttle_check(t)) return;

    delay_dist.store(qBound(0.0f, delay_dist.load(), m_config->get_data().tcu.max_dist));

    if (!aliasing_mode) {
        if (!qobject_cast<QSlider*>(sender())) m_ui->DELAY_SLIDER->setValue(delay_dist.load());
        emit send_double_tcu_msg(TCU::EST_DIST, delay_dist.load());
    }
    else {
        aliasing_mode = false;
        emit send_double_tcu_msg(TCU::EST_DIST, m_device_mgr->tcu()->get(TCU::EST_DIST));
    }
}

void TCUController::update_gate_width() {
#ifndef LVTONG
    static QElapsedTimer t;
    if (throttle_check(t)) return;
#endif
    depth_of_view = qBound(0.0f, depth_of_view, m_config->get_data().tcu.max_dov);

    if (!qobject_cast<QSlider*>(sender())) m_ui->GW_SLIDER->setValue(depth_of_view);

    emit send_double_tcu_msg(TCU::EST_DOV, depth_of_view);
}

void TCUController::update_light_speed(bool uw)
{
    c = uw ? 3e8 * 0.75 : 3e8;
    m_scan_config->dist_ns = m_pref->dist_ns = dist_ns = c * 1e-9 / 2;
    m_pref->update_distance_display();
    emit send_double_tcu_msg(TCU::LIGHT_SPEED, dist_ns);
    update_delay();
    update_gate_width();
}

void TCUController::update_current()
{
    QString send = QString::asprintf("PCUR %.2f\r", m_ui->CURRENT_EDIT->text().toFloat());
    emit send_laser_msg(send);
    qDebug() << send;
}

// --- convert_to_send_tcu ---

QByteArray TCUController::convert_to_send_tcu(uchar num, unsigned int send) {
    QByteArray out(7, 0x00);
    out[0] = 0x88;
    out[1] = num;
    out[6] = 0x99;

    out[5] = send & 0xFF; send >>= 8;
    out[4] = send & 0xFF; send >>= 8;
    out[3] = send & 0xFF; send >>= 8;
    out[2] = send & 0xFF;
    return out;
}

// --- Private helpers ---

inline void TCUController::update_tcu_param_pos(int dir, QLabel *u_unit, QLineEdit *n_input, QLabel *n_unit, QLineEdit *p_input)
{
    switch (dir) {
        case -1:
            u_unit->setText("μs");
            n_unit->setText("ns");
            u_unit->move(u_unit->pos() + QPoint(3, 0));
            n_input->move(n_input->pos() + QPoint(14, 0));
            n_unit->move(n_unit->pos() + QPoint(17, 0));
            p_input->hide();
            break;
        case 1:
            u_unit->setText(",");
            n_unit->setText(".");
            u_unit->move(u_unit->pos() - QPoint(3, 0));
            n_input->move(n_input->pos() - QPoint(14, 0));
            n_unit->move(n_unit->pos() - QPoint(17, 0));
            p_input->show();
            break;
        default: break;
    }
}

inline void TCUController::split_value_by_unit(float val, uint &us, uint &ns, uint &ps, int idx) {
    static float ONE = 1.f;
    switch (m_device_mgr->tcu()->type())
    {
    case 0:
    case 1:
    {
        us = uint(val + 0.001) / 1000;
        ns = uint(val + 0.001) % 1000;
        if (idx < 0) ps = 0;
        else         ps = std::round(std::modf(val + 0.001, &ONE) * 1000 / m_device_mgr->tcu()->get(TCU::PS_STEP_1 + idx)) * m_device_mgr->tcu()->get(TCU::PS_STEP_1 + idx);
        while (ps > 999) ps -= 1000, ns++;
        while (ns > 999) ns -= 1000, us++;
        break;
    }
    case 2:
    {
        double step = 14.9;
        if (idx == 1) step = 11.23;
        us = uint(val + 0.001) / 1000;
        ns = uint(val + 0.001) % 1000;
        ps = std::round(std::modf(val + 0.001, &ONE) * 1000 / step) * step;
        while (ps > 999) ps -= 1000, ns++;
        while (ns > 999) ns -= 1000, us++;
        break;
    }
    default: break;
    }
}
