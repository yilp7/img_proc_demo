#ifndef LASER_H
#define LASER_H

#include "controlport.h"

class Laser : public ControlPort
{
    Q_OBJECT
public:
    explicit Laser(int index = -1, uint laser_type = 0);
    ~Laser();

    QByteArray get_laser_status();

#if ENABLE_PORT_JSON
    nlohmann::json to_json() override;
#endif

protected slots:
//    bool connect_to_serial_port(QString port_name, QSerialPort::BaudRate baudrate = QSerialPort::Baud9600) override;
//    bool connect_to_tcp_port(QString ip, ushort port) override;
//    void try_communicate() override;

    int laser_control(QString msg);

#if ENABLE_PORT_JSON
    void load_from_json(const nlohmann::json &j) override;
#endif

private:
    uint laser_type;
};

#endif // LASER_H
