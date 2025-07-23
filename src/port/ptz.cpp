#include "ptz.h"

PTZ::PTZ(int index, uchar address, uint speed) :
    ControlPort{index},
    address(address),
    angle_h(0),
    angle_v(0),
    ptz_speed(speed)
{
    query[0] = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x51, 0x00, 0x00, 0x00}, 7);
    query[0][6] = checksum(query[0]);
    query[1] = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x53, 0x00, 0x00, 0x00}, 7);
    query[1][6] = checksum(query[1]);
}

PTZ::~PTZ()
{

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

    communicate(query[write_idx], 7, 40, true);
    QByteArray read;
    retrieve_mutex.lock();
    read = last_read;
    retrieve_mutex.unlock();

    if (read.size() != 7) successive_count = 0;
    int angle_fb = (uchar(read[4]) << 8) + uchar(read[5]);
    switch (write_idx)
    {
        case 0:
            if (read[3] == char(0x59)) {
                successive_count++;
                angle_fb %= 36000;
                double temp_angle = angle_fb / 100.;
                // Ensure horizontal angle is always positive (0 to 360)
                temp_angle = temp_angle < 0 ? temp_angle + 360.0 : temp_angle;
                emit ptz_param_updated(PTZ::ANGLE_H, angle_h = temp_angle);
            }
            else successive_count = 0;
            break;
        case 1:
            if (read[3] == char(0x5B))
            {
                successive_count++;
                angle_fb = (angle_fb + 4000) % 36000 - 4000;
                angle_fb = std::min(std::max(angle_fb, -4000), 4000);
                emit ptz_param_updated(PTZ::ANGLE_V, angle_v = angle_fb / 100.);
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
        case STOP:       command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x00, 0x00, 0x00, 0x00}, 7); goto chksm;
        case UP_LEFT:    command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x0C, ptz_speed, ptz_speed, 0x00}, 7); goto chksm;
        case UP:         command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x08,      0x00, ptz_speed, 0x00}, 7); goto chksm;
        case UP_RIGHT:   command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x0A, ptz_speed, ptz_speed, 0x00}, 7); goto chksm;
        case LEFT:       command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x04, ptz_speed,      0x00, 0x00}, 7); goto chksm;
        case SELF_CHECK: command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x07, 0x00, 0x77, 0x00}, 7); goto chksm;
        case RIGHT:      command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x02, ptz_speed,      0x00, 0x00}, 7); goto chksm;
        case DOWN_LEFT:  command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x14, ptz_speed, ptz_speed, 0x00}, 7); goto chksm;
        case DOWN:       command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x10,      0x00, ptz_speed, 0x00}, 7); goto chksm;
        case DOWN_RIGHT: command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x12, ptz_speed, ptz_speed, 0x00}, 7); goto chksm;
        case ANGLE_H:
        case ANGLE_V:
        {
            uchar buffer_out[7] = {0};
            buffer_out[0] = 0xFF;
            buffer_out[1] = address;
            buffer_out[2] = 0x00;
            buffer_out[3] = ptz_param == ANGLE_H ? 0x51 : 0x53;
            buffer_out[4] = 0x00;
            buffer_out[5] = 0x00;
            buffer_out[6] = ptz_param == ANGLE_H ? 0x52 : 0x54;
            command = QByteArray((char*)buffer_out, 7);
            break;
        }
        case SET_H:
        {
            int angle = (int((angle_h = val) * 100) % 36000 + 36000) % 36000;
            uchar buffer_out[7] = {0};
            buffer_out[0] = 0xFF;
            buffer_out[1] = address;
            buffer_out[2] = 0x00;
            buffer_out[3] = 0x4B;
            buffer_out[4] = (angle >> 8) & 0xFF;
            buffer_out[5] = angle & 0xFF;
            buffer_out[6] = 0x00;
            command = QByteArray((char*)buffer_out, 7);
            break;
        }
        case SET_V:
        {
            int angle = (int(std::min(std::max(angle_v = val, -40.), 40.) * 100) % 36000 + 36000) % 36000;
            uchar buffer_out[7] = {0};
            buffer_out[0] = 0xFF;
            buffer_out[1] = address;
            buffer_out[2] = 0x00;
            buffer_out[3] = 0x4D;
            buffer_out[4] = (angle >> 8) & 0xFF;
            buffer_out[5] = angle & 0xFF;
            buffer_out[6] = 0x00;
            command = QByteArray((char*)buffer_out, 7);
            break;
        }
        case SPEED: ptz_speed = std::min(std::max((uchar)val, uchar(0x01)), uchar(0x3F)); return 0;
        case ADDRESS: address = val; return 0;
        default: return -11111;
    }
chksm:
    command[6] = checksum(command);
send:
    switch (ptz_param)
    {
        case ANGLE_H:
        case ANGLE_V:
        {
            communicate(command, 7, 40);
            QByteArray read;
            retrieve_mutex.lock();
            read = last_read;
            retrieve_mutex.unlock();

            int angle = (uchar(read[4]) << 8) + uchar(read[5]);
            switch (ptz_param)
            {
                case ANGLE_H:
                    angle %= 36000;
                    {
                        double temp_angle = angle / 100.;
                        // Ensure horizontal angle is always positive (0 to 360)
                        temp_angle = temp_angle < 0 ? temp_angle + 360.0 : temp_angle;
                        emit ptz_param_updated(ptz_param, angle_h = temp_angle);
                    }
                    break;
                case ANGLE_V:
                    angle = (angle + 4000) % 36000 - 4000;
                    angle = std::min(std::max(angle, -4000), 4000);
                    emit ptz_param_updated(ptz_param, angle_v = angle / 100.);
                    break;
            }
            break;
        }
        default:
            communicate(command, 0, 40);
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
    command[0] = 0xFF;
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
