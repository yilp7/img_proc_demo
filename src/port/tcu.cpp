#include "tcu.h"
#include "tcu_protocol.h"
#include "util/constants.h"

TCU::TCU(int index, int tcu_type) :
    ControlPort{index},
    m_tcu_type(tcu_type),
    m_scan_mode(0),
    m_rep_freq(30),
    m_laser_width(40),
    m_delay_a(100),
    m_delay_b(100),
    m_gate_width_a(40),
    m_gate_width_b(40),
    m_ccd_freq(10),
    m_duty(5000),
    m_mcp(5),
    m_laser_on(0b0000),
    m_tcu_dist(300),
    m_dist_ns(LIGHT_SPEED_M_NS),
    m_delay_n(0),
    m_gate_width_n(0),
    m_delay_dist(m_delay_a * m_dist_ns),
    m_depth_of_view(m_gate_width_a * m_dist_ns),
    m_auto_rep_freq(false),
    m_ab_lock(true),
    m_laser_offset(0),
    m_delay_offset(0),
    m_gate_width_offset(0),
    m_ps_step{40, 40, 40, 40},
    m_use_gw_lut(false),
    m_gw_lut{0}
{
    uchar q[] = {TCU_HEAD, TCU_QUERY_CMD, 0x00, 0x00, 0x00, 0x00, TCU_TAIL};
    m_query = QByteArray((char*)q, 7);
}

TCU::~TCU()
{

}

double TCU::get(qint32 tcu_param)
{
    switch (tcu_param) {
    case REPEATED_FREQ: return m_rep_freq;
    case LASER_WIDTH:
        switch (m_tcu_type) {
        case 2:  return m_gate_width_a;
        default: return m_laser_width;
        }
    case DELAY_A:
        switch (m_tcu_type) {
        case 2:  return m_delay_b;
        default: return m_delay_a;
        }
    case GATE_WIDTH_A:
        switch (m_tcu_type) {
        case 2:  return m_gate_width_b;
        default: return m_gate_width_a;
        }
    case DELAY_B:       return m_delay_b;
    case GATE_WIDTH_B:  return m_gate_width_b;
    case CCD_FREQ:      return m_ccd_freq;
    case DUTY:          return m_duty;
    case MCP:           return m_mcp;
    case DELAY_N:       return m_delay_n;
    case GATE_WIDTH_N:  return m_gate_width_n;
    case OFFSET_DELAY:  return m_delay_offset;
    case OFFSET_GW:     return m_gate_width_offset;
    case OFFSET_LASER:  return m_laser_offset;
    case PS_STEP_1:     return m_ps_step[0];
    case PS_STEP_2:     return m_ps_step[1];
    case PS_STEP_3:     return m_ps_step[2];
    case PS_STEP_4:     return m_ps_step[3];
    case EST_DIST:      return m_delay_dist;
    case EST_DOV:       return m_depth_of_view;
    case AB_LOCK:       return m_ab_lock;
    default:            return 0;
    }
}

uint TCU::get_type() { return m_tcu_type.load(); }

void TCU::set_type(uint type) { m_tcu_type.store(type); }

#if ENABLE_PORT_JSON
nlohmann::json TCU::to_json()
{
    return nlohmann::json{
        {"type",        m_tcu_type.load()},
        {"PRF",         m_rep_freq},
        {"laser_width", m_laser_width},
        {"delay_a",     m_delay_a},
        {"gw_a",        m_gate_width_a},
        {"delay_b",     m_delay_b},
        {"gw_b",        m_gate_width_b},
        {"ccd_freq",    m_ccd_freq},
        {"duty",        m_duty},
        {"mcp",         m_mcp},
        {"delay_n",     m_delay_n},
        {"gw_n",        m_gate_width_n},
    };
}
#endif

bool TCU::connect_to_serial_port(QString port_name, qint32 baudrate)
{
    bool success = ControlPort::connect_to_serial_port(port_name, baudrate);
    if (success) init_tcu_on_connect();
    return success;
}

