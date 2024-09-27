#include "rangefinder.h"

RangeFinder::RangeFinder(int index) {}

RangeFinder::~RangeFinder() {}

#if ENABLE_PORT_JSON
nlohmann::json RangeFinder::to_json() { return nlohmann::json(); }
void RangeFinder::load_from_json(const nlohmann::json &j){}
#endif

bool RangeFinder::connect_to_serial_port(QString port_name, qint32 baudrate)
{
    return false;
}

bool RangeFinder::connect_to_tcp_port(QString ip, ushort port)
{
    bool success = ControlPort::connect_to_tcp_port(ip, port);

    return success;
}

void RangeFinder::try_communicate()
{
    ControlPort::try_communicate();

    communicate(QByteArray(), 20, 100, false);
    QByteArray read;
    retrieve_mutex.lock();
    read = last_read;
    retrieve_mutex.unlock();

    read_distance(read.mid(read.indexOf(0x5C), 4));
//    if (read.size() != 1 || read[0] != char(0x15)) successive_count = 0;
//    else                                           successive_count++;
}

inline void check_sum(QByteArray command)
{
    uint sum = 0xA5;
    for (uchar c : command) sum += c;
    command[command.length() - 1] = ~uchar(sum & 0xFF);
}

int RangeFinder::rf_control(qint32 rf_param, uint val)
{
    QByteArray command;
    switch (rf_type.load()) {
    case 0:
        switch (rf_param) {
        case SERIAL:
            command = generate_ba(new uchar[6]{0x5A, 0x0D, 0x02, 0x0D, 0x0D, 0x00}, 6);
            goto send;
        case FREQ_SET:
        {
            freq = val;
            uint param = 1e6 / freq - 1;
            command = generate_ba(new uchar[6]{0x5A, 0x0B, 0x02, uchar(param & 0xFF), uchar((param >> 8) & 0xFF), 0x00}, 6);
            goto send;
        }
        case FREQ_GET:
        {
            command = generate_ba(new uchar[6]{0x5A, 0x1B, 0x02, 0x1B, 0x1B, 0x00}, 6);
            goto send;
        }
        case BAUDRATE:
        {
            baudrate = val;
            uint param = val / 100;
            command = generate_ba(new uchar[6]{0x5A, 0x06, 0x02, uchar(param & 0xFF), uchar((param >> 8) & 0xFF), 0x00}, 6);
            goto send;
        }
        case NO_PARAM: goto read;
        default: break;
        }
        break;
    case 1:
        break;
    }

send:
    check_sum(command);
    communicate(command, 6, 40);

read:
    QByteArray read;
    retrieve_mutex.lock();
    read = last_read;
    retrieve_mutex.unlock();

    return 0;
}

int RangeFinder::read_distance(QByteArray data)
{
    if (data.length() != 4) return -1;
    if (!data.startsWith(0x5C)) return -2;

    uint distance = (uint(data[2]) << 8) + uint(data[1]);
    uchar checksum = uchar(data[1]) + uchar(data[2]);
    if (~checksum != data[3]) return -3;

    emit distance_updated(distance / 100.);
    return distance;
}
