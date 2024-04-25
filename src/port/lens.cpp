#include "lens.h"

Lens::Lens(int index, uchar address, uint speed) :
    ControlPort{index},
    address(address),
    zoom(0),
    focus(0),
    laser_radius(0),
    lens_speed(speed),
    hold(false),
    interupt(false)
{
    query[0] = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x55, 0x00, 0x00, 0x00}, 7);
    query[0][6] = checksum(query[0]);
    query[1] = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x56, 0x00, 0x00, 0x00}, 7);
    query[1][6] = checksum(query[1]);
    query[2] = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x57, 0x00, 0x00, 0x00}, 7);
    query[2][6] = checksum(query[2]);
}

Lens::~Lens()
{

}

uint Lens::get(qint32 lens_param)
{
    switch (lens_param)
    {
        case ZOOM_POS:     return zoom;
        case FOCUS_POS:    return focus;
        case LASER_RADIUS: return laser_radius;
        case STEPPING:     return lens_speed;
        default: return 0;
        }
}

#if ENABLE_PORT_JSON
nlohmann::json Lens::to_json()
{
    return nlohmann::json{
        {"address", address},
        {"zoom", zoom},
        {"focus", focus},
        {"laser_radius", laser_radius},
        {"speed", lens_speed},
    };
}
#endif

bool Lens::connect_to_serial_port(QString port_name, qint32 baudrate)
{
    bool success = ControlPort::connect_to_serial_port(port_name, baudrate);

    if (success) {
        lens_control(Lens::STOP);
        lens_control(Lens::ZOOM_POS);
        lens_control(Lens::FOCUS_POS);
        lens_control(Lens::LASER_RADIUS);
    }

    return success;
}

bool Lens::connect_to_tcp_port(QString ip, ushort port)
{
    bool success = ControlPort::connect_to_tcp_port(ip, port);

    if (success) {
        lens_control(Lens::STOP);
        lens_control(Lens::ZOOM_POS);
        lens_control(Lens::FOCUS_POS);
        lens_control(Lens::LASER_RADIUS);
    }

    return success;
}

void Lens::try_communicate()
{
    ControlPort::try_communicate();

    static int write_idx = 0;
    
    communicate(query[write_idx], 7, 40, true);
    QByteArray read;
    retrieve_mutex.lock();
    read = last_read;
    retrieve_mutex.unlock();

    if (read.size() != 7) successive_count = 0;
    uint lens_fb = (uchar(read[4]) << 8) + uchar(read[5]);
    switch (write_idx)
    {
        case 0:
            if (read[3] == char(0x5D)) successive_count++, emit lens_param_updated(Lens::ZOOM_POS, zoom = lens_fb);
            else                       successive_count = 0;
            break;
        case 1:
            if (read[3] == char(0x5E)) successive_count++, emit lens_param_updated(Lens::FOCUS_POS, focus = lens_fb);
            else                       successive_count = 0;
            break;
        case 2:
            if (read[3] == char(0x70)) successive_count++, emit lens_param_updated(Lens::LASER_RADIUS, laser_radius = lens_fb);
            else                       successive_count = 0;
            break;
        default: break;
    }

    write_idx = (write_idx + 1) % 3;
}

int Lens::lens_control(qint32 lens_prarm, uint val)
{
    QByteArray command;
    switch (lens_prarm)
    {
        case STOP:         command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x00, 0x00, 0x00, 0x00}, 7); goto chksm;
        case ZOOM_IN:      command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x20, 0x00, lens_speed, 0x00}, 7); goto chksm;
        case ZOOM_OUT:     command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x40, 0x00, lens_speed, 0x00}, 7); goto chksm;
        case FOCUS_FAR:    command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x80, 0x00, lens_speed, 0x00}, 7); goto chksm;
        case FOCUS_NEAR:   command = generate_ba(new uchar[7]{0xFF, address, 0x01, 0x00, 0x00, lens_speed, 0x00}, 7); goto chksm;
        case RADIUS_UP:    command = generate_ba(new uchar[7]{0xFF, address, 0x02, 0x00, 0x00, lens_speed, 0x00}, 7); goto chksm;
        case RADIUS_DOWN:  command = generate_ba(new uchar[7]{0xFF, address, 0x04, 0x00, 0x00, lens_speed, 0x00}, 7); goto chksm;
        case ZOOM_POS:     command = query[0]; goto send;
        case FOCUS_POS:    command = query[1]; goto send;
        case LASER_RADIUS: command = query[2]; goto send;
        case SET_ZOOM:
        {
            command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x4F, uchar((val >> 8) & 0xFF), uchar(val & 0xFF), 0x00}, 7);
            zoom = val;
            goto chksm;
        }
        case SET_FOCUS:
        {
            command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x4E, uchar((val >> 8) & 0xFF), uchar(val & 0xFF), 0x00}, 7);
            focus = val;
            goto chksm;
        }
        case SET_RADIUS:
        {
            command = generate_ba(new uchar[7]{0xFF, address, 0x00, 0x81, uchar((val >> 8) & 0xFF), uchar(val & 0xFF), 0x00}, 7);
            laser_radius = val;
            goto chksm;
        }
        case STEPPING:
        {
#if 0
            lens_speed = val;
            uchar out_data[9] = {0};
            out_data[0] = 0xB0;
            out_data[1] = 0x01;
            out_data[2] = 0xA0;
            out_data[3] = 0x01;
            out_data[4] = lens_speed;
            out_data[5] = lens_speed;
            out_data[6] = lens_speed;
            out_data[7] = lens_speed;
            out_data[8] = (4 * uint(lens_speed) + 0xA2) & 0xFF;

            command = QByteArray((char*)out_data, 9);
            goto send;
#endif
            lens_speed = std::min(std::max((uchar)val, uchar(0x01)), uchar(0x3F)); return 0;
        }
        case ADDRESS: address = val; return 0;
        default: return -11111;
    }

