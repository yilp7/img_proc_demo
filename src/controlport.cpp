#include "controlport.h"
#include "userpanel.h"

ControlPort::ControlPort(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent) :
    QThread{parent},
    thread_interrupt(false),
    connected_to_serial(false),
    connected_to_tcp(false),
    use_tcp(false),
    baudrate(QSerialPort::Baud9600),
    shared_from(-1),
    shared_port(NULL),
    time_interval(1000),
    write_idx(0),
    write_succeeded(false),
    count_threshold(5),
    successive_count(0)
{
    //[0] set up ui pointers with info structure
    this->lbl = label;
    this->edt = edit;
    this->idx = index;
    this->status_icon = status_icon;
    //![0]

    //[1] set up timer for communication status check
    timer = new QTimer(this);
//    connect(timer, SIGNAL(timeout()), this, SLOT(try_communicate()));
    connect(timer, &QTimer::timeout, this, &ControlPort::try_communicate);
    // TODO: change bool param to int
    connect(timer, &QTimer::timeout, this,
        [this](){
            // status: -1: not connected, 0: ok, 1: write successful
            static StatusIcon::STATUS last_status;
            last_status = StatusIcon::STATUS::CONNECTED;
            if (!connected_to_serial && !connected_to_tcp) last_status = StatusIcon::STATUS::NOT_CONNECTED;
            if (successive_count > 5) successive_count = 5, last_status = write_succeeded ? StatusIcon::STATUS::RECONNECTING : StatusIcon::DISCONNECTED;
//            emit port_connected(last_status);
            if (this->status_icon) this->status_icon->update_status(last_status);
        });
    timer->start(time_interval);
//    if (idx == 4) qDebug() << "init" << QThread::currentThread();
    //![1]

    //[2] setup serialport / tcpsocket for communication
    serial_port = new QSerialPort(this);
    tcp_port = new QTcpSocket(this);
    connect(edt, &QLineEdit::returnPressed, this, &ControlPort::setup_serial_port);
//    connect(parent, &UserPanel::connect_to_tcp_port, this, [this](QString ip, int port){});
    //![2]
}

ControlPort::~ControlPort() {}

void ControlPort::setup_serial_port()
{
    if (serial_port->isOpen()) serial_port->close();

    QString port_name = "COM" + edt->text();
    serial_port->setPortName(port_name);
//    qDebug("%p", *com);

    if (serial_port->open(QIODevice::ReadWrite)) {
        QThread::msleep(10);
        serial_port->clear();
        qDebug("%s connected", qPrintable(port_name));
        connected_to_serial = true;
        lbl->setStyleSheet(app_theme ? "color: #180D1C;" : "color: #B0C4DE;");

        serial_port->setBaudRate(baudrate);
        serial_port->setDataBits(QSerialPort::Data8);
        serial_port->setParity(QSerialPort::NoParity);
        serial_port->setStopBits(QSerialPort::OneStop);
        serial_port->setFlowControl(QSerialPort::NoFlowControl);

        connected_to_serial = true;
        successive_count = 0;

        // send initial data
        switch (idx) {
        case 4:
            q.push(PortData{generate_ba(new uchar[5]{0XFF, 0XAA, 0X69, 0X88, 0XB5}, 5), 5, 1, true, true});
            q.push(PortData{generate_ba(new uchar[5]{0xFF, 0xAA, 0x03, 0x03, 0x00}, 5), 5, 1, true, true});
            q.push(PortData{generate_ba(new uchar[5]{0xFF, 0xAA, 0x02, 0x0F, 0x00}, 5), 5, 1, true, true});
            q.push(PortData{generate_ba(new uchar[5]{0XFF, 0XAA, 0X00, 0X00, 0X00}, 5), 5, 1, true, true});
            break;
        default:
            break;
        }
    }
    else {
        connected_to_serial = false;
        lbl->setStyleSheet("color: #CD5C5C;");
    }

    // TODO: test for user utilities
    FILE *f = fopen("user_default", "rb+");
    if (!f) return;
    uchar port_num_uchar = edt->text().toUInt() & 0xFF;
    fseek(f, 4 + idx, SEEK_SET);
    fwrite(&port_num_uchar, 1, 1, f);
    fclose(f);
}