bool TCU::connect_to_tcp_port(QString ip, ushort port)
{
    bool success = ControlPort::connect_to_tcp_port(ip, port);
    if (success) init_tcu_on_connect();
    return success;
}

void TCU::init_tcu_on_connect()
{
    set_user_param(TCU::REPEATED_FREQ, m_rep_freq);
    set_user_param(TCU::LASER_WIDTH  , m_laser_width);
    set_user_param(TCU::DELAY_A      , m_delay_a);
    set_user_param(TCU::GATE_WIDTH_A , m_gate_width_a);
    set_user_param(TCU::DELAY_B      , m_delay_b);
    set_user_param(TCU::GATE_WIDTH_B , m_gate_width_b);
    set_user_param(TCU::CCD_FREQ     , m_ccd_freq);
    set_user_param(TCU::DUTY         , m_duty);
    set_user_param(TCU::MCP          , m_mcp);
    set_user_param(TCU::LASER_1      , uint(0x04));
    set_user_param(TCU::LASER_2      , uint(0x04));
    set_user_param(TCU::LASER_3      , uint(0x04));
    set_user_param(TCU::LASER_4      , uint(0x04));
    set_user_param(TCU::TOGGLE_LASER , uint(0x00));
}

void TCU::try_communicate()
{
    ControlPort::try_communicate(); return;

    static int write_idx = 0;

    communicate(m_query, 1, 10, true);
    QByteArray read;
    m_retrieve_mutex.lock();
    read = m_last_read;
    m_retrieve_mutex.unlock();

    if (read.size() != 1 || read[0] != char(TCU_QUERY_REPLY)) m_successive_count = 0;
    else                                           m_successive_count++;
}

