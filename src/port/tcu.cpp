#include "tcu.h"

TCU::TCU(int index, int tcu_type) :
    ControlPort{index},
    tcu_type(tcu_type),
    scan_mode(0),
    rep_freq(30),
    laser_width(100),
    delay_a(100),
    delay_b(100),
    gate_width_a(100),
    gate_width_b(100),
    ccd_freq(10),
    duty(5000),
    mcp(5),
    laser_on(0b0000),
    tcu_dist(300),
    dist_ns(3e8 * 1e-9 / 2),
    delay_n(0),
    gate_width_n(0),
    delay_dist(delay_a * dist_ns),
    depth_of_view(gate_width_a * dist_ns),
    auto_rep_freq(true),
    ab_lock(true),
    laser_offset(0),
    delay_offset(0),
    gate_width_offset(0),
    ps_step{40, 40, 40, 40},
    use_gw_lut(false),
    gw_lut{0}
{
    query = generate_ba(new uchar[7]{0x88, 0x15, 0x00, 0x00, 0x00, 0x00, 0x99}, 7);
}

TCU::~TCU()
{

}

double TCU::get(qint32 tcu_param)
{
    switch (tcu_param)
    {
        case REPEATED_FREQ: return rep_freq;
        case LASER_WIDTH:
            switch (tcu_type)
            {
            case 2:  return gate_width_a;
            default: return laser_width;
            }
        case DELAY_A:
            switch (tcu_type)
            {
            case 2:  return delay_b;
            default: return delay_a;
            }
        case GATE_WIDTH_A:  return gate_width_a;
            switch (tcu_type)
            {
            case 2:  return gate_width_b;
            default: return gate_width_a;
            }
        case DELAY_B:       return delay_b;
        case GATE_WIDTH_B:  return gate_width_b;
        case CCD_FREQ:      return ccd_freq;
        case DUTY:          return duty;
        case MCP:           return mcp;
        case DELAY_N:       return delay_n;
        case GATE_WIDTH_N:  return gate_width_n;
        case OFFSET_DELAY:  return delay_offset;
        case OFFSET_GW:     return gate_width_offset;
        case OFFSET_LASER:  return laser_offset;
        case PS_STEP_1:     return ps_step[0];
        case PS_STEP_2:     return ps_step[1];
        case PS_STEP_3:     return ps_step[2];
        case PS_STEP_4:     return ps_step[3];
        case EST_DIST:      return delay_dist;
        case EST_DOV:       return depth_of_view;
        default:            return 0;
        }
}

uint TCU::type() { return tcu_type.load(); }

void TCU::set_type(uint type) { tcu_type.store(type); }

#if ENABLE_PORT_JSON
nlohmann::json TCU::to_json()
{
    return nlohmann::json{
        {"type",        tcu_type.load()},
        {"PRF",         rep_freq},
        {"laser_width", laser_width},
        {"delay_a",     delay_a},
        {"gw_a",        gate_width_a},
        {"delay_b",     delay_b},
        {"gw_b",        gate_width_b},
        {"ccd_freq",    ccd_freq},
        {"duty",        duty},
        {"mcp",         mcp},
        {"delay_n",     delay_n},
        {"gw_n",        gate_width_n},
    };
}
#endif

bool TCU::connect_to_serial_port(QString port_name, qint32 baudrate)
{
    bool success = ControlPort::connect_to_serial_port(port_name, baudrate);

    if (success) {
        set_user_param(TCU::REPEATED_FREQ, rep_freq);
        set_user_param(TCU::LASER_WIDTH  , laser_width);
        set_user_param(TCU::DELAY_A      , delay_a);
        set_user_param(TCU::GATE_WIDTH_A , gate_width_a);
        set_user_param(TCU::DELAY_B      , delay_b);
        set_user_param(TCU::GATE_WIDTH_B , gate_width_b);
        set_user_param(TCU::CCD_FREQ     , ccd_freq);
        set_user_param(TCU::DUTY         , duty);
        set_user_param(TCU::MCP          , mcp);
        set_user_param(TCU::LASER_1      , uint(0x04));
        set_user_param(TCU::LASER_2      , uint(0x04));
        set_user_param(TCU::LASER_3      , uint(0x04));
        set_user_param(TCU::LASER_4      , uint(0x04));
        set_user_param(TCU::TOGGLE_LASER , uint(0x00));
    }

    return success;
}