chksm:
    command[6] = checksum(command);
send:
    if (hold.load()) return -1;
    switch (lens_prarm)
    {
        case ZOOM_POS:
        case FOCUS_POS:
        case LASER_RADIUS:
        {
            communicate(command, 7, 40);
            QByteArray read;
            retrieve_mutex.lock();
            read = last_read;
            retrieve_mutex.unlock();

            uint lens_fb = (uchar(read[4]) << 8) + uchar(read[5]);
            switch (lens_prarm)
            {
                case ZOOM_POS:     emit lens_param_updated(lens_prarm, zoom = lens_fb); break;
                case FOCUS_POS:    emit lens_param_updated(lens_prarm, focus = lens_fb); break;
                case LASER_RADIUS: emit lens_param_updated(lens_prarm, laser_radius = lens_fb); break;
            }
            break;
        }
        default:
            communicate(command, 0, 40);
            break;
    }
    return 0;
}

int Lens::lens_control_addr(qint32 lens_prarm, uchar addr, uchar speed, uint val)
{
    QByteArray command;
    switch (lens_prarm)
    {
        case STOP:         command = generate_ba(new uchar[7]{0xFF, addr, 0x00, 0x00, 0x00,  0x00, 0x00}, 7); break;
        case ZOOM_IN:      command = generate_ba(new uchar[7]{0xFF, addr, 0x00, 0x20, 0x00, speed, 0x00}, 7); break;
        case ZOOM_OUT:     command = generate_ba(new uchar[7]{0xFF, addr, 0x00, 0x40, 0x00, speed, 0x00}, 7); break;
        case FOCUS_FAR:    command = generate_ba(new uchar[7]{0xFF, addr, 0x00, 0x80, 0x00, speed, 0x00}, 7); break;
        case FOCUS_NEAR:   command = generate_ba(new uchar[7]{0xFF, addr, 0x01, 0x00, 0x00, speed, 0x00}, 7); break;
        case RADIUS_UP:    command = generate_ba(new uchar[7]{0xFF, addr, 0x02, 0x00, 0x00, speed, 0x00}, 7); break;
        case RADIUS_DOWN:  command = generate_ba(new uchar[7]{0xFF, addr, 0x04, 0x00, 0x00, speed, 0x00}, 7); break;
        default: return -11111;
    }

    command[6] = checksum(command);
    communicate(command, 0, 40);
    return 0;
}

void Lens::set_pos_temp(qint32 lens_prarm, uint val)
{
    if (lens_prarm != Lens::ZOOM_POS && lens_prarm != Lens::FOCUS_POS && lens_prarm != Lens::LASER_RADIUS) return;
    QByteArray query_cmd;
    char flag = 0x00;
    switch (lens_prarm)
    {
        case Lens::ZOOM_POS:
            if (val > zoom) lens_control(Lens::ZOOM_IN);
            else            lens_control(Lens::ZOOM_OUT);
            query_cmd = query[0];
            flag = 0x5D;
            break;
        case Lens::FOCUS_POS:
            if (val > focus) lens_control(Lens::FOCUS_FAR);
            else             lens_control(Lens::FOCUS_NEAR);
            query_cmd = query[1];
            flag = 0x5E;
            break;
        case Lens::LASER_RADIUS:
            if (val < laser_radius) lens_control(Lens::RADIUS_UP);
            else                    lens_control(Lens::RADIUS_DOWN);
            query_cmd = query[2];
            flag = 0x70;
            break;
    }
    QThread::msleep(5);
    hold.store(true);
    timer->stop();
    while (!interupt.load()) {
        communicate(query_cmd, 7, 40);
        QByteArray read;
        retrieve_mutex.lock();
        read = last_read;
        retrieve_mutex.unlock();
        if (read.size() != 7 || read[3] != flag) continue;
        uint lens_fb = (uchar(read[4]) << 8) + uchar(read[5]);
//        if (abs(int(lens_fb) - int(val)) / float(val) < 0.01) break;
        if (abs(int(lens_fb) - int(val)) < 3) break;
        QThread::msleep(40);
    }
    hold.store(false);
    lens_control(Lens::STOP);
    timer->start();
}

#if ENABLE_PORT_JSON
void Lens::load_from_json(const nlohmann::json &j)
{
    cout_mutex.lock();
    std::cout << j << std::endl;
    std::cout.flush();
    cout_mutex.unlock();
    lens_control(ADDRESS,      j["address"].get<uchar>());
    lens_control(STEPPING,     j["speed"].get<uchar>());
//    set_pos_temp(ZOOM_POS,     j["zoom"].get<uint>());
    set_pos_temp(FOCUS_POS,    j["focus"].get<uint>());
//    set_pos_temp(LASER_RADIUS, j["laser_radius"].get<uint>());
//    lens_control(SET_ZOOM,     j["zoom"].get<uint>());
//    lens_control(SET_FOCUS,    j["focus"].get<uint>());
//    lens_control(SET_RADIUS,   j["laser_radius"].get<uint>());
    emit lens_param_updated(Lens::NO_PARAM, 0);
}
#endif

inline uchar Lens::checksum(QByteArray data)
{
    uint sum = 1;
    for (char c : data) sum += uchar(c);
    return sum & 0xFF;
}
