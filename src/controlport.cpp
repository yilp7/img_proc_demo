#include "controlport.h"
#include "userpanel.h"

ControlPort::ControlPort(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, Preferences *pref, QObject *parent) :
    QObject{parent},
    connected_to_serial(false),
    connected_to_tcp(false),
    use_tcp(false),
    baudrate(QSerialPort::Baud9600),
    shared_from(-1),
    shared_port(NULL),
    time_interval(1000),
    write_succeeded(false),
    count_threshold(5),
    successive_count(0)
{
    //[0] set up ui pointers with info structure
    this->lbl = label;
    this->edt = edit;
    this->idx = index;
    this->status_icon = status_icon;
    this->pref = pref;
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
            this->status_icon->update_status(last_status);
        });
    timer->start(time_interval);
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
    fseek(f, idx, SEEK_SET);
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
        return use_tcp = false;
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

// send and receive data from any ports, and display in main window
QByteArray ControlPort::communicate_display(QByteArray write, int write_size, int read_size, bool fb, bool display) {
    if (shared_port) return shared_port->communicate_display(write, write_size, read_size, fb, display);

    port_mutex.lock();
    QByteArray read;
    QString str_s("sent    "), str_r("received");

    for (char i = 0; i < write_size; i++) str_s += QString::asprintf(" %02X", (uchar)write[i]);
    if (display) emit port_io(str_s);

    QIODevice *port = use_tcp ? (QIODevice*)tcp_port : (QIODevice*)serial_port;

    if (!port->isOpen()) {
        port_mutex.unlock();
        return QByteArray();
    }
    port->write(write, write_size);
    write_succeeded = port->waitForBytesWritten(10);

    if (fb) port->waitForReadyRead(10);

    read = read_size ? port->read(read_size) : port->readAll();
    for (char i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
    if (display) emit port_io(str_r);

    port_mutex.unlock();
    QThread().msleep(10);
    return read;
}

void ControlPort::share_port_from(ControlPort *port)
{
    shared_port = port;
    shared_from = port ? port->idx : -1;
}

int ControlPort::status()
{
    if (use_tcp) return connected_to_tcp ? 1 : 0;
    else return connected_to_serial ? 2 : 0;
}

void ControlPort::set_theme()
{
    lbl->setStyleSheet(app_theme ? "color: #180D1C;" : "color: #B0C4DE;");
}

TCU::TCU(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, Preferences *pref, ScanConfig *sc, QObject *parent, uint init_width) :
    ControlPort(label, edit, index, status_icon, pref, parent),
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
    depth_of_view(gate_width_a * dist_ns)
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
            set_tcu_param(TCU::LASER_WIDTH, val + pref->laser_width_offset);
            break;
        case EST_DIST: {
            delay_dist = val;
            delay_a = delay_dist / dist_ns;
            delay_b = delay_a + delay_n;

            if (scan_mode) rep_freq = scan_config->rep_freq;
            else {
                // change repeated frequency according to delay: rep frequency (kHz) <= 1s / delay (μs)
                if (pref->auto_rep_freq) {
                    rep_freq = delay_dist ? 1e6 / (delay_dist / dist_ns + depth_of_view / dist_ns + 1000) : 30;
                    if (rep_freq > 30) rep_freq = 30;
//                        if (rep_freq < 10) rep_freq = 10;
                }
                set_tcu_param(TCU::REPEATED_FREQ, std::round(1.25e5 / rep_freq));
            }
            set_tcu_param(TCU::DELAY_A, std::round(delay_a + pref->delay_offset));
            set_tcu_param(TCU::DELAY_B, std::round(delay_b + pref->delay_offset));

            break;
        }
        case EST_DOV: {
            int gw = std::round(val / dist_ns), gw_corrected_a = gw + pref->gate_width_offset, gw_corrected_b = gw + gate_width_n + pref->gate_width_offset;
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

        communicate_display(out, 7, 1, false);
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

        communicate_display(out, 8, 1, false);
        return false;
    }
    default: return false;
    }
}

void TCU::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write = generate_ba(new uchar[7]{0x88, 0x15, 0x00, 0x00, 0x00, 0x00, 0x99}, 7);
    QByteArray read = communicate_display(write, 7, 1, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
}

Lens::Lens(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, Preferences *pref, QObject *parent, int init_speed) : ControlPort(label, edit, index, status_icon, pref, parent)
{
    zoom = focus = laser = 0;
    speed = init_speed;
}

void Lens::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write1 = generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56}, 7);
    static QByteArray write2 = generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57}, 7);
    static QByteArray write3 = generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x57, 0x00, 0x00, 0x58}, 7);
    QByteArray read = communicate_display(write1, 7, 1, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
    read = communicate_display(write2, 7, 1, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
    read = communicate_display(write3, 7, 1, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
}

Laser::Laser(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, Preferences *pref, QObject *parent) : ControlPort(label, edit, index, status_icon, pref, parent) {}

void Laser::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write = generate_ba(new uchar[7]{0x88, 0x15, 0x00, 0x00, 0x00, 0x00, 0x99}, 7);
    QByteArray read = communicate_display(write, 7, 1, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
}

PTZ::PTZ(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, Preferences *pref, QObject *parent, int init_speed) : ControlPort(label, edit, index, status_icon, pref, parent)
{
    angle_h = angle_v = 0;
    ptz_speed = init_speed;
}

void PTZ::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write1 = generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x51, 0x00, 0x00, 0x52}, 7);
    static QByteArray write2 = generate_ba(new uchar[7]{0xFF, 0x01, 0x00, 0x53, 0x00, 0x00, 0x54}, 7);
    QByteArray read = communicate_display(write1, 7, 1, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
    read = communicate_display(write2, 7, 1, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
}
