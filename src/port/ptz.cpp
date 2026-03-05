#include "ptz.h"
#include "pelco_protocol.h"

PTZ::PTZ(int index, uchar address, uint speed) :
    ControlPort{index},
    address(address),
    angle_h(0),
    angle_v(0),
    ptz_speed(speed)
{
    uchar q0[] = {SYNC_BYTE, address, 0x00, PTZ_QUERY_H, 0x00, 0x00, 0x00};
    query[0] = QByteArray((char*)q0, 7);
    query[0][6] = checksum(query[0]);
    uchar q1[] = {SYNC_BYTE, address, 0x00, PTZ_QUERY_V, 0x00, 0x00, 0x00};
    query[1] = QByteArray((char*)q1, 7);
    query[1][6] = checksum(query[1]);
}

PTZ::~PTZ()
{

}

// IPTZController interface

void PTZ::ptz_move(int direction, int speed)
{
    ptz_speed = std::min(std::max((uchar)speed, uchar(MIN_SPEED)), uchar(MAX_SPEED));
    // Direction enum values map directly to PARAMS enum (direction + 1 for Pelco-D)
    ptz_control(direction + 1);
}

void PTZ::ptz_stop()
{
    ptz_control(STOP);
}

void PTZ::ptz_set_angle(float h, float v)
{
    ptz_control(SET_H, h);
    ptz_control(SET_V, v);
}

void PTZ::ptz_set_angle_h(float h) { ptz_control(SET_H, h); }
void PTZ::ptz_set_angle_v(float v) { ptz_control(SET_V, v); }

float PTZ::ptz_get_angle_h() const { return (float)angle_h; }
float PTZ::ptz_get_angle_v() const { return (float)angle_v; }

bool PTZ::ptz_is_connected() const
{
    return get_port_status() & (ControlPort::SERIAL_CONNECTED | ControlPort::TCP_CONNECTED);
}

double PTZ::get(qint32 ptz_param)
{
    switch (ptz_param)
    {
        case ANGLE_H: return angle_h;
        case ANGLE_V: return angle_v;
        case SPEED:   return ptz_speed;
        default: return 0;
        }
}

#if ENABLE_PORT_JSON
nlohmann::json PTZ::to_json()
{
    return nlohmann::json{
        {"address", address},
        {"angle_h", angle_h},
        {"angle_v", angle_v},
        {"speed", ptz_speed},
    };
}
#endif

bool PTZ::connect_to_serial_port(QString port_name, qint32 baudrate)
{
    bool success = ControlPort::connect_to_serial_port(port_name, baudrate);

    if (success) {
        ptz_control(PTZ::STOP);
        ptz_control(PTZ::ANGLE_H);
        ptz_control(PTZ::ANGLE_V);
    }

    return success;
}

bool PTZ::connect_to_tcp_port(QString ip, ushort port)
{
    bool success = ControlPort::connect_to_tcp_port(ip, port);

    if (success) {
        ptz_control(PTZ::STOP);
        ptz_control(PTZ::ANGLE_H);
        ptz_control(PTZ::ANGLE_V);
    }

    return success;
}