void TCU::set_user_param(qint32 tcu_param, double val)
{
    uint clock_cycle;
    switch (m_tcu_type.load()) {
    case 0: clock_cycle = TCU_CLOCK_8; break;
    case 1: clock_cycle = TCU_CLOCK_4; break;
    case 2: clock_cycle = TCU_CLOCK_4; break;
    default: break;
    }

    switch (tcu_param) {
    case REPEATED_FREQ: m_rep_freq = val;     set_tcu_param(tcu_param, (1e6 / clock_cycle) / m_rep_freq); break;
    case LASER_WIDTH  : m_laser_width = val;  set_tcu_param(tcu_param, m_tcu_type.load() == 0 ? m_laser_width / TCU_CLOCK_8 : m_laser_width); break;
    case DELAY_A:       m_delay_a = val;      set_tcu_param(tcu_param, m_delay_a); break;
    case GATE_WIDTH_A:  m_gate_width_a = val; set_tcu_param(tcu_param, m_gate_width_a); break;
    case DELAY_B:       m_delay_b = val;      set_tcu_param(tcu_param, m_delay_b); emit tcu_param_updated(EST_DIST); break;
    case GATE_WIDTH_B:  m_gate_width_b = val; set_tcu_param(tcu_param, m_gate_width_b); emit tcu_param_updated(EST_DOV); break;
    case CCD_FREQ:      m_ccd_freq = val;     set_tcu_param(tcu_param, (1e9 / clock_cycle) / m_ccd_freq); break;
    case DUTY:          m_duty = val;         set_tcu_param(tcu_param, m_duty * 1e3 / clock_cycle); break;
    case DELAY_N:       m_delay_n = val;                                     break;
    case GATE_WIDTH_N:  m_gate_width_n = val;                                break;
    case LASER_USR:
    {
        m_laser_width = val;
        set_user_param(TCU::LASER_WIDTH, val + m_laser_offset);

        emit tcu_param_updated(LASER_USR);
        break;
    }
    case EST_DIST:
    {
        m_delay_dist = val;
        m_delay_a = m_delay_dist / m_dist_ns;
        if (m_ab_lock) m_delay_b = m_delay_a + m_delay_n;

        if (m_scan_mode) m_rep_freq = m_rep_freq_scan;
        else if (m_auto_rep_freq) {
            // change repeated frequency according to delay: rep frequency (kHz) <= 1s / delay (μs)
            m_rep_freq = m_delay_dist ? 1e6 / (m_delay_dist / m_dist_ns + m_depth_of_view / m_dist_ns + 1000) : TCU_MAX_PRF_KHZ;
            if (m_rep_freq > TCU_MAX_PRF_KHZ) m_rep_freq = TCU_MAX_PRF_KHZ;
//                if (m_rep_freq < 10) m_rep_freq = 10;
            set_user_param(TCU::REPEATED_FREQ, m_rep_freq);
        }
        set_user_param(TCU::DELAY_A, m_delay_a + m_delay_offset);
        if (m_ab_lock) set_user_param(TCU::DELAY_B, m_delay_b + m_delay_offset);

        emit tcu_param_updated(EST_DIST);
        break;
    }
    case EST_DOV:
    {
#if ENABLE_GW_LUT
        m_depth_of_view = val;
        int gw = std::round(val / m_dist_ns), gw_corrected_a = gw + m_gate_width_offset, gw_corrected_b = gw + m_gate_width_n + m_gate_width_offset;
        if (gw_corrected_a < 0 || gw_corrected_b < 0) return -1;
        if (gw_corrected_a < 100) gw_corrected_a = m_gw_lut[gw_corrected_a];
        if (gw_corrected_b < 100) gw_corrected_b = m_gw_lut[gw_corrected_b];
        if (gw_corrected_a == -1 || gw_corrected_b == -1) return -1;

        m_gate_width_a = (m_depth_of_view = val) / m_dist_ns;
        m_gate_width_b = m_gate_width_a + m_gate_width_n;
        set_tcu_param(TCU::GATE_WIDTH_A, m_use_gw_lut ? gw_corrected_a : m_gate_width_a);
        set_tcu_param(TCU::GATE_WIDTH_B, m_use_gw_lut ? gw_corrected_b : m_gate_width_b);
        break;
#else
        m_depth_of_view = val;
        m_gate_width_a = (m_depth_of_view = val) / m_dist_ns;
        if (m_ab_lock) m_gate_width_b = m_gate_width_a + m_gate_width_n;
        set_user_param(TCU::GATE_WIDTH_A, m_gate_width_a + m_gate_width_offset);
        if (m_ab_lock) set_user_param(TCU::GATE_WIDTH_B, m_gate_width_b + m_gate_width_offset);

        emit tcu_param_updated(EST_DOV);
        break;
#endif
    }
    case REP_FREQ_SCAN: m_rep_freq_scan = val; break;
    case OFFSET_DELAY:   m_delay_offset = val; break;
    case OFFSET_GW: m_gate_width_offset = val; break;
    case OFFSET_LASER:   m_laser_offset = val; break;
    case LIGHT_SPEED:         m_dist_ns = val; break;

    default: break;
    }
}

void TCU::set_user_param(qint32 tcu_param, uint val)
{
    switch (tcu_param) {
    case MCP:  m_mcp = val;                  set_tcu_param(tcu_param, val); break;
    case LASER_1:                            set_tcu_param(tcu_param, val); break;
    case LASER_2:                            set_tcu_param(tcu_param, val); break;
    case LASER_3:                            set_tcu_param(tcu_param, val); break;
    case LASER_4:                            set_tcu_param(tcu_param, val); break;
    case DIST: m_tcu_dist = std::round(val); set_tcu_param(tcu_param, val); break;
    case TOGGLE_LASER:                       set_tcu_param(tcu_param, val); break;
    case LASER_ON:
    {
        int change = m_laser_on ^ int(val);
        m_laser_on = val;
        for(int i = 0; i < 4; i++) if ((change >> i) & 1) set_tcu_param(TCU::Params(TCU::LASER_1 + i), int(val) & (1 << i) ? 0x08 : 0x04);
        break;
    }
    case PS_STEP_1: m_ps_step[0] = val; break;
    case PS_STEP_2: m_ps_step[1] = val; break;
    case PS_STEP_3: m_ps_step[2] = val; break;
    case PS_STEP_4: m_ps_step[3] = val; break;
//        case TCU_TYPE:  m_tcu_type = val; break;
    case AUTO_PRF:  m_auto_rep_freq = val; break;
    case AB_LOCK:   m_ab_lock = val; break;
    default: break;
    }
}

