#ifndef CONTROLPORTTHREAD_H
#define CONTROLPORTTHREAD_H

#include "util/util.h"
#include "widgets/dotindicator.h"

struct ScanParams;

struct PortData {
    QByteArray write;
    int write_size;
    int read_size;
    bool fb;
    bool heartbeat;
    bool *completed;
    uchar **data_ptr;
};

class ControlPortThread : public QThread
{
    Q_OBJECT
public:
    enum PortStatus {
        NOT_CONNECTED    = 0x00,
        SERIAL_CONNECTED = 0x01,
        TCP_CONNECTED    = 0x10,
    };

    ControlPortThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon = nullptr, QObject *parent = nullptr);
    ~ControlPortThread();

signals:
    void port_connected(int connected_status);
    void port_io(QString data);
    void send_port_data(PortData port_data);

public slots:
    virtual void try_communicate();

    void setup_serial_port();
    bool setup_tcp_port(QString ip, int port, bool connect);

    QByteArray communicate_display(QByteArray write, int write_size, int read_size, bool fb = false, bool heartbeat = false, int read_timeout = 100);
    void communicate_display(PortData port_data);

public:
    QByteArray generate_ba(uchar *data, int len);
    int get_baudrate();
    bool set_baudrate(int baudrate);
    bool get_tcp_status();
    void set_tcp_status(bool use_tcp);
    void share_port_from(ControlPortThread *port);
    int get_port_status();
    void apply_color_mode();
    void quit_thread();
    void send_data(PortData d);

protected:
    void run() override;

// TODO: determine if thread implementation is needed
protected:
    bool m_thread_interrupt;

    QLabel      *m_lbl;
    QLineEdit   *m_edt;
    DotIndicator *m_status_icon;
    int          m_idx; // 0: TCU, 1: lens, 2: laser, 3: ptz, 4: inclinometer
    bool         m_connected_to_serial;
    bool         m_connected_to_tcp;
    bool         m_use_tcp;
    QMutex       m_port_mutex;
    QSerialPort *m_serial_port;
    QTcpSocket  *m_tcp_port;
    int          m_baudrate;
    int          m_shared_from; // -1: none, 0-3: refer to idx
    ControlPortThread *m_shared_port;

    std::queue<PortData> m_q;

    bool         m_hold;
    QTimer      *m_timer;
    int          m_time_interval;
    int          m_write_idx;
    bool         m_write_succeeded;
    int          m_count_threshold;
    int          m_successive_count;
};

struct TCUDataGroup {
    float rep_freq;
    float laser_width;
    float delay_a;
    float gate_width_a;
    float delay_b;
    float gate_width_b;
    float ccd_freq;
    float duty;
    uint  mcp;
};

class TCUThread : public ControlPortThread
{
    Q_OBJECT
public:
    enum UserParams {
        REPEATED_FREQ = 0x00,
        LASER_WIDTH   = 0x01,
        DELAY_A       = 0x02,
        GATE_WIDTH_A  = 0x03,
        DELAY_B       = 0x04,
        GATE_WIDTH_B  = 0x05,
        CCD_FREQ      = 0x06,
        DUTY          = 0x07,
        MCP           = 0x0A,
        LASER1        = 0x1A,
        LASER2        = 0x1B,
        LASER3        = 0x1C,
        LASER4        = 0x1D,
        DIST          = 0x1E,
        TOGGLE_LASER  = 0x1F,
        GW_A_PS       = 0x28,
        DELAY_A_PS    = 0x29,
        GW_B_PS       = 0x2A,
        DELAY_B_PS    = 0x2B,

        DELAY_N       = 0x0100,
        GATE_WIDTH_N  = 0x0101,
        LASER_USR     = 0x0102,
        EST_DIST      = 0x0103,
        EST_DOV       = 0x0104,
        LASER_ON      = 0x0105,

        NO_PARAM      = 0xFF,
    };

    TCUThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon = nullptr, ScanParams *sp = nullptr, QObject *parent = nullptr, uint init_width = 100);

    int set_user_param(TCUThread::UserParams param, float val);
    TCUDataGroup get_tcu_params();

public slots:
    void try_communicate() override;

private:
    int set_tcu_param(TCUThread::UserParams param, float val);