bool TCU::connect_to_tcp_port(QString ip, ushort port)
{
    bool success = ControlPort::connect_to_tcp_port(ip, port);

    if (success) {
        set_user_param(TCU::REPEATED_FREQ, rep_freq);
        set_user_param(TCU::LASER_WIDTH  , laser_width);
        set_user_param(TCU::DELAY_A      , delay_a);
        set_user_param(TCU::GATE_WIDTH_A , gate_width_a);
        set_user_param(TCU::DELAY_B      , delay_b);
        set_user_param(TCU::GATE_WIDTH_B , gate_width_b);
        set_user_param(TCU::CCD_FREQ     , ccd_freq);
        set_user_param(TCU::DUTY         , duty);
        set_user_param(TCU::MCP          , mcp);
        set_user_param(TCU::LASER_1      , uint(0x04));
        set_user_param(TCU::LASER_2      , uint(0x04));
        set_user_param(TCU::LASER_3      , uint(0x04));
        set_user_param(TCU::LASER_4      , uint(0x04));
        set_user_param(TCU::TOGGLE_LASER , uint(0x00));
    }

    return success;
}

void TCU::try_communicate()
{
    ControlPort::try_communicate(); return;

    static int write_idx = 0;

    communicate(query, 1, 10, true);
    QByteArray read;
    retrieve_mutex.lock();
    read = last_read;
    retrieve_mutex.unlock();

    if (read.size() != 1 || read[0] != char(0x15)) successive_count = 0;
    else                                           successive_count++;
}

void TCU::set_user_param(qint32 tcu_param, double val)
{
    uint clock_cycle;
    switch (tcu_type.load()) {
        case 0: clock_cycle = 8; break;
        case 1: clock_cycle = 4; break;
        case 2: clock_cycle = 4; break;
        default: break;
    }

    switch (tcu_param) {
        case REPEATED_FREQ: rep_freq = val;     set_tcu_param(tcu_param, (1e6 / clock_cycle) / rep_freq); break;
        case LASER_WIDTH  : laser_width = val;  set_tcu_param(tcu_param, laser_width); break;
        case DELAY_A:       delay_a = val;      set_tcu_param(tcu_param, delay_a); break;
        case GATE_WIDTH_A:  gate_width_a = val; set_tcu_param(tcu_param, gate_width_a); break;
        case DELAY_B:       delay_b = val;      set_tcu_param(tcu_param, delay_b); break;
        case GATE_WIDTH_B:  gate_width_b = val; set_tcu_param(tcu_param, gate_width_b); break;
        case CCD_FREQ:      ccd_freq = val;     set_tcu_param(tcu_param, (1e9 / clock_cycle) / ccd_freq); break;
        case DUTY:          duty = val;         set_tcu_param(tcu_param, duty * 1e3 / clock_cycle); break;
        case DELAY_N:       delay_n = val;                                     break;
        case GATE_WIDTH_N:  gate_width_n = val;                                break;
        case LASER_USR:
        {
            laser_width = val;
            set_user_param(TCU::LASER_WIDTH, val + laser_offset);

            emit tcu_param_updated(LASER_USR);
            break;
        }
        case EST_DIST:
        {
            delay_dist = val;
            delay_a = delay_dist / dist_ns;
            delay_b = delay_a + delay_n;

            if (scan_mode) rep_freq = rep_freq_scan;
            else if (auto_rep_freq) {
                // change repeated frequency according to delay: rep frequency (kHz) <= 1s / delay (μs)
                rep_freq = delay_dist ? 1e6 / (delay_dist / dist_ns + depth_of_view / dist_ns + 1000) : 30;
                if (rep_freq > 30) rep_freq = 30;
//                if (rep_freq < 10) rep_freq = 10;
                set_user_param(TCU::REPEATED_FREQ, rep_freq);
            }
            set_user_param(TCU::DELAY_A, delay_a + delay_offset);
            set_user_param(TCU::DELAY_B, delay_b + delay_offset);

            emit tcu_param_updated(EST_DIST);
            break;
        }
        case EST_DOV:
        {
#if ENABLE_GW_LUT
            depth_of_view = val;
            int gw = std::round(val / dist_ns), gw_corrected_a = gw + gate_width_offset, gw_corrected_b = gw + gate_width_n + gate_width_offset;
            if (gw_corrected_a < 0 || gw_corrected_b < 0) return -1;
            if (gw_corrected_a < 100) gw_corrected_a = gw_lut[gw_corrected_a];
            if (gw_corrected_b < 100) gw_corrected_b = gw_lut[gw_corrected_b];
            if (gw_corrected_a == -1 || gw_corrected_b == -1) return -1;

            gate_width_a = (depth_of_view = val) / dist_ns;
            gate_width_b = gate_width_a + gate_width_n;
            set_tcu_param(TCU::GATE_WIDTH_A, use_gw_lut ? gw_corrected_a : gate_width_a);
            set_tcu_param(TCU::GATE_WIDTH_B, use_gw_lut ? gw_corrected_b : gate_width_b);
            break;
#else
            depth_of_view = val;
            gate_width_a = (depth_of_view = val) / dist_ns;
            gate_width_b = gate_width_a + gate_width_n;
            set_user_param(TCU::GATE_WIDTH_A, gate_width_a + gate_width_offset);
            set_user_param(TCU::GATE_WIDTH_B, gate_width_b + gate_width_offset);

            emit tcu_param_updated(EST_DOV);
            break;
#endif
        }
        case REP_FREQ_SCAN: rep_freq_scan = val; break;
        case OFFSET_DELAY:   delay_offset = val; break;
        case OFFSET_GW: gate_width_offset = val; break;
        case OFFSET_LASER:   laser_offset = val; break;
        case LIGHT_SPEED:         dist_ns = val; break;

        default: break;
        }
}