bool ControlPort::setup_tcp_port(QString ip, int port, bool connect)
{
    if (connect) {
        tcp_port->connectToHost(ip, port);
        return use_tcp = connected_to_tcp = tcp_port->waitForConnected(1000);
    }
        tcp_port->disconnectFromHost();
        return use_tcp = connected_to_tcp = false;
}

inline QByteArray ControlPort::generate_ba(uchar *data, int len)
{
    QByteArray ba((char*)data, len);
    delete[] data;
    return ba;
}

int ControlPort::get_baudrate()
{
    return serial_port->baudRate();
}

bool ControlPort::set_baudrate(int baudrate)
{
    return serial_port->setBaudRate(baudrate);
}

bool ControlPort::get_tcp_status()
{
    return use_tcp;
}

void ControlPort::set_tcp_status(bool use_tcp)
{
    this->use_tcp = use_tcp;
}

// send and receive data from any ports, and display in main window
QByteArray ControlPort::communicate_display(QByteArray write, int write_size, int read_size, bool fb, bool heartbeat) {
    if (shared_port) return shared_port->communicate_display(write, write_size, read_size, fb, heartbeat);

//    port_mutex.lock();
    QByteArray read;
    QString str_s("sent    "), str_r("received");

    for (char i = 0; i < write_size; i++) str_s += QString::asprintf(" %02X", (uchar)write[i]);
    if (!heartbeat) emit port_io(str_s);

    QIODevice *port = use_tcp ? (QIODevice*)tcp_port : (QIODevice*)serial_port;

    if (!port->isOpen()) {
//        port_mutex.unlock();
        return QByteArray();
    }
    port->readAll();
    port->write(write.constData(), write_size);
    write_succeeded = port->waitForBytesWritten(5);

    if (fb) port->waitForReadyRead(40);

    read = read_size ? port->read(read_size) : port->readAll();
//    read = port->readAll();
    for (char i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
    if (!heartbeat) emit port_io(str_r);

//    port_mutex.unlock();
    QThread().msleep(5);
    return read;
}

void ControlPort::communicate_display(PortData port_data)
{
    QByteArray read = communicate_display(port_data.write, port_data.write_size, port_data.read_size, port_data.fb, port_data.heartbeat);
    if (port_data.heartbeat) {
        switch (idx) {
        case 0:
            qDebug() << "TCU heartbeat" << read.toHex(' ').toUpper();
            if (read != QByteArray(1, 0x15)) successive_count++;
            else                             successive_count = 0;
            break;
        case 1:
            qDebug() << "lens heartbeat" << read.toHex(' ').toUpper();
            break;
        case 2:
            qDebug() << "laser heartbeat" << read.toHex(' ').toUpper();
            break;
        case 3:
            qDebug() << "PTZ heartbeat" << read.toHex(' ').toUpper();
            break;
        case 4: {
            static float g = 9.8f;
            (uchar)read[0] == 0x55;
            (uchar)read[1] == 0x50;
            read[2], read[10];
            (uchar)read[11] == 0x55;
            (uchar)read[12] == 0x51;
            float acceleration_x = 16 * g * (uint(read[14] << 8) + read[13]) / 32768.;
            float acceleration_y = 16 * g * (uint(read[16] << 8) + read[15]) / 32768.;
            float acceleration_z = 16 * g * (uint(read[18] << 8) + read[17]) / 32768.;
            read[19], read[21];
            (uchar)read[22] == 0x55;
            (uchar)read[23] == 0x52;
            float angular_x = 2000 * (uint(read[25] << 8) + read[24]) / 32768.;
            float angular_y = 2000 * (uint(read[27] << 8) + read[26]) / 32768.;
            float angular_z = 2000 * (uint(read[29] << 8) + read[28]) / 32768.;
            read[30], read[32];
            (uchar)read[33] == 0x55;
            (uchar)read[34] == 0x53;
            float roll  = 180 * (uint(read[36] << 8) + read[35]) / 32768.;
            float pitch = 180 * (uint(read[38] << 8) + read[37]) / 32768.;
            float yaw   = 180 * (uint(read[40] << 8) + read[39]) / 32768.;
            read[41], read[43];
            qDebug() << roll << pitch << yaw;
        }
        default: break;
        }
    }
}

void ControlPort::share_port_from(ControlPort *port)
{
    shared_port = port;
    shared_from = port ? port->idx : -1;
}

int ControlPort::get_port_status()
{
    return (connected_to_tcp ? PortStatus::TCP_CONNECTED : PortStatus::NOT_CONNECTED) |
           (connected_to_serial ? PortStatus::SERIAL_CONNECTED : PortStatus::NOT_CONNECTED);
}

void ControlPort::set_theme()
{
    lbl->setStyleSheet(connected_to_serial ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;");
}

void ControlPort::quit_thread()
{
    thread_interrupt = true;
    quit();
    wait();
}

void ControlPort::run()
{
    qRegisterMetaType<PortData>("PortData");
    connect(this, SIGNAL(send_port_data(PortData)), this, SLOT(communicate_display(PortData)));
    while (!thread_interrupt) {
        if (q.empty()) { QThread::msleep(10); continue; }
        PortData temp = q.front();
        q.pop();
        QByteArray read, read_byte(2, 0x00);
        if (temp.write.isEmpty()) {
            QIODevice *port = use_tcp ? (QIODevice*)tcp_port : (QIODevice*)serial_port;
            if (!port->isOpen()) continue;
//            static QByteArray head = generate_ba(new uchar[2]{0x55, 0x50}, 2);
//            do {
//                read_byte[0] = read_byte[1];
//                read_byte.append(port->read(1));
//            } while (read_byte != head);
//            read = read_byte + port->read(42);
            read = port->read(44);
//            qDebug() << read.toHex(',');
            QThread::msleep(1);
        }
//        else read = communicate_display(temp.write, temp.write_size, temp.read_size, temp.fb, temp.heartbeat);
        else emit send_port_data(temp);
//        qDebug() << read.toHex(',');
    }
}

TCU::TCU(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, ScanConfig *sc, QObject *parent, uint init_width) :
    ControlPort(label, edit, index, status_icon, parent),
    tcu_type(0),
    scan_mode(0),
    rep_freq(30),
    laser_width(init_width),
    delay_a(500),
    delay_b(500),
    gate_width_a(init_width),
    gate_width_b(init_width),
    ccd_freq(10),
    duty(5000),
    mcp(5),
    laser_on(0b0000),
    tcu_dist(1500),
#ifndef LVTONG
    dist_ns(3e8 * 1e-9 / 2),
#else
    dist_ns(3e8 * 1e-9 / 2 * 0.75),
#endif
    delay_n(0),
    gate_width_n(0),
    delay_dist(delay_a * dist_ns),
    depth_of_view(gate_width_a * dist_ns),
    auto_rep_freq(true),
    laser_offset(0),
    delay_offset(0),
    gate_width_offset(0),
    scan_config(sc)
{

}

int TCU::set_user_param(TCU::USER_PARAMS cmd, float val)
{
    if (cmd < 0x0100) {
        switch (cmd) {
        case REPEATED_FREQ: rep_freq = val;     set_tcu_param(cmd, 1.25e5 / rep_freq); break;
        case LASER_WIDTH  : laser_width = val;  set_tcu_param(cmd, laser_width / 8); break;
        case DELAY_A      : delay_a = val;      set_tcu_param(cmd, delay_a); break;
        case GATE_WIDTH_A : gate_width_a = val; set_tcu_param(cmd, gate_width_a); break;
        case DELAY_B      : delay_b = val;      set_tcu_param(cmd, delay_b); break;
        case GATE_WIDTH_B : gate_width_b = val; set_tcu_param(cmd, gate_width_b); break;
        case CCD_FREQ     : ccd_freq = val;     set_tcu_param(cmd, 1.25e8 / ccd_freq); break;
        case DUTY         : duty = val;         set_tcu_param(cmd, duty * 1.25e2); break;
        case MCP          : mcp = val;          set_tcu_param(cmd, val); break;
        case LASER1       :                     set_tcu_param(cmd, val); break;
        case LASER2       :                     set_tcu_param(cmd, val); break;
        case LASER3       :                     set_tcu_param(cmd, val); break;
        case LASER4       :                     set_tcu_param(cmd, val); break;
        case DIST         : tcu_dist = val;     set_tcu_param(cmd, val); break;
        case TOGGLE_LASER :                     set_tcu_param(cmd, val); break;
        default: break;
        }
    }
    else {
        switch (cmd) {
        case DELAY_N:
            delay_n = val;
            break;
        case GATE_WIDTH_N:
            gate_width_n = val;
            break;
        case LASER_USR:
            laser_width = val;
            set_tcu_param(TCU::LASER_WIDTH, (val + laser_offset) / 8);
            break;
        case EST_DIST: {
            delay_dist = val;
            delay_a = delay_dist / dist_ns;
            delay_b = delay_a + delay_n;

            if (scan_mode) rep_freq = scan_config->rep_freq;
            else if (auto_rep_freq) {
                // change repeated frequency according to delay: rep frequency (kHz) <= 1s / delay (μs)
                rep_freq = delay_dist ? 1e6 / (delay_dist / dist_ns + depth_of_view / dist_ns + 1000) : 30;
                if (rep_freq > 30) rep_freq = 30;
//                        if (rep_freq < 10) rep_freq = 10;
                set_tcu_param(TCU::REPEATED_FREQ, std::round(1.25e5 / rep_freq));
            }
            set_tcu_param(TCU::DELAY_A, std::round(delay_a + delay_offset));
            set_tcu_param(TCU::DELAY_B, std::round(delay_b + delay_offset));

            break;
        }
        case EST_DOV: {
            int gw = std::round(val / dist_ns), gw_corrected_a = gw + gate_width_offset, gw_corrected_b = gw + gate_width_n + gate_width_offset;
            if (gw_corrected_a < 0 || gw_corrected_b < 0) return -1;
            if (gw_corrected_a < 100) gw_corrected_a = gw_lut[gw_corrected_a];
            if (gw_corrected_b < 100) gw_corrected_b = gw_lut[gw_corrected_b];
            if (gw_corrected_a == -1 || gw_corrected_b == -1) return -1;

            gate_width_a = (depth_of_view = val) / dist_ns;
            gate_width_b = gate_width_a + gate_width_n;
            set_tcu_param(TCU::GATE_WIDTH_A, std::round(gw_corrected_a));
            set_tcu_param(TCU::GATE_WIDTH_B, std::round(gw_corrected_b));
            break;
        }
        case LASER_ON: {
            int change = laser_on ^ int(val);
            for(int i = 0; i < 4; i++) if ((change >> i) & 1) set_tcu_param(TCU::USER_PARAMS(TCU::LASER1 + i), int(val) & (1 << i) ? 8 : 4);
            laser_on = val;
            break;
        }
        default: break;
        }
    }

    return 0;
}

TCUDataGroup TCU::get_tcu_params()
{
    return TCUDataGroup{rep_freq, laser_width, delay_a, gate_width_a, delay_b, gate_width_b, ccd_freq, duty, mcp};
}

int TCU::set_tcu_param(TCU::USER_PARAMS param, float val)
{
    uint integer_part = std::round(val);
    uint decimal_part = 0;

    switch (tcu_type) {
    case 0: {
        QByteArray out(7, 0x00);
        out[0] = 0x88;
        out[1] = param;
        out[6] = 0x99;

        out[5] = integer_part & 0xFF; integer_part >>= 8;
        out[4] = integer_part & 0xFF; integer_part >>= 8;
        out[3] = integer_part & 0xFF; integer_part >>= 8;
        out[2] = integer_part & 0xFF;

//        communicate_display(out, 7, 1, false);
        q.push(PortData{out, 7, 1, false, false});
        return false;
    }
    case 1:
    case 2:
    case 3:
    case 4:
    {
        switch (param) {
        case DELAY_A:
        case DELAY_B:
        case GATE_WIDTH_A:
        case GATE_WIDTH_B:
            integer_part = (val >= tcu_type - 1) ? std::round(val - (uint(val) - tcu_type + 1) % 4) : 0;
            decimal_part = std::round((val - integer_part) / 0.05);
        default: break;
        }

        QByteArray out(8, 0x00);
        out[0] = 0x88;
        out[1] = param;
        out[7] = 0x99;

        out[6] = decimal_part;
        out[5] = integer_part & 0xFF; integer_part >>= 8;
        out[4] = integer_part & 0xFF; integer_part >>= 8;
        out[3] = integer_part & 0xFF; integer_part >>= 8;
        out[2] = integer_part & 0xFF;

//        communicate_display(out, 8, 1, false);
        q.push(PortData{out, 8, 1, false, false});
        return false;
    }
    default: return false;
    }
}

void TCU::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write[1] = {generate_ba(new uchar[7]{0x88, 0x15, 0x00, 0x00, 0x00, 0x00, 0x99}, 7)};
    q.push(PortData{write[write_idx], 7, 1, true, true});
//    QByteArray read = communicate_display(write, 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) successive_count++;
//    else                             successive_count = 0;
}

Lens::Lens(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent, int init_speed) :
    ControlPort(label, edit, index, status_icon, parent)
{
    zoom = focus = laser = 0;
    speed = init_speed;
}

void Lens::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write[3] = {generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56}, 7),
                                  generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57}, 7),
                                  generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x57, 0x00, 0x00, 0x58}, 7)};

    q.push(PortData{write[write_idx], 7, 7, true, true});
    write_idx = (write_idx + 1) % 3;