public:
    // 0: default, head(88) cmd(00) data(MM NN PP QQ) tail(99)
    // 1: step-50ps, head(88) cmd(00) data(MM NN PP QQ SS) tail(99), SS->step by 50ps (max 80)
    int   m_tcu_type;
    bool  m_scan_mode; // 0: default, 1: scan

    // TCUThread params
    float m_rep_freq;                     // unit: kHz
    float m_laser_width;                  // unit: ns
    float m_delay_a, m_delay_b;           // unit: ns
    float m_gate_width_a, m_gate_width_b; // unit: ns
    float m_ccd_freq;                     // unit: frame/s
    float m_duty;                         // unit: μs
    uint  m_mcp;
    uint  m_laser_on;
    uint  m_tcu_dist;                   // unit: m

    // user params
    float m_dist_ns;                    // unit: m/ns
    float m_delay_n;                    // unit: ns
    float m_gate_width_n;               // unit: ns
    float m_delay_dist;                 // unit: m
    float m_depth_of_view;              // unit: m

    // configuration
    bool  m_auto_rep_freq;
    bool  m_ab_lock;
    float m_laser_offset;
    float m_delay_offset;
    float m_gate_width_offset;
    int   m_ps_step[4];

    bool  m_use_gw_lut;
    int   m_gw_lut[100];                // lookup table for gatewidth config by serial

private:
    ScanParams *m_scan_params;
};

class LensThread : public ControlPortThread
{
    Q_OBJECT
public:
    enum Params {
        STOP          = 0x00,
        ZOOM_IN       = 0x01,
        ZOOM_OUT      = 0x02,
        FOCUS_FAR     = 0x03,
        FOCUS_NEAR    = 0x04,
        RADIUS_UP     = 0x05,
        RADIUS_DOWN   = 0x06,
        ZOOM_POS      = 0x07,
        FOCUS_POS     = 0x08,
        LASER_RADIUS  = 0x09,
        SET_ZOOM      = 0x0A,
        SET_FOCUS     = 0x0B,
        SET_RADIUS    = 0x0C,
        STEPPING      = 0x100,
        ADDRESS       = 0x101,
    };

    LensThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon = nullptr, QObject *parent = nullptr, int init_speed = 32);

    int lens_control(LensThread::Params command, uint *val = nullptr);

public slots:
    void try_communicate() override;

public:
    uint m_zoom;
    uint m_focus;
    uint m_laser_radius;
    uint m_lens_speed;
};

class LaserThread : public ControlPortThread
{
    Q_OBJECT
public:
    LaserThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon = nullptr, QObject *parent = nullptr);

public slots:
    void try_communicate() override;
};

class PTZThread : public ControlPortThread
{
    Q_OBJECT
public:
    enum Params {
        STOP       = 0x00,
        UP_LEFT    = 0x01,
        UP         = 0x02,
        UP_RIGHT   = 0x03,
        LEFT       = 0x04,
        SELF_CHECK = 0x05,
        RIGHT      = 0x06,
        DOWN_LEFT  = 0x07,
        DOWN       = 0x08,
        DOWN_RIGHT = 0x09,
        ANGLE_H    = 0x0A,
        ANGLE_V    = 0x0B,
        SET_H      = 0x0C,
        SET_V      = 0x0D,
        SPEED      = 0x100,
        ADDRESS    = 0x101,
    };

    PTZThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon = nullptr, QObject *parent = nullptr, int init_speed = 16);

    int ptz_control(PTZThread::Params param, float *val = nullptr);

public slots:
    void try_communicate() override;

private:
    void send_ctrl_cmd(uchar dir);

public:
    float m_angle_h;
    float m_angle_v;
    int   m_ptz_speed;
};

class InclinThread : public ControlPortThread
{
    Q_OBJECT
public:
    InclinThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon = nullptr, QObject *parent = nullptr);

public slots:
    void try_communicate() override;

private:
    // unlock: 0XFF 0XAA 0X69 0X88 0XB5
    // set output frequency: 0xFF 0xAA 0x03 0x03 0x00 (1Hz)
    // set output content: 0xFF 0xAA 0x02 0x0F 0x00 (TIME, ACC, GYRO, ANGLE)
    // save config: 0XFF 0XAA 0X00 0X00 0X00
    float m_acceleration_x;
    float m_acceleration_y;
    float m_acceleration_z;
    float m_temperature;

    float m_angular_x;
    float m_angular_y;
    float m_angular_z;

    float m_roll;
    float m_pitch;
    float m_yaw;
};

#endif // CONTROLPORTTHREAD_H
