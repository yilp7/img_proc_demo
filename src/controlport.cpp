#include "controlport.h"
#include "userpanel.h"

ControlPort::ControlPort(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent) :
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

void ControlPort::setup_tcp_port(QString ip, int port)
{
    tcp_port->connectToHost(ip, port);
    connected_to_tcp = tcp_port->waitForConnected(1000);
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

void ControlPort::set_theme()
{
    lbl->setStyleSheet(app_theme ? "color: #180D1C;" : "color: #B0C4DE;");
}

TCU::TCU(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent, uint init_width) : ControlPort(label, edit, index, status_icon, parent)
{
    int u = init_width / 1000, n = init_width % 1000;
    rep_freq = 3e4f;
    laser_width_u = u, laser_width_n = n;
    delay_a_u = 0, delay_a_n = 500;
    delay_b_u = 0, delay_b_n = 500;
    gate_width_a_u = u, gate_width_a_n = n;
    gate_width_b_u = u, gate_width_b_n = n;
    fps = 10, duty = 5000;
    mcp = 5;
    laser_on = 0;

    dist_ns = 3e8 * 1e-9 / 2;
#ifdef LVTONG
    dist_ns *= 0.75;
#endif
    delay_n_n = 0;
    delay_dist = (delay_a_u * 1000 + delay_a_n) * dist_ns;
    depth_of_view = (gate_width_a_u * 1000 + gate_width_a_n) * dist_ns;
}

bool TCU::set_tcu_param(TCU::TCU_PARAMS param, uint val)
{
    QByteArray out(7, 0x00);
    out[0] = 0x88;
    out[1] = param;
    out[6] = 0x99;

    out[5] = val & 0xFF; val >>= 8;
    out[4] = val & 0xFF; val >>= 8;
    out[3] = val & 0xFF; val >>= 8;
    out[2] = val & 0xFF;

    communicate_display(out, 7, 1, false);
    return false;
}

bool TCU::set_user_param(TCU::USER_PARAMS cmd, uint val)
{
    switch (cmd) {
    case DELAY_N:
        delay_n_n = val;
        break;
    case EST_DIST: {
        break;
    }
    case EST_DOV:
        break;
    case LASER_ON:
        break;
    }
    return false;
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

Lens::Lens(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent, int init_speed) : ControlPort(label, edit, index, status_icon, parent)
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

Laser::Laser(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent) : ControlPort(label, edit, index, status_icon, parent) {}

void Laser::try_communicate()
{
    // TODO: disable self-check system
    if (!connected_to_serial && !connected_to_tcp) return;

    static QByteArray write = generate_ba(new uchar[7]{0x88, 0x15, 0x00, 0x00, 0x00, 0x00, 0x99}, 7);
    QByteArray read = communicate_display(write, 7, 1, true, false);
    if (read != QByteArray(1, 0x15)) successive_count++;
    else                             successive_count = 0;
}

PTZ::PTZ(QLabel *label, QLineEdit *edit, int index, StatusIcon *status_icon, QObject *parent, int init_speed) : ControlPort(label, edit, index, status_icon, parent)
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