void TCU::set_user_param(qint32 tcu_param, uint val)
{
    switch (tcu_param)
    {
        case MCP:  mcp = val;                  set_tcu_param(tcu_param, val); break;
        case LASER_1:                          set_tcu_param(tcu_param, val); break;
        case LASER_2:                          set_tcu_param(tcu_param, val); break;
        case LASER_3:                          set_tcu_param(tcu_param, val); break;
        case LASER_4:                          set_tcu_param(tcu_param, val); break;
        case DIST: tcu_dist = std::round(val); set_tcu_param(tcu_param, val); break;
        case TOGGLE_LASER:                     set_tcu_param(tcu_param, val); break;
        case LASER_ON:
        {
            int change = laser_on ^ int(val);
            laser_on = val;
            for(int i = 0; i < 4; i++) if ((change >> i) & 1) set_tcu_param(TCU::PARAMS(TCU::LASER_1 + i), int(val) & (1 << i) ? 8 : 4);
            break;
        }
        case PS_STEP_1: ps_step[0] = val; break;
        case PS_STEP_2: ps_step[1] = val; break;
        case PS_STEP_3: ps_step[2] = val; break;
        case PS_STEP_4: ps_step[3] = val; break;
//        case TCU_TYPE:  tcu_type = val; break;
        case AUTO_PRF:  auto_rep_freq = val; break;
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
    set_user_param(TCU_TYPE,      j["type"].get<uint>());
    set_user_param(REPEATED_FREQ, j["PRF"].get<double>());
    set_user_param(LASER_USR,     j["laser_width"].get<double>());
    set_user_param(DELAY_A,       j["delay_a"].get<double>());
    set_user_param(GATE_WIDTH_A,  j["gw_a"].get<double>());
    set_user_param(DELAY_B,       j["delay_b"].get<double>());
    set_user_param(GATE_WIDTH_B,  j["gw_b"].get<double>());
    set_user_param(CCD_FREQ,      j["ccd_freq"].get<double>());
    set_user_param(DUTY,          j["duty"].get<double>());
    set_user_param(MCP,           j["mcp"].get<uint>());
    set_user_param(DELAY_N,       j["delay_n"].get<double>());
    set_user_param(GATE_WIDTH_N,  j["gw_n"].get<double>());
    delay_dist = delay_a * dist_ns;
    depth_of_view = gate_width_a * dist_ns;
    emit tcu_param_updated(NO_PARAM);
}
#endif

void TCU::set_tcu_param(qint32 tcu_param, double val)
{
    uint integer_part, decimal_part;

    switch (tcu_type.load())
    {
        case 0:
        {
            integer_part = std::round(val);
            decimal_part = 0;

            QByteArray out(7, 0x00);
            out[0] = 0x88;
            out[1] = tcu_param;
            out[6] = 0x99;

            out[5] = integer_part & 0xFF; integer_part >>= 8;
            out[4] = integer_part & 0xFF; integer_part >>= 8;
            out[3] = integer_part & 0xFF; integer_part >>= 8;
            out[2] = integer_part & 0xFF;

            communicate(out, 1, 10);
            break;;
        }
        case 1:
        {
            TCU::PARAMS param_ps = TCU::PARAMS::NO_PARAM;
            int step = 0;
            switch (tcu_param) {
                case DELAY_A:
                    param_ps = DELAY_A_PS;
                    step = ps_step[0];
                    integer_part = int(val + 0.001) / 4;
                    decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
                    delay_a = integer_part * 4 + decimal_part * step / 1000.;
                    break;
                case DELAY_B:
                    param_ps = DELAY_B_PS;
                    step = ps_step[1];
                    integer_part = int(val + 0.001) / 4;
                    decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
                    delay_b = integer_part * 4 + decimal_part * step / 1000.;
                    break;
                case GATE_WIDTH_A:
                    param_ps = GW_A_PS;
                    step = ps_step[2];
                    integer_part = int(val + 0.001) / 4;
                    decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
                    gate_width_a = integer_part * 4 + decimal_part * step / 1000.;
                    break;
                case GATE_WIDTH_B:
                    param_ps = GW_B_PS;
                    step = ps_step[3];
                    integer_part = int(val + 0.001) / 4;
                    decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
                    gate_width_b = integer_part * 4 + decimal_part * step / 1000.;
                    break;
                default:
                    integer_part = int(val);
                    decimal_part = 0;
                    break;
            }

            QByteArray out_1(7, 0x00);
            out_1[0] = 0x88;
            out_1[1] = tcu_param;
            out_1[6] = 0x99;

            out_1[5] = integer_part & 0xFF; integer_part >>= 8;
            out_1[4] = integer_part & 0xFF; integer_part >>= 8;
            out_1[3] = integer_part & 0xFF; integer_part >>= 8;
            out_1[2] = integer_part & 0xFF;

            communicate(out_1, 7, 40);

            if (param_ps != TCU::PARAMS::NO_PARAM) {
                QByteArray out_2(7, 0x00);
                out_2[0] = 0x88;
                out_2[1] = param_ps;
                out_2[6] = 0x99;

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
            TCU::PARAMS param_ps = TCU::PARAMS::NO_PARAM;
            int step = 0;
            switch (tcu_param) {
            case DELAY_A:
                tcu_param = DELAY_B;
                param_ps = DELAY_B_PS;
                integer_part = int(val + 0.001);
                decimal_part = std::round((val - integer_part + 0.001) * 1000 / 11.23);
                delay_b = integer_part + decimal_part * 11.23 / 1000.;
                break;
            case LASER_WIDTH:
                tcu_param = GATE_WIDTH_A;
                param_ps = GW_A_PS;
                integer_part = int(val + 0.001);
                decimal_part = std::round((val - integer_part + 0.001) * 1000 / 14.9);
                gate_width_a = integer_part + decimal_part * 14.9 / 1000.;
                break;
            case GATE_WIDTH_B:
                tcu_param = GATE_WIDTH_B;
                param_ps = GW_B_PS;
                integer_part = int(val + 0.001);
                decimal_part = std::round((val - integer_part + 0.001) * 1000 / 14.9);
                gate_width_b = integer_part + decimal_part * 14.9 / 1000.;
                break;
            default:
                integer_part = int(val);
                decimal_part = 0;
                break;
            }

            QByteArray out_1(7, 0x00);
            out_1[0] = 0x88;
            out_1[1] = tcu_param;
            out_1[6] = 0x99;

            out_1[5] = integer_part & 0xFF; integer_part >>= 8;
            out_1[4] = integer_part & 0xFF; integer_part >>= 8;
            out_1[3] = integer_part & 0xFF; integer_part >>= 8;
            out_1[2] = integer_part & 0xFF;

            communicate(out_1, 7, 40);

            if (param_ps != TCU::PARAMS::NO_PARAM) {
                QByteArray out_2(7, 0x00);
                out_2[0] = 0x88;
                out_2[1] = param_ps;
                out_2[6] = 0x99;

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
