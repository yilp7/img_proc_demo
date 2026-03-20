#include "controller/tcucontroller.h"
#include "controller/devicemanager.h"
#include "visual/scanconfig.h"
#include "visual/aliasing.h"
#include "util/constants.h"

#include <QElapsedTimer>
#include <QSlider>
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

void TCUController::init(ScanConfig *scan_config, Aliasing *aliasing)
{
    m_scan_config = scan_config;
    m_aliasing = aliasing;
}

// --- Preferences slots ---

void TCUController::setup_hz(int hz_unit)
{
    if (aliasing_mode) return;
    this->hz_unit = hz_unit;
    switch (hz_unit) {
    case 0: emit freq_display_updated("kHz", QString::number(m_device_mgr->tcu()->get(TCU::REPEATED_FREQ), 'f', 2)); break;
    case 1: emit freq_display_updated("Hz", QString::number((int)(m_device_mgr->tcu()->get(TCU::REPEATED_FREQ) * 1000))); break;
    default: break;
    }
}

void TCUController::setup_stepping(int base_unit)
{
    this->base_unit = base_unit;
    switch (base_unit) {
    case 0: emit stepping_display_updated("ns", QString::number(stepping, 'f', 2)); break;
    case 1: emit stepping_display_updated("μs", QString::number(stepping / 1000, 'f', 2)); break;
    case 2: emit stepping_display_updated("m", QString::number(stepping * dist_ns, 'f', 2)); break;
    default: break;
    }
}

void TCUController::setup_max_dist(float max_dist)
{
    emit delay_slider_max_changed(max_dist);
}

void TCUController::setup_max_dov(float max_dov)
{
    emit gw_slider_max_changed(max_dov);
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
    if (m_device_mgr->tcu()->type() == 0 && idx) diff = 1;
    if (m_device_mgr->tcu()->type() && idx == 0) diff = -1;
    m_device_mgr->tcu()->set_type(idx);

    switch (idx) {
    case 0: mcp_max = MAX_MCP_8BIT; break;
    case 1: mcp_max = MAX_MCP_12BIT; break;
    case 2: mcp_max = MAX_MCP_12BIT; break;
    default: break;
    }

    emit tcu_type_layout_changed(idx, diff);
    emit mcp_slider_max_changed(mcp_max);
}

void TCUController::update_ps_config(bool read, int idx, uint val)
{
    if (read) {
        float ps_step = m_device_mgr->tcu()->get(TCU::PS_STEP_1 + idx);
        emit ps_config_display_updated(
            QString::number(ps_step),
            QString::number(int(std::round(4000. / ps_step))));
    }
    else emit send_uint_tcu_msg(TCU::PS_STEP_1 + idx, val);
}

void TCUController::set_auto_mcp_slot(bool auto_mcp)
{
    this->auto_mcp = auto_mcp;
    emit auto_mcp_chk_changed(auto_mcp);
}

// --- Aliasing slot ---

void TCUController::set_distance_set(int id)
{
    // FIXME: aliasing parameter application temporary banned
    Q_UNUSED(id);
    return;
}

// --- TCU param update (from port feedback) ---

void TCUController::update_tcu_params(qint32 tcu_param)
{
    uint us, ns, ps;
    switch (tcu_param)
    {
        case TCU::LASER_USR:
            split_value_by_unit(m_device_mgr->tcu()->get(TCU::LASER_WIDTH), us, ns, ps);
            emit laser_width_display_updated(
                QString::asprintf(  "%d", us),
                QString::asprintf("%03d", ns),
                QString::asprintf("%03d", ps));
            break;
        case TCU::EST_DIST:
            delay_dist.store(m_device_mgr->tcu()->get(TCU::EST_DIST));
            emit est_dist_text_updated(QString::asprintf("%.2f m", delay_dist.load()));
            rep_freq = m_device_mgr->tcu()->get(TCU::REPEATED_FREQ);
            setup_hz(hz_unit);
            {
                uint au, an, ap, bu, bn, bp;
                split_value_by_unit(m_device_mgr->tcu()->get(TCU::DELAY_A), au, an, ap, 1);
                split_value_by_unit(m_device_mgr->tcu()->get(TCU::DELAY_B), bu, bn, bp, 1);
                emit delay_display_updated(
                    QString::asprintf(  "%d", au), QString::asprintf("%03d", an), QString::asprintf("%03d", ap),
                    QString::asprintf(  "%d", bu), QString::asprintf("%03d", bn), QString::asprintf("%03d", bp));
            }
            emit delay_slider_value_changed(delay_dist.load());
            break;
        case TCU::EST_DOV:
            depth_of_view = m_device_mgr->tcu()->get(TCU::EST_DOV);
            {
                uint au, an, ap, bu, bn, bp;
                split_value_by_unit(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A), au, an, ap, 0);
                split_value_by_unit(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_B), bu, bn, bp, 0);
                emit gw_display_updated(
                    QString::asprintf(  "%d", au), QString::asprintf("%03d", an), QString::asprintf("%03d", ap),
                    QString::asprintf(  "%d", bu), QString::asprintf("%03d", bn), QString::asprintf("%03d", bp),
                    QString::asprintf("%.2f m", depth_of_view));
            }
            emit gw_slider_value_changed(depth_of_view);
            break;
        case TCU::NO_PARAM:
            update_tcu_params(TCU::LASER_USR);
            update_tcu_params(TCU::EST_DIST);
            update_tcu_params(TCU::EST_DOV);
            emit mcp_display_updated(m_device_mgr->tcu()->get(TCU::MCP), "");
        default: break;
    }
}

