#ifndef LENS_H
#define LENS_H

#include "controlport.h"

class Lens : public ControlPort
{
    Q_OBJECT
public:
    enum PARAMS {
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
        RELAY1_ON     = 0x0D,
        RELAY1_OFF    = 0x0E,

        ADDRESS       = 0x100,
        STEPPING      = 0x101,

        NO_PARAM      = 0xFFFF,
    };

    explicit Lens(int index = -1, uchar address = 0x01, uint speed = 32);
    ~Lens();

    uint get(qint32 lens_param);

#if ENABLE_PORT_JSON
    nlohmann::json to_json() override;
#endif

signals:
    void lens_param_updated(qint32 lens_param, uint val);

protected slots:
    bool connect_to_serial_port(QString port_name, qint32 baudrate = QSerialPort::Baud9600) override;
    bool connect_to_tcp_port(QString ip, ushort port) override;
    void try_communicate() override;

    int lens_control(qint32 lens_param, uint val = 0);
    int lens_control_addr(qint32 lens_param, uchar addr, uchar speed = 0, uint val = 0);
    void set_pos_temp(qint32 lens_param, uint val);

#if ENABLE_PORT_JSON
    void load_from_json(const nlohmann::json &j) override;
#endif

private:
    uchar checksum(QByteArray data);

private:
    uchar address;
    uint  zoom;
    uint  focus;
    uint  laser_radius;
    uchar lens_speed;

    std::atomic<bool> hold;
    std::atomic<bool> interupt;

    QByteArray query[3];
};

#endif // LENS_H