void PTZ::try_communicate()
{
    ControlPort::try_communicate();

    static int write_idx = 0;

    communicate(query[write_idx], 7, READ_TIMEOUT_MS, true);
    QByteArray read;
    retrieve_mutex.lock();
    read = last_read;
    retrieve_mutex.unlock();

    if (read.size() != 7) successive_count = 0;
    int angle_fb = (uchar(read[4]) << 8) + uchar(read[5]);
    switch (write_idx)
    {
        case 0:
            if (read[3] == char(PTZ_REPLY_H)) {
                successive_count++;
                angle_fb %= FULL_ROTATION;
                double temp_angle = angle_fb / (double)ANGLE_SCALE;
                // Ensure horizontal angle is always positive (0 to 360)
                temp_angle = temp_angle < 0 ? temp_angle + 360.0 : temp_angle;
                emit ptz_param_updated(PTZ::ANGLE_H, angle_h = temp_angle);
                emit angle_updated((float)angle_h, (float)angle_v);
            }
            else successive_count = 0;
            break;
        case 1:
            if (read[3] == char(PTZ_REPLY_V))
            {
                successive_count++;
                angle_fb = (angle_fb + VERTICAL_LIMIT) % FULL_ROTATION - VERTICAL_LIMIT;
                angle_fb = std::min(std::max(angle_fb, -VERTICAL_LIMIT), VERTICAL_LIMIT);
                emit ptz_param_updated(PTZ::ANGLE_V, angle_v = angle_fb / (double)ANGLE_SCALE);
                emit angle_updated((float)angle_h, (float)angle_v);
            }
            else successive_count = 0;
            break;
        default: break;
    }

    write_idx = (write_idx + 1) % 2;
}

int PTZ::ptz_control(qint32 ptz_param, double val)
{
    QByteArray command;
    switch(ptz_param)
    {
        case STOP:       { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_STOP,       0x00,                  0x00,                  0x00}; command = QByteArray((char*)buf, 7); break; }
        case UP_LEFT:    { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_UP_LEFT,    ptz_speed,             ptz_speed,             0x00}; command = QByteArray((char*)buf, 7); break; }
        case UP:         { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_UP,         0x00,                  ptz_speed,             0x00}; command = QByteArray((char*)buf, 7); break; }
        case UP_RIGHT:   { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_UP_RIGHT,   ptz_speed,             ptz_speed,             0x00}; command = QByteArray((char*)buf, 7); break; }
        case LEFT:       { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_LEFT,       ptz_speed,             0x00,                  0x00}; command = QByteArray((char*)buf, 7); break; }
        case SELF_CHECK: { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_SELF_CHECK, 0x00,                  PTZ_SELF_CHECK_DATA, 0x00}; command = QByteArray((char*)buf, 7); break; }
        case RIGHT:      { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_RIGHT,      ptz_speed,             0x00,                  0x00}; command = QByteArray((char*)buf, 7); break; }
        case DOWN_LEFT:  { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_DOWN_LEFT,  ptz_speed,             ptz_speed,             0x00}; command = QByteArray((char*)buf, 7); break; }
        case DOWN:       { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_DOWN,       0x00,                  ptz_speed,             0x00}; command = QByteArray((char*)buf, 7); break; }
        case DOWN_RIGHT: { uchar buf[] = {SYNC_BYTE, address, 0x00, PTZ_DIR_DOWN_RIGHT, ptz_speed,             ptz_speed,             0x00}; command = QByteArray((char*)buf, 7); break; }
        case ANGLE_H:
        case ANGLE_V:
        {
            uchar buffer_out[7] = {0};
            buffer_out[0] = SYNC_BYTE;
            buffer_out[1] = address;
            buffer_out[2] = 0x00;
            buffer_out[3] = ptz_param == ANGLE_H ? PTZ_QUERY_H : PTZ_QUERY_V;
            buffer_out[4] = 0x00;
            buffer_out[5] = 0x00;
            buffer_out[6] = ptz_param == ANGLE_H ? 0x52 : 0x54;
            command = QByteArray((char*)buffer_out, 7);
            break;
        }
        case SET_H:
        {
            int angle = (int((angle_h = val) * ANGLE_SCALE) % FULL_ROTATION + FULL_ROTATION) % FULL_ROTATION;
            uchar buffer_out[7] = {0};
            buffer_out[0] = SYNC_BYTE;
            buffer_out[1] = address;
            buffer_out[2] = 0x00;
            buffer_out[3] = PTZ_SET_H;
            buffer_out[4] = (angle >> 8) & 0xFF;
            buffer_out[5] = angle & 0xFF;
            buffer_out[6] = 0x00;
            command = QByteArray((char*)buffer_out, 7);
            break;
        }
        case SET_V:
        {
            int angle = (int(std::min(std::max(angle_v = val, -VERTICAL_DEG), VERTICAL_DEG) * ANGLE_SCALE) % FULL_ROTATION + FULL_ROTATION) % FULL_ROTATION;
            uchar buffer_out[7] = {0};
            buffer_out[0] = SYNC_BYTE;
            buffer_out[1] = address;
            buffer_out[2] = 0x00;
            buffer_out[3] = PTZ_SET_V;
            buffer_out[4] = (angle >> 8) & 0xFF;
            buffer_out[5] = angle & 0xFF;
            buffer_out[6] = 0x00;
            command = QByteArray((char*)buffer_out, 7);
            break;
        }
        case SPEED: ptz_speed = std::min(std::max((uchar)val, uchar(MIN_SPEED)), uchar(MAX_SPEED)); return 0;
        case ADDRESS: address = val; return 0;
        default: return -11111;
    }
    command[6] = checksum(command);
    switch (ptz_param)
    {
        case ANGLE_H:
        case ANGLE_V:
        {
            communicate(command, 7, READ_TIMEOUT_MS);
            QByteArray read;
            retrieve_mutex.lock();
            read = last_read;
            retrieve_mutex.unlock();

            int angle = (uchar(read[4]) << 8) + uchar(read[5]);
            switch (ptz_param)
            {
                case ANGLE_H:
                    angle %= FULL_ROTATION;
                    {
                        double temp_angle = angle / (double)ANGLE_SCALE;
                        // Ensure horizontal angle is always positive (0 to 360)
                        temp_angle = temp_angle < 0 ? temp_angle + 360.0 : temp_angle;
                        emit ptz_param_updated(ptz_param, angle_h = temp_angle);
                        emit angle_updated((float)angle_h, (float)angle_v);
                    }
                    break;
                case ANGLE_V:
                    angle = (angle + VERTICAL_LIMIT) % FULL_ROTATION - VERTICAL_LIMIT;
                    angle = std::min(std::max(angle, -VERTICAL_LIMIT), VERTICAL_LIMIT);
                    emit ptz_param_updated(ptz_param, angle_v = angle / (double)ANGLE_SCALE);
                    emit angle_updated((float)angle_h, (float)angle_v);
                    break;
            }
            break;
        }
        default:
            communicate(command, 0, READ_TIMEOUT_MS);
            break;
    }
        return 0;
}

