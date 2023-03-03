#ifndef CONTROLPORT_H
#define CONTROLPORT_H

#include "utils.h"
#include "mywidget.h"

class ControlPort : public QObject
{
    Q_OBJECT
public:
    ControlPort(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, Preferences *pref = nullptr, QObject *parent = nullptr);
    ~ControlPort();

signals:
    void port_connected(int connected_status);
    void port_io(QString data);

public slots:
    virtual void try_communicate() = 0;

    void setup_serial_port();
    bool setup_tcp_port(QString ip, int port, bool connect);

public:
    QByteArray generate_ba(uchar *data, int len);
    int get_baudrate();
    bool set_baudrate(int baudrate);
    QByteArray communicate_display(QByteArray write, int write_size, int read_size, bool fb = false, bool display = true);
    void share_port_from(ControlPort *port);
    int status();

    void set_theme();

// TODO: determine if thread implementation is needed
protected:
    QLabel*      lbl;
    QLineEdit*   edt;
    StatusIcon*  status_icon;
    Preferences* pref;
    int          idx; // 0: TCU, 1: lens, 2: laser, 3: ptz
    bool         connected_to_serial;
    bool         connected_to_tcp;
    bool         use_tcp;
    QMutex       port_mutex;
    QSerialPort* serial_port;
    QTcpSocket*  tcp_port;
    int          baudrate;
    int          shared_from; // -1: none, 0-3: refer to idx
    ControlPort* shared_port;

    QTimer*      timer;
    int          time_interval;
    bool         write_succeeded;
    int          count_threshold;
    int          successive_count;
};

class TCU : public ControlPort {
    Q_OBJECT
public:
    enum USER_PARAMS {
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

        DELAY_N       = 0x0100,
        GATE_WIDTH_N  = 0x0101,
        EST_DIST      = 0x0102,
        EST_DOV       = 0x0103,
        LASER_ON      = 0x0104,
    };

    TCU(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, Preferences *pref = nullptr,
        ScanConfig *sc = nullptr, QObject *parent = nullptr, uint init_width = 500);

    int set_user_param(TCU::USER_PARAMS param, float val);

public slots:
    void try_communicate() override;

private:
    int set_tcu_param(TCU::USER_PARAMS param, float val);

public:
    // 0: default, head(88) cmd(00) data(MM NN PP QQ) tail(99)
    // 1: step-50ps, head(88) cmd(00) data(MM NN PP QQ SS) tail(99), SS->step by 50ps (max 80)
    int   tcu_type;
    bool  scan_mode; // 0: default, 1: scan

    // TCU params
    float rep_freq;                   // unit: kHz
    float laser_width;                // unit: ns
    float delay_a, delay_b;           // unit: ns
    float gate_width_a, gate_width_b; // unit: ns
    float ccd_freq;                        // unit: frame/s
    float duty;                       // unit: ns
    uint  mcp;
    uint  laser_on;
    uint  tcu_dist;                   // unit: m

    // user params
    float dist_ns;                    // unit: m/ns
    float delay_n;                    // unit: ns
    float gate_width_n;               // unit: ns
    float delay_dist;                 // unit: m
    float depth_of_view;              // unit: m

    int   gw_lut[100];                // lookup table for gatewidth config by serial

private:
    ScanConfig *scan_config;
};

class Lens : public ControlPort {
    Q_OBJECT
public:
    enum PARAMS {
        STOP          = 0x00,
        ZOOM_IN       = 0x01,
        ZOOM_OUT      = 0x02,
        FOCUS_FAR     = 0x03,
        FOCUS_NEAR    = 0x04,
        ENLARGE_LASER = 0x05,
        NARROW_LASER  = 0x06,
        ZOOM_POS      = 0x07,
        FOCUS_POS     = 0x08,
        LASER_POS     = 0x09,
        SET_ZOOM      = 0x0A,
        SET_FOCUS     = 0x0B,
        SET_LASER     = 0x0C,
        STEPPING      = 0x100,
        ADDRESS       = 0x101,
    };

    Lens(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, Preferences *pref = nullptr, QObject *parent = nullptr, int init_speed = 32);

public slots:
    void try_communicate() override;

private:
    uint zoom;
    uint focus;
    uint laser;
    uint speed;
};

class Laser : public ControlPort {
    Q_OBJECT
public:
    Laser(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, Preferences *pref = nullptr, QObject *parent = nullptr);

public slots:
    void try_communicate() override;
};

class PTZ : public ControlPort {
    Q_OBJECT
public:
    enum PARAMS {
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
        ANG_H      = 0x0A,
        ANG_V      = 0x0B,
        SET_H      = 0x0C,
        SET_V      = 0x0D,
        STEPPING   = 0x100,
        ADDRESS    = 0x101,
    };

    PTZ(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, Preferences *pref = nullptr, QObject *parent = nullptr, int init_speed = 16);

public slots:
    void try_communicate() override;

private:
    float angle_h;
    float angle_v;
    int   ptz_speed;
};

#endif // CONTROLPORT_H
