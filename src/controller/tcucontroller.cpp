#include "controller/tcucontroller.h"
#include "controller/devicemanager.h"
#include "visual/aliasing.h"
#include "util/constants.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QSlider>
#include <cmath>

static bool throttle_check(QElapsedTimer &t)
{
    if (t.isValid() && t.elapsed() < THROTTLE_MS) return true;
    t.restart();
    return false;
}

TCUController::TCUController(Config *config, DeviceManager *device_mgr, QObject *parent) :
    QObject(parent),
    m_config(config),
    m_device_mgr(device_mgr),
    m_aliasing(nullptr),
    m_stepping(10),
    m_hz_unit(0),
    m_base_unit(0),
    m_rep_freq(30),
    m_laser_width(40),
    m_delay_dist(15),
    m_depth_of_view(6),
    m_mcp_max(MAX_MCP_8BIT),
    m_aliasing_mode(false),
    m_aliasing_level(0),
    m_c(3e8),
    m_dist_ns(0),
    m_auto_mcp(false),
    m_frame_a_3d(false),
    m_distance(0)
{
    m_dist_ns = m_c * 1e-9 / 2;

    m_delay_box.watchdog.setSingleShot(true);
    m_dov_box.watchdog.setSingleShot(true);
    connect(&m_delay_box.watchdog, &QTimer::timeout, this,
            [this] { echo_timeout(m_delay_box, TCU::EST_DIST); });
    connect(&m_dov_box.watchdog, &QTimer::timeout, this,
            [this] { echo_timeout(m_dov_box, TCU::EST_DOV); });
}

// EST_DIST and EST_DOV arrive faster than the serial link can send them.
// One command per parameter is in flight; later requests collapse into the latest
// value and go out when the port echoes the one in flight.

// Well past the longest transaction the port can take, so it only fires on a
// lost echo. Firing early costs one redundant command, nothing worse.
static constexpr int ECHO_WATCHDOG_MS = 500;

void TCUController::request(Mailbox &box, qint32 param, double val)
{
    box.desired = val;
    if (box.in_flight) {
        box.pending = true;
        return;
    }
    dispatch(box, param);
}

void TCUController::dispatch(Mailbox &box, qint32 param)
{
    box.sent = box.desired;
    box.pending = false;
    box.in_flight = true;
    box.watchdog.start(ECHO_WATCHDOG_MS);
    emit send_double_tcu_msg(param, box.sent);
}

// Only the echo of the command in flight counts: any other value is a superseded
// command or the duplicate the port emits under AB lock. With a newer request
// waiting, the echo frees the link but must not rewind the UI.
bool TCUController::accept_echo(Mailbox &box, qint32 param, double echoed)
{
    if (box.sent < 0) return true;
    if (std::abs(echoed - box.sent) > 1e-3) return false;
    box.in_flight = false;
    box.watchdog.stop();
    if (box.pending) {
        dispatch(box, param);
        return false;
    }
    return true;
}

void TCUController::echo_timeout(Mailbox &box, qint32 param)
{
    qWarning() << "TCU param" << param << "did not echo within" << ECHO_WATCHDOG_MS << "ms";
    box.in_flight = false;
    if (box.pending) dispatch(box, param);
}

// A full readback takes the device as truth, so no earlier command counts
// against the echoes it produces.
void TCUController::resync(Mailbox &box)
{
    box.sent = -1;
    box.in_flight = false;
    box.watchdog.stop();
}

void TCUController::init(Aliasing *aliasing)
{
    m_aliasing = aliasing;
}

void TCUController::setup_hz(int hz_unit)
{
    if (m_aliasing_mode) return;
    this->m_hz_unit = hz_unit;
    switch (hz_unit) {
    case 0: emit freq_display_updated("kHz", QString::number(m_device_mgr->get_tcu()->get(TCU::REPEATED_FREQ), 'f', 2)); break;
    case 1: emit freq_display_updated("Hz", QString::number((int)(m_device_mgr->get_tcu()->get(TCU::REPEATED_FREQ) * 1000))); break;
    default: break;
    }
}

