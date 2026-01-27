#include "laser.h"

Laser::Laser(int index, uint laser_type) :
    ControlPort{index},
    laser_type(laser_type),
    system_enabled(false),
    laser_enabled(false),
    trigger_enabled(false),
    laser_energy_level(0)
{
    // Initialize query commands (4-byte format)
    query_status = QByteArray("\xAA\x01\x01\xAC", 4);  // AA 01 01 AC
}

Laser::~Laser()
{

}

double Laser::get(qint32 laser_param)
{
    switch (laser_param)
    {
        case GET_ENERGY: return energy_level_to_mj(laser_energy_level);
        case GET_STATUS: return (system_enabled ? 1 : 0) | (laser_enabled ? 2 : 0) | (trigger_enabled ? 4 : 0);
        default: return 0;
    }
}

uint Laser::get_status()
{
    return (system_enabled ? 1 : 0) | (laser_enabled ? 2 : 0) | (trigger_enabled ? 4 : 0);
}

// Public slot for GUI connections
void Laser::laser_control_slot(qint32 laser_param, uint value)
{
    switch (laser_param)
    {
        case SYSTEM:
        case LASER:
        case TRIGGER:
            // For ON/OFF commands, value is 0 or 1
            laser_control(laser_param, (bool)value);
            break;
        case SET_ENERGY:
            // For energy level, value is 0-5
            laser_control(laser_param, value);
            break;
        default:
            break;
    }
}

// New overloaded function for ON/OFF commands
int Laser::laser_control(qint32 laser_param, bool enable)
{
    QByteArray command;

    switch (laser_param)
    {
        case SYSTEM:
            command = build_command(SYSTEM, enable);
            system_enabled = enable;
            break;
        case LASER:
            command = build_command(LASER, enable);
            laser_enabled = enable;
            break;
        case TRIGGER:
            command = build_command(TRIGGER, enable);
            trigger_enabled = enable;
            break;
        default:
            return -1;
    }

    communicate(command, 0, 100);
    emit laser_status_updated(system_enabled, laser_enabled, trigger_enabled);
    return 0;
}

// New overloaded function for value commands (energy level)
int Laser::laser_control(qint32 laser_param, uint value)
{
    if (laser_param == SET_ENERGY)
    {
        if (value > 5) value = 5;  // Clamp to max level

        QByteArray command = build_command(SET_ENERGY, value);
        communicate(command, 0, 100);

        laser_energy_level = value;
        emit laser_param_updated(SET_ENERGY, energy_level_to_mj(value));
        return 0;
    }
    return -1;
}

bool Laser::connect_to_serial_port(QString port_name, qint32 baudrate)
{
    bool success = ControlPort::connect_to_serial_port(port_name, baudrate);

    if (success) {
        // Query initial status
        communicate(query_status, 4, 100);
    }

    return success;
}

bool Laser::connect_to_tcp_port(QString ip, ushort port)
{
    bool success = ControlPort::connect_to_tcp_port(ip, port);

    if (success) {
        // Query initial status
        communicate(query_status, 4, 100);
    }

    return success;
}

void Laser::try_communicate()
{
    ControlPort::try_communicate();

    // Periodically query status
    communicate(query_status, 4, 100, true);
    QByteArray read;
    retrieve_mutex.lock();
    read = last_read;
    retrieve_mutex.unlock();

    // TODO: Parse status response when protocol is known
}

// Build command for ON/OFF operations (8-byte format)
QByteArray Laser::build_command(uchar cmd, bool enable)
{
    QByteArray command(8, 0x00);
    command[0] = 0xAA;  // Header
    command[1] = 0x05;  // Length
    command[2] = cmd;   // Command code
    command[3] = enable ? 0x01 : 0x00;  // Data byte 1 (low byte)
    command[4] = 0x00;  // Data byte 2
    command[5] = 0x00;  // Data byte 3
    command[6] = 0x00;  // Data byte 4
    command[7] = calculate_checksum(command);
    return command;
}

// Build command for value operations (8-byte format)
QByteArray Laser::build_command(uchar cmd, uint value)
{
    QByteArray command(8, 0x00);
    command[0] = 0xAA;  // Header
    command[1] = 0x05;  // Length
    command[2] = cmd;   // Command code
    command[3] = value & 0xFF;          // Data byte 1 (low byte)
    command[4] = (value >> 8) & 0xFF;   // Data byte 2
    command[5] = (value >> 16) & 0xFF;  // Data byte 3
    command[6] = (value >> 24) & 0xFF;  // Data byte 4 (high byte)
    command[7] = calculate_checksum(command);
    return command;
}

// Calculate checksum (last byte of sum)
uchar Laser::calculate_checksum(const QByteArray &data)
{
    uint sum = 0;
    for (int i = 0; i < data.size() - 1; i++)  // Don't include the checksum byte itself
    {
        sum += (uchar)data[i];
    }
    return sum & 0xFF;  // Return last byte
}

// Convert energy level to millijoules
double Laser::energy_level_to_mj(uint level)
{
    switch (level)
    {
        case 0: return 5.0;
        case 1: return 30.0;
        case 2: return 50.0;
        case 3: return 100.0;
        case 4: return 130.0;
        case 5: return 155.0;
        default: return 0.0;
    }
}

#if ENABLE_PORT_JSON
nlohmann::json Laser::to_json()
{
    return nlohmann::json{
        {"type", laser_type},
        };
}

int Laser::laser_control(QString msg)
{
    communicate(msg.toLatin1(), 0, 100);

    QThread::msleep(100);

    return 0;
}
#endif

#if ENABLE_PORT_JSON
void Laser::load_from_json(const nlohmann::json &j)
{
    std::cout << j << std::endl;
    std::cout.flush();
}
#endif
