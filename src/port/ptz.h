#ifndef PTZ_H
#define PTZ_H

#include "controlport.h"
#include "iptzcontroller.h"

class PTZ : public ControlPort, public IPTZController
{
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
        ANGLE_H    = 0x0A,
        ANGLE_V    = 0x0B,
        SET_H      = 0x0C,
        SET_V      = 0x0D,
        SPEED      = 0x100,
        ADDRESS    = 0x101,
        NO_PARAM   = 0xFFFF,
    };

    explicit PTZ(int index = -1, uchar address = 0x01, uint speed = 16);
    ~PTZ();

    double get(qint32 ptz_param);

    // IPTZController interface
    void ptz_move(int direction, int speed) override;
    void ptz_stop() override;
    void ptz_set_angle(float h, float v) override;
    void ptz_set_angle_h(float h) override;
    void ptz_set_angle_v(float v) override;
    float ptz_get_angle_h() const override;
    float ptz_get_angle_v() const override;
    bool ptz_is_connected() const override;
    QObject* ptz_qobject() override { return this; }

#if ENABLE_PORT_JSON
    nlohmann::json to_json() override;
#endif

signals:
    void ptz_param_updated(qint32 ptz_param, double val);
    void angle_updated(float _h, float _v);

protected slots:
    bool connect_to_serial_port(QString port_name, qint32 baudrate = QSerialPort::Baud9600) override;
    bool connect_to_tcp_port(QString ip, ushort port) override;
    void try_communicate() override;

    int ptz_control(qint32 ptz_param, double val = 0);

#if ENABLE_PORT_JSON
    void load_from_json(const nlohmann::json &j) override;
#endif

private:
    void send_ctrl_cmd(uchar dir);
    uchar checksum(QByteArray data);

private:
    uchar  address;
    double angle_h;
    double angle_v;
    uchar  ptz_speed;
    
    QByteArray query[2];
};

#endif // PTZ_H