#if ENABLE_PORT_JSON
void TCU::load_from_json(const nlohmann::json &j)
{
    cout_mutex.lock();
    std::cout << j << std::endl;
    std::cout.flush();
    cout_mutex.unlock();
    
    try {
        if (j.contains("type") && j["type"].is_number())
            set_user_param(TCU_TYPE, j["type"].get<uint>());
        if (j.contains("PRF") && j["PRF"].is_number())
            set_user_param(REPEATED_FREQ, j["PRF"].get<double>());
        if (j.contains("laser_width") && j["laser_width"].is_number())
            set_user_param(LASER_USR, j["laser_width"].get<double>());
        if (j.contains("delay_a") && j["delay_a"].is_number())
            set_user_param(DELAY_A, j["delay_a"].get<double>());
        if (j.contains("gw_a") && j["gw_a"].is_number())
            set_user_param(GATE_WIDTH_A, j["gw_a"].get<double>());
        if (j.contains("delay_b") && j["delay_b"].is_number())
            set_user_param(DELAY_B, j["delay_b"].get<double>());
        if (j.contains("gw_b") && j["gw_b"].is_number())
            set_user_param(GATE_WIDTH_B, j["gw_b"].get<double>());
        if (j.contains("ccd_freq") && j["ccd_freq"].is_number())
            set_user_param(CCD_FREQ, j["ccd_freq"].get<double>());
        if (j.contains("duty") && j["duty"].is_number())
            set_user_param(DUTY, j["duty"].get<double>());
        if (j.contains("mcp") && j["mcp"].is_number())
            set_user_param(MCP, j["mcp"].get<uint>());
        if (j.contains("delay_n") && j["delay_n"].is_number())
            set_user_param(DELAY_N, j["delay_n"].get<double>());
        if (j.contains("gw_n") && j["gw_n"].is_number())
            set_user_param(GATE_WIDTH_N, j["gw_n"].get<double>());
    }
    catch (const nlohmann::json::exception &e) {
        qWarning("JSON parsing error in TCU configuration: %s", e.what());
    }
    
    m_delay_dist = m_delay_a * m_dist_ns;
    m_depth_of_view = m_gate_width_a * m_dist_ns;
    emit tcu_param_updated(NO_PARAM);
}
#endif