void TCUController::setup_stepping(int base_unit)
{
    this->m_base_unit = base_unit;
    switch (base_unit) {
    case 0: emit stepping_display_updated("ns", QString::number(m_stepping, 'f', 2)); break;
    case 1: emit stepping_display_updated("μs", QString::number(m_stepping / 1000, 'f', 2)); break;
    case 2: emit stepping_display_updated("m", QString::number(m_stepping * m_dist_ns, 'f', 2)); break;
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

void TCUController::setup_min_delay()
{
    emit delay_slider_min_changed(qRound(min_delay_dist()));
}

void TCUController::update_delay_offset(float dist_offset)
{
    emit send_double_tcu_msg(TCU::OFFSET_DELAY, dist_offset / m_dist_ns);
}

void TCUController::update_gate_width_offset(float dov_offset)
{
    emit send_double_tcu_msg(TCU::OFFSET_GW, dov_offset / m_dist_ns);
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
    int prev_type = m_device_mgr->get_tcu()->get_type();
    int diff = 0;
    int laser_diff = 0;
    if (prev_type == 0 && idx) diff = 1;
    if (prev_type && idx == 0) diff = -1;
    if (idx == 2 && prev_type != 2) laser_diff = 1;
    if (prev_type == 2 && idx != 2) laser_diff = -1;
    m_device_mgr->get_tcu()->set_type(idx);

    switch (idx) {
    case 0: m_mcp_max = MAX_MCP_8BIT; break;
    case 1: m_mcp_max = MAX_MCP_12BIT; break;
    case 2: m_mcp_max = MAX_MCP_12BIT; break;
    default: break;
    }

    emit tcu_type_layout_changed(idx, diff, laser_diff);
    emit mcp_slider_max_changed(m_mcp_max);
}

void TCUController::update_ps_config(bool read, int idx, uint val)
{
    if (read) {
        float ps_step = m_device_mgr->get_tcu()->get(TCU::PS_STEP_1 + idx);
        emit ps_config_display_updated(
            QString::number(ps_step),
            QString::number(int(std::round(4000. / ps_step))));
    }
    else emit send_uint_tcu_msg(TCU::PS_STEP_1 + idx, val);
}

void TCUController::set_auto_mcp_slot(bool auto_mcp)
{
    this->m_auto_mcp = auto_mcp;
    emit auto_mcp_chk_changed(auto_mcp);
}

void TCUController::set_distance_set(int id)
{
    // FIXME: aliasing parameter application temporary banned
    Q_UNUSED(id);
    return;
}

void TCUController::update_tcu_params(qint32 tcu_param)
{
    uint us, ns, ps;
    switch (tcu_param) {
    case TCU::LASER_USR:
        split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::LASER_WIDTH), us, ns, ps);
        emit laser_width_display_updated(
            QString::asprintf(  "%d", us),
            QString::asprintf("%03d", ns),
            QString::asprintf("%03d", ps));
        break;
    case TCU::EST_DIST:
    {
        const double echoed = m_device_mgr->get_tcu()->get(TCU::EST_DIST);
        if (!accept_echo(m_delay_box, TCU::EST_DIST, echoed)) break;
        m_delay_dist.store(echoed);
        emit est_dist_text_updated(QString::asprintf("%.2f m", m_delay_dist.load()));
        m_rep_freq = m_device_mgr->get_tcu()->get(TCU::REPEATED_FREQ);
        setup_hz(m_hz_unit);
        {
            uint au, an, ap, bu, bn, bp;
            split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::DELAY_A), au, an, ap, 1);
            split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::DELAY_B), bu, bn, bp, 1);
            emit delay_display_updated(
                QString::asprintf(  "%d", au), QString::asprintf("%03d", an), QString::asprintf("%03d", ap),
                QString::asprintf(  "%d", bu), QString::asprintf("%03d", bn), QString::asprintf("%03d", bp));
        }
        emit delay_slider_value_changed(m_delay_dist.load());
        break;
    }
    case TCU::EST_DOV:
    {
        const double echoed = m_device_mgr->get_tcu()->get(TCU::EST_DOV);
        if (!accept_echo(m_dov_box, TCU::EST_DOV, echoed)) break;
        m_depth_of_view = echoed;
        {
            uint au, an, ap, bu, bn, bp;
            split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::GATE_WIDTH_A), au, an, ap, 0);
            split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::GATE_WIDTH_B), bu, bn, bp, 0);
            emit gw_display_updated(
                QString::asprintf(  "%d", au), QString::asprintf("%03d", an), QString::asprintf("%03d", ap),
                QString::asprintf(  "%d", bu), QString::asprintf("%03d", bn), QString::asprintf("%03d", bp),
                QString::asprintf("%.2f m", m_depth_of_view));
        }
        emit gw_slider_value_changed(m_depth_of_view);
        break;
    }
    case TCU::NO_PARAM:
        // See resync(). A request that was waiting on the link when the
        // readback happened still goes out after it.
        resync(m_delay_box);
        resync(m_dov_box);
        update_tcu_params(TCU::LASER_USR);
        update_tcu_params(TCU::EST_DIST);
        update_tcu_params(TCU::EST_DOV);
        emit mcp_display_updated(m_device_mgr->get_tcu()->get(TCU::MCP), "");
        if (m_delay_box.pending) dispatch(m_delay_box, TCU::EST_DIST);
        if (m_dov_box.pending)   dispatch(m_dov_box,   TCU::EST_DOV);
    default: break;
    }
}