#if ENABLE_PORT_JSON
void PTZ::load_from_json(const nlohmann::json &j)
{
    cout_mutex.lock();
    std::cout << j << std::endl;
    std::cout.flush();
    cout_mutex.unlock();

    try {
        if (j.contains("address") && j["address"].is_number())
            ptz_control(ADDRESS, j["address"].get<uchar>());
        if (j.contains("speed") && j["speed"].is_number())
            ptz_control(SPEED, j["speed"].get<uchar>());
        if (j.contains("angle_h") && j["angle_h"].is_number())
            ptz_control(SET_H, j["angle_h"].get<double>());
        if (j.contains("angle_v") && j["angle_v"].is_number())
            ptz_control(SET_V, j["angle_v"].get<double>());
    }
    catch (const nlohmann::json::exception& e) {
        qWarning("JSON parsing error in PTZ configuration: %s", e.what());
    }

    emit ptz_param_updated(PTZ::NO_PARAM, 0);
}
#endif

void PTZ::send_ctrl_cmd(uchar dir)
{
    QByteArray command(7, 0x00);
    command[0] = SYNC_BYTE;
    command[1] = 0x01;
    command[2] = 0x00;
    command[3] = dir;
    command[4] = dir < 5 ? ptz_speed : 0x00;
    command[5] = dir > 5 ? ptz_speed : 0x00;
    command[6] = checksum(command);

    communicate(command, 0, 0);
}

inline uchar PTZ::checksum(QByteArray data)
{
    uint sum = 1;
    for (char c : data) sum += uchar(c);
    return sum & 0xFF;
}