void TCU::set_tcu_param(qint32 tcu_param, double val)
{
    uint integer_part, decimal_part;

    switch (m_tcu_type.load()) {
    case 0:
    {
        integer_part = std::round(val);
        decimal_part = 0;

        QByteArray out(7, 0x00);
        out[0] = TCU_HEAD;
        out[1] = tcu_param;
        out[6] = TCU_TAIL;

        out[5] = integer_part & 0xFF; integer_part >>= 8;
        out[4] = integer_part & 0xFF; integer_part >>= 8;
        out[3] = integer_part & 0xFF; integer_part >>= 8;
        out[2] = integer_part & 0xFF;

        communicate(out, 1, 10);
        break;;
    }
    case 1:
    {
        TCU::Params param_ps = TCU::Params::NO_PARAM;
        int step = 0;
        switch (tcu_param) {
        case DELAY_A:
            param_ps = DELAY_A_PS;
            step = m_ps_step[0];
            integer_part = int(val + 0.001) / 4;
            decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
            m_delay_a = integer_part * 4 + decimal_part * step / 1000.;
            break;
        case DELAY_B:
            param_ps = DELAY_B_PS;
            step = m_ps_step[1];
            integer_part = int(val + 0.001) / 4;
            decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
            m_delay_b = integer_part * 4 + decimal_part * step / 1000.;
            break;
        case GATE_WIDTH_A:
            param_ps = GW_A_PS;
            step = m_ps_step[2];
            integer_part = int(val + 0.001) / 4;
            decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
            m_gate_width_a = integer_part * 4 + decimal_part * step / 1000.;
            break;
        case GATE_WIDTH_B:
            param_ps = GW_B_PS;
            step = m_ps_step[3];
            integer_part = int(val + 0.001) / 4;
            decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
            m_gate_width_b = integer_part * 4 + decimal_part * step / 1000.;
            break;
        default:
            integer_part = int(val);
            decimal_part = 0;
            break;
        }

        QByteArray out_1(7, 0x00);
        out_1[0] = TCU_HEAD;
        out_1[1] = tcu_param;
        out_1[6] = TCU_TAIL;

        out_1[5] = integer_part & 0xFF; integer_part >>= 8;
        out_1[4] = integer_part & 0xFF; integer_part >>= 8;
        out_1[3] = integer_part & 0xFF; integer_part >>= 8;
        out_1[2] = integer_part & 0xFF;

        communicate(out_1, 7, 40);

        if (param_ps != TCU::Params::NO_PARAM) {
            QByteArray out_2(7, 0x00);
            out_2[0] = TCU_HEAD;
            out_2[1] = param_ps;
            out_2[6] = TCU_TAIL;

            out_2[5] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[4] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[3] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[2] = decimal_part & 0xFF;

            communicate(out_2, 7, 40);
        }
        break;
    }
    case 2:
    {
        TCU::Params param_ps = TCU::Params::NO_PARAM;
        int step = 0;
        switch (tcu_param) {
        case DELAY_A:
            tcu_param = DELAY_B;
            param_ps = DELAY_B_PS;
            integer_part = int(val + 0.001);
            decimal_part = std::round((val - integer_part + 0.001) * 1000 / 11.23);
            m_delay_b = integer_part + decimal_part * 11.23 / 1000.;
            break;
        case LASER_WIDTH:
            tcu_param = GATE_WIDTH_A;
            param_ps = GW_A_PS;
            integer_part = int(val + 0.001);
            decimal_part = std::round((val - integer_part + 0.001) * 1000 / 14.9);
            m_gate_width_a = integer_part + decimal_part * 14.9 / 1000.;
            break;
        case GATE_WIDTH_B:
            tcu_param = GATE_WIDTH_B;
            param_ps = GW_B_PS;
            integer_part = int(val + 0.001);
            decimal_part = std::round((val - integer_part + 0.001) * 1000 / 14.9);
            m_gate_width_b = integer_part + decimal_part * 14.9 / 1000.;
            break;
        default:
            integer_part = int(val);
            decimal_part = 0;
            break;
        }

        QByteArray out_1(7, 0x00);
        out_1[0] = TCU_HEAD;
        out_1[1] = tcu_param;
        out_1[6] = TCU_TAIL;

        out_1[5] = integer_part & 0xFF; integer_part >>= 8;
        out_1[4] = integer_part & 0xFF; integer_part >>= 8;
        out_1[3] = integer_part & 0xFF; integer_part >>= 8;
        out_1[2] = integer_part & 0xFF;

        communicate(out_1, 7, 40);

        if (param_ps != TCU::Params::NO_PARAM) {
            QByteArray out_2(7, 0x00);
            out_2[0] = TCU_HEAD;
            out_2[1] = param_ps;
            out_2[6] = TCU_TAIL;

            out_2[5] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[4] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[3] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[2] = decimal_part & 0xFF;

            communicate(out_2, 7, 40);
        }
        break;
    }
    default: break;
    }
}
