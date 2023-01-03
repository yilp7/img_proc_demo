#ifndef CONTROLPORT_H
#define CONTROLPORT_H

#include "utils.h"
#include "mywidget.h"

class ControlPort : public QObject
{
    Q_OBJECT
public:
    ControlPort(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, QObject *parent = nullptr);
    ~ControlPort();

signals:
    void port_connected(int connected_status);
    void port_io(QString data);

public slots:
    virtual void try_communicate() = 0;

    void setup_serial_port();
    void setup_tcp_port(QString ip, int port);

// WARNING: will become public after a full transfer of com communication
//protected:
public:
    QByteArray generate_ba(uchar *data, int len);
    int get_baudrate();
    bool set_baudrate(int baudrate);
    QByteArray communicate_display(QByteArray write, int write_size, int read_size, bool fb = false, bool display = true);
    void share_port_from(ControlPort *port);

    void set_theme();

// TODO: determine if thread implementation is needed
protected:
    QLabel*      lbl;
    QLineEdit*   edt;
    StatusIcon*  status_icon;
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
    enum TCU_PARAMS {
        REPEATED_FREQ = 0x00,
        LASER_WIDTH   = 0x01,
        DELAY_A       = 0x02,
        GATE_WIDTH_A  = 0x03,
        DELAY_B       = 0x04,
        GATE_WIDTH_B  = 0x05,
        CCD_FREQ      = 0x06,
        TIME_EXPO     = 0x07,
        MCP           = 0x0A,
        LASER1        = 0x1A,
        LASER2        = 0x1B,
        LASER3        = 0x1C,
        LASER4        = 0x1D,
        DIST          = 0x1E,
        TOGGLE_LASER  = 0x1F,
    };

    enum USER_PARAMS {
        DELAY_N       = 0x0100,
        EST_DIST      = 0x0101,
        EST_DOV       = 0x0102,
        LASER_ON      = 0x0103,
    };

    TCU(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, QObject *parent = nullptr, uint init_width = 500);

    bool set_tcu_param(TCU::TCU_PARAMS param, uint val);
    bool set_user_param(TCU::USER_PARAMS param, uint val);
    bool set_user_param(TCU::USER_PARAMS param, float val);

public slots:
    void try_communicate() override;

private:
    // TCU params
    float rep_freq;
    uint  laser_width_u, laser_width_n;
    uint  delay_a_u, delay_a_n, delay_b_u, delay_b_n;
    uint  gate_width_a_u, gate_width_a_n, gate_width_b_u, gate_width_b_n;
    uint  fps, duty;
    uint  mcp;
    uint  laser_on;

    // user params
    float dist_ns;
    uint  delay_n_n;
    uint  delay_dist;
    uint  depth_of_view;
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

    Lens(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, QObject *parent = nullptr, int init_speed = 32);

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
    Laser(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, QObject *parent = nullptr);

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

    PTZ(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon = nullptr, QObject *parent = nullptr, int init_speed = 16);

public slots:
    void try_communicate() override;

private:
    float angle_h;
    float angle_v;
    int   ptz_speed;
};

#endif // CONTROLPORT_H
