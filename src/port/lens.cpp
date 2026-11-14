#include "lens.h"
#include "pelco_protocol.h"

Lens::Lens(int index, uchar address, uint speed) :
    ControlPort{index},
    m_address(address),
    m_zoom(0),
    m_focus(0),
    m_laser_radius(0),
    m_lens_speed(speed),
    m_hold(false),
    m_interupt(false)
{
    uchar q0[] = {SYNC_BYTE, address, 0x00, LENS_QUERY_ZOOM, 0x00, 0x00, 0x00};
    m_query[0] = QByteArray((char*)q0, 7);
    m_query[0][6] = checksum(m_query[0]);
    uchar q1[] = {SYNC_BYTE, address, 0x00, LENS_QUERY_FOCUS, 0x00, 0x00, 0x00};
    m_query[1] = QByteArray((char*)q1, 7);
    m_query[1][6] = checksum(m_query[1]);
    uchar q2[] = {SYNC_BYTE, address, 0x00, LENS_QUERY_RADIUS, 0x00, 0x00, 0x00};
    m_query[2] = QByteArray((char*)q2, 7);
    m_query[2][6] = checksum(m_query[2]);
}

Lens::~Lens()
{

}

uint Lens::get(qint32 lens_param)
{
    switch (lens_param) {
    case ZOOM_POS:     return m_zoom;
    case FOCUS_POS:    return m_focus;
    case LASER_RADIUS: return m_laser_radius;
    case STEPPING:     return m_lens_speed;
    default: return 0;
    }
}

#if ENABLE_PORT_JSON
nlohmann::json Lens::to_json()
{
    return nlohmann::json{
        {"address", m_address},
        {"zoom", m_zoom},
        {"focus", m_focus},
        {"laser_radius", m_laser_radius},
        {"speed", m_lens_speed},
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
    
    communicate(m_query[write_idx], 7, READ_TIMEOUT_MS, true);
    QByteArray read;
    m_retrieve_mutex.lock();
    read = m_last_read;
    m_retrieve_mutex.unlock();

    if (read.size() != 7) m_successive_count = 0;
    uint lens_fb = (uchar(read[4]) << 8) + uchar(read[5]);
    switch (write_idx) {
    case 0:
        if (read[3] == char(LENS_REPLY_ZOOM)) m_successive_count++, emit lens_param_updated(Lens::ZOOM_POS, m_zoom = lens_fb);
        else                                m_successive_count = 0;
        break;
    case 1:
        if (read[3] == char(LENS_REPLY_FOCUS)) m_successive_count++, emit lens_param_updated(Lens::FOCUS_POS, m_focus = lens_fb);
        else                                  m_successive_count = 0;
        break;
    case 2:
        if (read[3] == char(LENS_REPLY_RADIUS)) m_successive_count++, emit lens_param_updated(Lens::LASER_RADIUS, m_laser_radius = lens_fb);
        else                       m_successive_count = 0;
        break;
    default: break;
    }

    write_idx = (write_idx + 1) % 3;
}

int Lens::lens_control(qint32 lens_param, uint val)
{
    QByteArray command;
    switch (lens_param) {
    case STOP:         { uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_CMD_STOP, 0x00, 0x00, 0x00}; command = QByteArray((char*)buf, 7); goto chksm; }
    case ZOOM_IN:      { uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_CMD_ZOOM_IN, 0x00, m_lens_speed, 0x00}; command = QByteArray((char*)buf, 7); goto chksm; }
    case ZOOM_OUT:     { uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_CMD_ZOOM_OUT, 0x00, m_lens_speed, 0x00}; command = QByteArray((char*)buf, 7); goto chksm; }
    case FOCUS_FAR:    { uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_CMD_FOCUS_FAR, 0x00, m_lens_speed, 0x00}; command = QByteArray((char*)buf, 7); goto chksm; }
    case FOCUS_NEAR:   { uchar buf[] = {SYNC_BYTE, m_address, LENS_CMD_FOCUS_NEAR, 0x00, 0x00, m_lens_speed, 0x00}; command = QByteArray((char*)buf, 7); goto chksm; }
    case RADIUS_UP:    { uchar buf[] = {SYNC_BYTE, m_address, LENS_CMD_RADIUS_UP, 0x00, 0x00, m_lens_speed, 0x00}; command = QByteArray((char*)buf, 7); goto chksm; }
    case RADIUS_DOWN:  { uchar buf[] = {SYNC_BYTE, m_address, LENS_CMD_RADIUS_DOWN, 0x00, 0x00, m_lens_speed, 0x00}; command = QByteArray((char*)buf, 7); goto chksm; }
    case ZOOM_POS:     command = m_query[0]; goto send;
    case FOCUS_POS:    command = m_query[1]; goto send;
    case LASER_RADIUS: command = m_query[2]; goto send;
    case SET_ZOOM:
    {
        uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_SET_ZOOM, uchar((val >> 8) & 0xFF), uchar(val & 0xFF), 0x00};
        command = QByteArray((char*)buf, 7);
        m_zoom = val;
        goto chksm;
    }
    case SET_FOCUS:
    {
        uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_SET_FOCUS, uchar((val >> 8) & 0xFF), uchar(val & 0xFF), 0x00};
        command = QByteArray((char*)buf, 7);
        m_focus = val;
        goto chksm;
    }
    case SET_RADIUS:
    {
        uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_SET_RADIUS, uchar((val >> 8) & 0xFF), uchar(val & 0xFF), 0x00};
        command = QByteArray((char*)buf, 7);
        m_laser_radius = val;
        goto chksm;
    }
    case RELAY1_ON:
    {
        // Pelco-D Aux 1 ON: byte 5 is the aux number (1). With 0x00 the device ignores the frame.
        uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_RELAY_ON, 0x00, 0x01, 0x00};
        command = QByteArray((char*)buf, 7);
        goto chksm;
    }
    case RELAY1_OFF:
    {
        // Pelco-D Aux 1 OFF: byte 5 = aux number (1).
        uchar buf[] = {SYNC_BYTE, m_address, 0x00, LENS_RELAY_OFF, 0x00, 0x01, 0x00};
        command = QByteArray((char*)buf, 7);
        goto chksm;
    }
    case STEPPING:
    {
        m_lens_speed = std::min(std::max((uchar)val, uchar(MIN_SPEED)), uchar(MAX_SPEED));  return 0;
    }
    case ADDRESS: m_address = val; return 0;
    default: return -11111;
    }

