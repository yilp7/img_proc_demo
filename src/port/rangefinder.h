#ifndef RANGEFINDER_H
#define RANGEFINDER_H

#include "controlport.h"

class RangeFinder : public ControlPort
{
    Q_OBJECT
public:
    enum PARAMS {
        SERIAL    = 0x00,
        FREQ_SET  = 0x01,
        FREQ_GET  = 0x02,
        BAUDRATE  = 0x03,

        NO_PARAM  = 0xFF,
    };

    explicit RangeFinder(int index = -1);
    ~RangeFinder();

    uint type();
    void set_type(uint type);

#if ENABLE_PORT_JSON
    nlohmann::json to_json() override;
    void load_from_json(const nlohmann::json &j);
#endif

signals:
    void distance_updated(double val);

protected slots:
    bool connect_to_serial_port(QString port_name, qint32 baudrate = QSerialPort::Baud9600) override;
    bool connect_to_tcp_port(QString ip, ushort port) override;
    void try_communicate() override;

    int rf_control(qint32 rf_param, uint val = 0);
    int read_distance(QByteArray data);

private:
    std::atomic<uint> rf_type;

    uint freq;
    uint baudrate;
};

#endif // RANGEFINDER_H