void TCUController::change_mcp(int val)
{
    if (val < 0) val = 0;
    if (val > m_mcp_max) val = m_mcp_max;
    emit mcp_display_updated(val, QString::number(val));

    if (val == (int)std::round(m_device_mgr->get_tcu()->get(TCU::MCP))) return;

    emit send_uint_tcu_msg(TCU::MCP, val);
}

void TCUController::change_delay(int val)
{
    if (abs(m_delay_dist.load() - val) < 1) return;
    m_delay_dist.store(val);
    update_delay();
}

void TCUController::change_gatewidth(int val)
{
    if (abs(m_depth_of_view - val) < 1) return;
    m_depth_of_view = val;
    update_gate_width();
}

void TCUController::apply_laser_width(float val)
{
    m_laser_width = qBound(0.0f, val, m_config->get_data().tcu.max_laser_width);

    uint us, ns, ps;
    split_value_by_unit(m_laser_width, us, ns, ps);
    emit laser_width_display_updated(
        QString::asprintf(  "%d", us),
        QString::asprintf("%03d", ns),
        QString::asprintf("%03d", ps));
    emit send_double_tcu_msg(TCU::LASER_USR, m_laser_width);
}

void TCUController::apply_delay(int val)
{
    const int min_dist = qRound(min_delay_dist());
    const int max_dist = qMax(0, static_cast<int>(m_config->get_data().tcu.max_dist));
    const int applied = qBound(min_dist, val, max_dist);
    m_delay_dist.store(applied);

    emit delay_slider_value_changed(applied);
    emit est_dist_text_updated(QString::asprintf("%.2f m", m_delay_dist.load()));
    request(m_delay_box, TCU::EST_DIST, m_delay_dist.load());
}

void TCUController::apply_gatewidth(int val)
{
    const int max_dov = qMax(0, static_cast<int>(m_config->get_data().tcu.max_dov));
    const int applied = qBound(0, val, max_dov);
    m_depth_of_view = applied;

    emit gw_slider_value_changed(applied);
    request(m_dov_box, TCU::EST_DOV, m_depth_of_view);
}

void TCUController::apply_distance(int dist)
{
    m_distance = dist;
    if (m_distance < 100) m_distance = 100;
    emit distance_text_updated(QString::asprintf("%d m", m_distance));

    m_delay_dist.store(m_distance);
    update_delay();

    m_rep_freq = 1e6 / (m_delay_dist.load() / m_dist_ns + m_depth_of_view / m_dist_ns + 1000);
    if (m_rep_freq > 30) m_rep_freq = 30;
    if      (m_distance < 1000) m_depth_of_view =  500 * m_dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(m_depth_of_view / m_dist_ns));
    else if (m_distance < 3000) m_depth_of_view = 1000 * m_dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(m_depth_of_view / m_dist_ns));
    else if (m_distance < 6000) m_depth_of_view = 2000 * m_dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(m_depth_of_view / m_dist_ns));
    else                        m_depth_of_view = 3500 * m_dist_ns, emit send_double_tcu_msg(TCU::LASER_WIDTH, std::round(m_depth_of_view / m_dist_ns));

    emit send_uint_tcu_msg(TCU::DIST, m_distance);
    emit est_dist_text_updated(QString::asprintf("%.2f m", m_delay_dist.load()));

    setup_hz(m_hz_unit);
    uint us, ns, ps;
    split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::LASER_WIDTH), us, ns, ps);
    emit laser_width_display_updated(
        QString::asprintf(  "%d", us),
        QString::asprintf("%03d", ns),
        QString::asprintf("%03d", ps));

    {
        uint au, an, ap, bu, bn, bp;
        split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::GATE_WIDTH_A), au, an, ap, 0);
        split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::GATE_WIDTH_B), bu, bn, bp, 2);
        emit gw_display_updated(
            QString::asprintf(  "%d", au), QString::asprintf("%03d", an), QString::asprintf("%03d", ap),
            QString::asprintf(  "%d", bu), QString::asprintf("%03d", bn), QString::asprintf("%03d", bp),
            "");
    }
    {
        uint au, an, ap, bu, bn, bp;
        split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::DELAY_A), au, an, ap, 1);
        split_value_by_unit(m_device_mgr->get_tcu()->get(TCU::DELAY_B), bu, bn, bp, 3);
        emit delay_display_updated(
            QString::asprintf(  "%d", au), QString::asprintf("%03d", an), QString::asprintf("%03d", ap),
            QString::asprintf(  "%d", bu), QString::asprintf("%03d", bn), QString::asprintf("%03d", bp));
    }
}

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