// --- Slider-controlled ---

void TCUController::change_mcp(int val)
{
    if (val < 0) val = 0;
    if (val > mcp_max) val = mcp_max;
    emit mcp_display_updated(val, QString::number(val));

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
    emit distance_text_updated(QString::asprintf("%d m", distance));

    delay_dist.store(distance);
    update_delay();

    rep_freq = 1e6 / (delay_dist.load() / dist_ns + depth_of_view / dist_ns + 1000);
    if (rep_freq > 30) rep_freq = 30;
    if      (distance < 1000) depth_of_view =  500 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else if (distance < 3000) depth_of_view = 1000 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else if (distance < 6000) depth_of_view = 2000 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));
    else                      depth_of_view = 3500 * dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(depth_of_view / dist_ns));

    emit send_uint_tcu_msg(TCU::DIST, distance);
    emit est_dist_text_updated(QString::asprintf("%.2f m", delay_dist.load()));

    setup_hz(hz_unit);
    uint us, ns, ps;
    split_value_by_unit(m_device_mgr->tcu()->get(TCU::LASER_WIDTH), us, ns, ps);
    emit laser_width_display_updated(
        QString::asprintf(  "%d", us),
        QString::asprintf("%03d", ns),
        QString::asprintf("%03d", ps));

    {
        uint au, an, ap, bu, bn, bp;
        split_value_by_unit(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_A), au, an, ap, 0);
        split_value_by_unit(m_device_mgr->tcu()->get(TCU::GATE_WIDTH_B), bu, bn, bp, 2);
        emit gw_display_updated(
            QString::asprintf(  "%d", au), QString::asprintf("%03d", an), QString::asprintf("%03d", ap),
            QString::asprintf(  "%d", bu), QString::asprintf("%03d", bn), QString::asprintf("%03d", bp),
            "");
    }
    {
        uint au, an, ap, bu, bn, bp;
        split_value_by_unit(m_device_mgr->tcu()->get(TCU::DELAY_A), au, an, ap, 1);
        split_value_by_unit(m_device_mgr->tcu()->get(TCU::DELAY_B), bu, bn, bp, 3);
        emit delay_display_updated(
            QString::asprintf(  "%d", au), QString::asprintf("%03d", an), QString::asprintf("%03d", ap),
            QString::asprintf(  "%d", bu), QString::asprintf("%03d", bn), QString::asprintf("%03d", bp));
    }
}

// --- UI button slots ---

void TCUController::on_SWITCH_TCU_UI_BTN_clicked()
{
    static int show_diff = 1;
    emit tcu_diff_view_toggled(show_diff);
    show_diff ^= 1;
}

void TCUController::on_SIMPLE_LASER_CHK_clicked()
{
#ifdef LVTONG
    if (!qobject_cast<QCheckBox *>(sender())) return;
    emit fire_laser_btn_click_requested();
#else //LVTONG
    emit laser_chk_click_requested();
#endif
}

void TCUController::on_AUTO_MCP_CHK_clicked()
{
    emit auto_mcp_chk_click_requested();
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
        if (!qobject_cast<QSlider*>(sender()))
            emit delay_slider_value_changed(delay_dist.load());
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

    if (!qobject_cast<QSlider*>(sender()))
        emit gw_slider_value_changed(depth_of_view);

    emit send_double_tcu_msg(TCU::EST_DOV, depth_of_view);
}

void TCUController::update_light_speed(bool uw)
{
    c = uw ? 3e8 * 0.75 : 3e8;
    dist_ns = c * 1e-9 / 2;
    m_scan_config->dist_ns = dist_ns;
    emit dist_ns_changed(dist_ns);
    emit update_distance_display_requested();
    emit send_double_tcu_msg(TCU::LIGHT_SPEED, dist_ns);
    update_delay();
    update_gate_width();
}

void TCUController::update_current(float current_value)
{
    QString send = QString::asprintf("PCUR %.2f\r", current_value);
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
