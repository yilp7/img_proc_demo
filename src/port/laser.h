#ifndef LASER_H
#define LASER_H

#include "controlport.h"

class Laser : public ControlPort
{
    Q_OBJECT
public:
    enum PARAMS {
        GET_STATUS    = 0x01,   // Query status
        GET_ENERGY    = 0x02,   // Query energy
        SYSTEM        = 0x10,   // System ON/OFF
        LASER         = 0x11,   // Laser ON/OFF
        SET_ENERGY    = 0x12,   // Set laser energy
        TRIGGER       = 0x15,   // Trigger ON/OFF

        NO_PARAM      = 0xFFFF,
    };

    explicit Laser(int index = -1, uint laser_type = 0);
    ~Laser();

    double get(qint32 laser_param);
    uint get_status();

#if ENABLE_PORT_JSON
    nlohmann::json to_json() override;
#endif

signals:
    void laser_param_updated(qint32 laser_param, double value);
    void laser_status_updated(bool system_on, bool laser_on, bool trigger_on);

public slots:
    void laser_control_slot(qint32 laser_param, uint value);  // Public slot for GUI connections

protected slots:
    bool connect_to_serial_port(QString port_name, qint32 baudrate = QSerialPort::Baud115200) override;
    bool connect_to_tcp_port(QString ip, ushort port) override;
    void try_communicate() override;

    int laser_control(QString msg);  // Keep existing function
    int laser_control(qint32 laser_param, bool enable);  // New overloaded function for ON/OFF commands
    int laser_control(qint32 laser_param, uint value);  // New overloaded function for value commands (energy level)

#if ENABLE_PORT_JSON
    void load_from_json(const nlohmann::json &j) override;
#endif

private:
    QByteArray build_command(uchar cmd, bool enable);
    QByteArray build_command(uchar cmd, uint value);
    uchar calculate_checksum(const QByteArray &data);
    double energy_level_to_mj(uint level);  // Helper to convert level to mJ

private:
    uint laser_type;

    // State tracking
    bool system_enabled;
    bool laser_enabled;
    bool trigger_enabled;
    uint laser_energy_level;  // Energy level 0-5 (0=5mJ, 1=30mJ, 2=50mJ, 3=100mJ, 4=130mJ, 5=155mJ)

    // Query command for status
    QByteArray query_status;
};

#endif // LASER_H
