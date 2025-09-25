#ifndef TCU_H
#define TCU_H

#include "controlport.h"

class TCU : public ControlPort
{
    Q_OBJECT
public:
    enum PARAMS {
        REPEATED_FREQ = 0x00,
        LASER_WIDTH   = 0x01,
        DELAY_A       = 0x02,
        GATE_WIDTH_A  = 0x03,
        DELAY_B       = 0x04,
        GATE_WIDTH_B  = 0x05,
        CCD_FREQ      = 0x06,
        DUTY          = 0x07,
        MCP           = 0x0A,
        LASER_1       = 0x1A,
        LASER_2       = 0x1B,
        LASER_3       = 0x1C,
        LASER_4       = 0x1D,
        DIST          = 0x1E,
        TOGGLE_LASER  = 0x1F,
        GW_A_PS       = 0x28,
        DELAY_A_PS    = 0x29,
        GW_B_PS       = 0x2A,
        DELAY_B_PS    = 0x2B,
        DELAY_C       = 0x1C,
        GATE_WIDTH_C  = 0x1D,
        DELAY_D       = 0x20,
        GATE_WIDTH_D  = 0x21,
        DELAY_E       = 0x1B,
        GATE_WIDTH_E  = 0x22,

        NO_PARAM      = 0xFF,

        DELAY_N       = 0x0100,
        GATE_WIDTH_N  = 0x0101,
        LASER_USR     = 0x0102,
        EST_DIST      = 0x0103,
        EST_DOV       = 0x0104,
        LASER_ON      = 0x0105,
        REP_FREQ_SCAN = 0x0106,
        OFFSET_DELAY  = 0x0107,
        OFFSET_GW     = 0x0108,
        OFFSET_LASER  = 0x0109,
        PS_STEP_1     = 0x010A,
        PS_STEP_2     = 0x010B,
        PS_STEP_3     = 0x010C,
        PS_STEP_4     = 0x010D,
        AUTO_PRF      = 0x010E,
        LIGHT_SPEED   = 0x010F,
        OFFSET_ND     = 0x0110,
        OFFSET_NG     = 0x0111,
        AB_LOCK       = 0x0112,

        TCU_TYPE      = 0xFFFF,
    };

    explicit TCU(int index = -1, int tcu_type = 0);
    ~TCU();

    double get(qint32 tcu_param);
    uint type();
    void set_type(uint type);

#if ENABLE_PORT_JSON
    nlohmann::json to_json() override;
#endif

signals:
    void tcu_param_updated(qint32 tcu_param);

protected slots:
    bool connect_to_serial_port(QString port_name, qint32 baudrate = QSerialPort::Baud9600) override;
    bool connect_to_tcp_port(QString ip, ushort port) override;
    void try_communicate() override;

    void set_user_param(qint32 tcu_param, double val);
    void set_user_param(qint32 tcu_param, uint val);

#if ENABLE_PORT_JSON
    void load_from_json(const nlohmann::json &j) override;
#endif

private:
    void set_tcu_param(qint32 tcu_param, double val);

private:
    // 0: default, head(88) cmd(00) data(MM NN PP QQ) tail(99)
    // 1: step-40ps, head(88) cmd(00) data(MM NN PP QQ) tail(99), ps cmd step by 40ps (max 100)
    // 1: step-10ps, head(88) cmd(00) data(MM NN PP QQ) tail(99), ps cmd step by 10ps (max 100)
    std::atomic<uint> tcu_type;
    bool   scan_mode; // 0: default, 1: scan

    // TCUThread params
    double rep_freq;                   // unit: kHz
    double laser_width;                // unit: ns
    double delay_a, delay_b;           // unit: ns
    double gate_width_a, gate_width_b; // unit: ns
    double delay_c, delay_d;           // unit: ns
    double gate_width_c, gate_width_d; // unit: ns
    double ccd_freq;                   // unit: frame/s
    double duty;                       // unit: μs
    uint   mcp;
    uint   laser_on;
    uint   tcu_dist;                   // unit: m

    // user params
    double dist_ns;                    // unit: m/ns
    double delay_n;                    // unit: ns
    double gate_width_n;               // unit: ns
    double delay_dist;                 // unit: m
    double depth_of_view;              // unit: m
    double rep_freq_scan;              // unit: kHz

    // configuration
    bool   auto_rep_freq;
    bool   ab_lock;
    double laser_offset;
    double delay_offset;
    double gate_width_offset;
    uint   ps_step[4];

    bool   use_gw_lut;
    int    gw_lut[100];                // lookup table for gatewidth config by serial

    std::atomic<bool> hold;
    std::atomic<bool> interupt;

    QByteArray query;
};

#endif // TCU_H