void TCUController::update_laser_width()
{
#ifndef LVTONG
    static QElapsedTimer t;
    if (throttle_check(t)) return;
#endif
    m_laser_width = qBound(0.0f, m_laser_width, m_config->get_data().tcu.max_laser_width);
    emit send_double_tcu_msg(TCU::LASER_USR, m_laser_width);
}

void TCUController::update_delay()
{
    m_delay_dist.store(qBound(min_delay_dist(), m_delay_dist.load(), m_config->get_data().tcu.max_dist));

    if (!m_aliasing_mode) {
        // Slider and estimate follow the operator at once. The A/B register
        // fields wait for the echo, since they show what the device holds.
        if (!qobject_cast<QSlider*>(sender()))
            emit delay_slider_value_changed(m_delay_dist.load());
        emit est_dist_text_updated(QString::asprintf("%.2f m", m_delay_dist.load()));
        request(m_delay_box, TCU::EST_DIST, m_delay_dist.load());
    }
    else {
        m_aliasing_mode = false;
        request(m_delay_box, TCU::EST_DIST, m_device_mgr->get_tcu()->get(TCU::EST_DIST));
    }
}

void TCUController::update_gate_width()
{
    m_depth_of_view = qBound(0.0f, m_depth_of_view, m_config->get_data().tcu.max_dov);

    if (!qobject_cast<QSlider*>(sender()))
        emit gw_slider_value_changed(m_depth_of_view);

    request(m_dov_box, TCU::EST_DOV, m_depth_of_view);
}

void TCUController::update_light_speed(bool uw)
{
    m_c = uw ? 3e8 * 0.75 : 3e8;
    m_dist_ns = m_c * 1e-9 / 2;
    // ScanController keeps its own dist_ns; userpanel.cpp updates it from this signal.
    emit dist_ns_changed(m_dist_ns);
    emit update_distance_display_requested();
    // The floor is fixed in ns, so its position in metres moves with dist_ns.
    setup_min_delay();
    emit send_double_tcu_msg(TCU::LIGHT_SPEED, m_dist_ns);
    // Re-push EST_DIST and EST_DOV so the port reconverts metres to ns with the new dist_ns.
    // The mailbox holds the re-push until the link is free, so rapid toggles cannot drop it.
    request(m_delay_box, TCU::EST_DIST, m_delay_dist.load());
    request(m_dov_box,   TCU::EST_DOV,  m_depth_of_view);
}

void TCUController::update_current(float current_value)
{
    QString send = QString::asprintf("PCUR %.2f\r", current_value);
    emit send_laser_msg(send);
    qDebug() << send;
}

QByteArray TCUController::convert_to_send_tcu(uchar num, unsigned int send)
{
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

inline void TCUController::split_value_by_unit(float val, uint &us, uint &ns, uint &ps, int idx)
{
    static float ONE = 1.f;
    switch (m_device_mgr->get_tcu()->get_type()) {
    case 0:
    case 1:
    {
        us = uint(val + 0.001) / 1000;
        ns = uint(val + 0.001) % 1000;
        if (idx < 0) ps = 0;
        else         ps = std::round(std::modf(val + 0.001, &ONE) * 1000 / m_device_mgr->get_tcu()->get(TCU::PS_STEP_1 + idx)) * m_device_mgr->get_tcu()->get(TCU::PS_STEP_1 + idx);
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