//    QByteArray read = communicate_display(write[write_idx], 7, 7, true, false);
//    if (read != QByteArray(1, 0x15)) successive_count++;
//    else                             successive_count = 0;
//    read = communicate_display(write[write_idx], 7, 7, true, false);
//    if (read != QByteArray(1, 0x15)) successive_count++;
//    else                             successive_count = 0;
//    read = communicate_display(write[write_idx], 7, 7, true, false);
//    if (read != QByteArray(1, 0x15)) successive_count++;
//    else                             successive_count = 0;
}

Laser::Laser(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent) :
    ControlPort(label, edit, index, status_icon, parent) {}

void Laser::try_communicate()
{return;
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write = generate_ba(new uchar[7]{0x88, 0x15, 0x00, 0x00, 0x00, 0x00, 0x99}, 7);
    QByteArray read = communicate_display(write, 7, 7, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
}

PTZ::PTZ(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent, int init_speed) :
    ControlPort(label, edit, index, status_icon, parent)
{
    angle_h = angle_v = 0;
    ptz_speed = init_speed;
}

void PTZ::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write[2] = {generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x51, 0x00, 0x00, 0x52}, 7),
                                  generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x53, 0x00, 0x00, 0x54}, 7)};

    q.push(PortData{write[write_idx], 7, 7, true, true});
    write_idx = (write_idx + 1) % 2;

//    QByteArray read = communicate_display(write[write_idx], 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) successive_count++;
//    else                             successive_count = 0;
//    read = communicate_display(write[write_idx], 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) successive_count++;
//    else                             successive_count = 0;
}

Inclin::Inclin(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent) :
    ControlPort(label, edit, index, status_icon, parent)
{
//    time_interval = 990;
}

void Inclin::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    q.push(PortData{QByteArray(), 7, 44, true, true});

//    QByteArray read = communicate_display(write[write_idx], 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) successive_count++;
//    else                             successive_count = 0;
//    read = communicate_display(write[write_idx], 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) successive_count++;
//    else                             successive_count = 0;
}