chksm:
    command[6] = checksum(command);
send:
    if (m_hold.load()) return -1;
    switch (lens_param) {
    case ZOOM_POS:
    case FOCUS_POS:
    case LASER_RADIUS:
    {
        communicate(command, 7, READ_TIMEOUT_MS);
        QByteArray read;
        m_retrieve_mutex.lock();
        read = m_last_read;
        m_retrieve_mutex.unlock();

        uint lens_fb = (uchar(read[4]) << 8) + uchar(read[5]);
        switch (lens_param) {
        case ZOOM_POS:     emit lens_param_updated(lens_param, m_zoom = lens_fb); break;
        case FOCUS_POS:    emit lens_param_updated(lens_param, m_focus = lens_fb); break;
        case LASER_RADIUS: emit lens_param_updated(lens_param, m_laser_radius = lens_fb); break;
        }
        break;
    }
    default:
        communicate(command, 0, READ_TIMEOUT_MS);
        break;
    }
    return 0;
}

int Lens::lens_control_addr(qint32 lens_param, uchar addr, uchar speed, uint val)
{
    QByteArray command;
    switch (lens_param) {
    case STOP:         { uchar buf[] = {SYNC_BYTE, addr, 0x00, LENS_CMD_STOP, 0x00,  0x00, 0x00}; command = QByteArray((char*)buf, 7); break; }
    case ZOOM_IN:      { uchar buf[] = {SYNC_BYTE, addr, 0x00, LENS_CMD_ZOOM_IN, 0x00, speed, 0x00}; command = QByteArray((char*)buf, 7); break; }
    case ZOOM_OUT:     { uchar buf[] = {SYNC_BYTE, addr, 0x00, LENS_CMD_ZOOM_OUT, 0x00, speed, 0x00}; command = QByteArray((char*)buf, 7); break; }
    case FOCUS_FAR:    { uchar buf[] = {SYNC_BYTE, addr, 0x00, LENS_CMD_FOCUS_FAR, 0x00, speed, 0x00}; command = QByteArray((char*)buf, 7); break; }
    case FOCUS_NEAR:   { uchar buf[] = {SYNC_BYTE, addr, LENS_CMD_FOCUS_NEAR, 0x00, 0x00, speed, 0x00}; command = QByteArray((char*)buf, 7); break; }
    case RADIUS_UP:    { uchar buf[] = {SYNC_BYTE, addr, LENS_CMD_RADIUS_UP, 0x00, 0x00, speed, 0x00}; command = QByteArray((char*)buf, 7); break; }
    case RADIUS_DOWN:  { uchar buf[] = {SYNC_BYTE, addr, LENS_CMD_RADIUS_DOWN, 0x00, 0x00, speed, 0x00}; command = QByteArray((char*)buf, 7); break; }
    default: return -11111;
    }

    command[6] = checksum(command);
    communicate(command, 0, READ_TIMEOUT_MS);
    return 0;
}

void Lens::set_pos_temp(qint32 lens_prarm, uint val)
{
    if (lens_prarm != Lens::ZOOM_POS && lens_prarm != Lens::FOCUS_POS && lens_prarm != Lens::LASER_RADIUS) return;
    QByteArray query_cmd;
    char flag = 0x00;
    switch (lens_prarm) {
    case Lens::ZOOM_POS:
        if (val > m_zoom) lens_control(Lens::ZOOM_IN);
        else              lens_control(Lens::ZOOM_OUT);
        query_cmd = m_query[0];
        flag = LENS_REPLY_ZOOM;
        break;
    case Lens::FOCUS_POS:
        if (val > m_focus) lens_control(Lens::FOCUS_FAR);
        else               lens_control(Lens::FOCUS_NEAR);
        query_cmd = m_query[1];
        flag = LENS_REPLY_FOCUS;
        break;
    case Lens::LASER_RADIUS:
        if (val < m_laser_radius) lens_control(Lens::RADIUS_UP);
        else                      lens_control(Lens::RADIUS_DOWN);
        query_cmd = m_query[2];
        flag = LENS_REPLY_RADIUS;
        break;
    }
    QThread::msleep(5);
    m_hold.store(true);
    m_timer->stop();
    while (!m_interupt.load()) {
        communicate(query_cmd, 7, READ_TIMEOUT_MS);
        QByteArray read;
        m_retrieve_mutex.lock();
        read = m_last_read;
        m_retrieve_mutex.unlock();
        if (read.size() != 7 || read[3] != flag) continue;
        uint lens_fb = (uchar(read[4]) << 8) + uchar(read[5]);
//        if (abs(int(lens_fb) - int(val)) / float(val) < 0.01) break;
        if (abs(int(lens_fb) - int(val)) < 3) break;
        QThread::msleep(40);
    }
    m_hold.store(false);
    lens_control(Lens::STOP);
    m_timer->start();
}

#if ENABLE_PORT_JSON
void Lens::load_from_json(const nlohmann::json &j)
{
    cout_mutex.lock();
    std::cout << j << std::endl;
    std::cout.flush();
    cout_mutex.unlock();
    
    try {
        if (j.contains("address") && j["address"].is_number())
            lens_control(ADDRESS, j["address"].get<uchar>());
        if (j.contains("speed") && j["speed"].is_number())  
            lens_control(STEPPING, j["speed"].get<uchar>());
        if (j.contains("zoom") && j["zoom"].is_number())
            lens_control(SET_ZOOM, j["zoom"].get<uint>());
        if (j.contains("focus") && j["focus"].is_number())
            lens_control(SET_FOCUS, j["focus"].get<uint>());
        if (j.contains("laser_radius") && j["laser_radius"].is_number())
            lens_control(SET_RADIUS, j["laser_radius"].get<uint>());
    }
    catch (const nlohmann::json::exception &e) {
        qWarning("JSON parsing error in lens configuration: %s", e.what());
    }
    
    emit lens_param_updated(Lens::NO_PARAM, 0);
}
#endif

inline uchar Lens::checksum(QByteArray data)
{
    uint sum = 1;
    for (char c : data) sum += uchar(c);
    return sum & 0xFF;
}
