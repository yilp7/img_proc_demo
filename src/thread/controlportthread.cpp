#include "controlportthread.h"
// #include "userpanel.h"

#include <QLabel>
#include <QStyle>

namespace {
void set_port_label_state(QLabel *label, bool connected)
{
    if (!label) return;
    label->setProperty("portState", connected ? "connected" : "error");
    label->setProperty("portNetwork", "false");
    if (label->style()) {
        label->style()->unpolish(label);
        label->style()->polish(label);
    }
    label->update();
}
}

ControlPortThread::ControlPortThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon, QObject *parent) :
    QThread{parent},
    m_thread_interrupt(false),
    m_connected_to_serial(false),
    m_connected_to_tcp(false),
    m_use_tcp(false),
    m_baudrate(QSerialPort::Baud9600),
    m_shared_from(-1),
    m_shared_port(nullptr),
    m_hold(false),
    m_time_interval(1000),
    m_write_idx(0),
    m_write_succeeded(false),
    m_count_threshold(5),
    m_successive_count(0)
{
    //[0] set up ui pointers with info structure
    this->m_lbl = label;
    this->m_edt = edit;
    this->m_idx = index;
    this->m_status_icon = status_icon;
    //![0]

    //[1] set up timer for communication status check
    m_timer = new QTimer(this);
//    connect(m_timer, SIGNAL(timeout()), this, SLOT(try_communicate()));
    connect(m_timer, &QTimer::timeout, this, &ControlPortThread::try_communicate);
    // TODO: change bool param to int
    connect(m_timer, &QTimer::timeout, this,
        [this](){
            // status: -1: not connected, 0: ok, 1: write successful
            static int last_status; // 4=CONNECTED 3=RECONNECTING 2=DISCONNECTED 1=NOT_CONNECTED
            last_status = 4; // CONNECTED
            if (!m_connected_to_serial && !m_connected_to_tcp) last_status = 1; // NOT_CONNECTED
            if (m_successive_count > 5) m_successive_count = 5, last_status = m_write_succeeded ? 3 : 2; // RECONNECTING : DISCONNECTED
//            emit port_connected(last_status);
            if (this->m_status_icon) this->m_status_icon->update_status(last_status);
        });
    m_timer->start(m_time_interval);
//    if (m_idx == 4) qDebug() << "init" << QThread::currentThread();
    //![1]

    //[2] setup serialport / tcpsocket for communication
    m_serial_port = new QSerialPort(this);
    m_tcp_port = new QTcpSocket(this);
    connect(m_edt, &QLineEdit::returnPressed, this, &ControlPortThread::setup_serial_port);
//    connect(parent, &UserPanel::connect_to_tcp_port, this, [this](QString ip, int port){});
    //![2]
}

ControlPortThread::~ControlPortThread() {}

void ControlPortThread::try_communicate()
{
    if (m_hold) return;
}

void ControlPortThread::setup_serial_port()
{
    if (m_serial_port->isOpen()) m_serial_port->close();

    QString port_name = "COM" + m_edt->text();
    m_serial_port->setPortName(port_name);
//    qDebug("%p", *com);

    if (m_serial_port->open(QIODevice::ReadWrite)) {
        QThread::msleep(10);
        m_serial_port->clear();
        qDebug("%s connected", qPrintable(port_name));
//        m_connected_to_serial = true;
        set_port_label_state(m_lbl, true);

        m_serial_port->setBaudRate(m_baudrate);
        m_serial_port->setDataBits(QSerialPort::Data8);
        m_serial_port->setParity(QSerialPort::NoParity);
        m_serial_port->setStopBits(QSerialPort::OneStop);
        m_serial_port->setFlowControl(QSerialPort::NoFlowControl);

        m_connected_to_serial = true;
        m_successive_count = 0;

        // send initial data
        switch (m_idx) {
        case 4:
            { uchar buf[] = {0XFF, 0XAA, 0X69, 0X88, 0XB5}; m_q.push(PortData{QByteArray((char*)buf, 5), 5, 1, true, true, nullptr, nullptr}); }
            { uchar buf[] = {0xFF, 0xAA, 0x03, 0x03, 0x00}; m_q.push(PortData{QByteArray((char*)buf, 5), 5, 1, true, true, nullptr, nullptr}); }
            { uchar buf[] = {0xFF, 0xAA, 0x02, 0x0F, 0x00}; m_q.push(PortData{QByteArray((char*)buf, 5), 5, 1, true, true, nullptr, nullptr}); }
            { uchar buf[] = {0XFF, 0XAA, 0X00, 0X00, 0X00}; m_q.push(PortData{QByteArray((char*)buf, 5), 5, 1, true, true, nullptr, nullptr}); }
            break;
        default:
            break;
        }
    }
    else {
        m_connected_to_serial = false;
        set_port_label_state(m_lbl, false);
    }
    // COM port changes will be saved to JSON config when user saves configuration
}

bool ControlPortThread::setup_tcp_port(QString ip, int port, bool connect)
{
    if (connect) {
        m_tcp_port->connectToHost(ip, port);
        return m_use_tcp = m_connected_to_tcp = m_tcp_port->waitForConnected(1000);
    }
        m_tcp_port->disconnectFromHost();
        return m_use_tcp = m_connected_to_tcp = false;
}

inline QByteArray ControlPortThread::generate_ba(uchar *data, int len)
{
    QByteArray ba((char*)data, len);
    delete[] data;
    return ba;
}

int ControlPortThread::get_baudrate()
{
    return m_serial_port->baudRate();
}

bool ControlPortThread::set_baudrate(int baudrate)
{
    return m_serial_port->setBaudRate(baudrate);
}

bool ControlPortThread::get_tcp_status()
{
    return m_use_tcp;
}

void ControlPortThread::set_tcp_status(bool use_tcp)
{
    this->m_use_tcp = use_tcp;
}

// send and receive data from any ports, and display in main window
QByteArray ControlPortThread::communicate_display(QByteArray write, int write_size, int read_size, bool fb, bool heartbeat, int read_timeout)
{
    if (m_shared_port) return m_shared_port->communicate_display(write, write_size, read_size, fb, heartbeat, read_timeout);

//    qDebug() << "port communication" << QThread::currentThread();
    m_port_mutex.lock();
    QByteArray read;
    QString str_s("sent    "), str_r("received");

    for (int i = 0; i < write_size; i++) str_s += QString::asprintf(" %02X", (uchar)write[i]);
    if (!heartbeat) emit port_io(str_s);

    QIODevice *port = m_use_tcp ? (QIODevice*)m_tcp_port : (QIODevice*)m_serial_port;

    if (!port->isOpen()) {
        m_port_mutex.unlock();
        return QByteArray();
    }
    port->readAll();
    port->write(write.constData(), write_size);
    m_write_succeeded = port->waitForBytesWritten(5);

    static QElapsedTimer read_timer;
    if (read_timer.elapsed()) read_timer.restart();
    else                      read_timer.start();
    read = read_size ? port->read(read_size) : port->readAll();
    while (fb && (read_size == 0 || read.size() < read_size) && read_timer.elapsed() < read_timeout) {
        read += read_size ? port->read(read_size - read.size()) : port->readAll();
        qDebug() << read.size();
        port->waitForReadyRead(10);
    }

    for (int i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
    if (!heartbeat) emit port_io(str_r);

    m_port_mutex.unlock();
    QThread().msleep(5);
    return read;
}

void ControlPortThread::communicate_display(PortData port_data)
{
    QByteArray read = communicate_display(port_data.write, port_data.write_size, port_data.read_size, port_data.fb, port_data.heartbeat, 100);
    if (port_data.heartbeat) {
        switch (m_idx) {
        case 0:
            // qDebug() << "TCU heartbeat" << read.toHex(' ').toUpper();
            if (read != QByteArray(1, 0x15)) m_successive_count++;
            else                             m_successive_count = 0;
            break;
        case 1:
            // qDebug() << "lens heartbeat" << read.toHex(' ').toUpper();
            qDebug() << read.toHex(' ').toUpper();
            switch (port_data.write[3]) {
            case 0x55: if (read[3] != char(0x5D)) m_successive_count++;
                       else                       m_successive_count = 0;
            case 0x56: if (read[3] != char(0x5E)) m_successive_count++;
                       else                       m_successive_count = 0;
            case 0x57: if (read[3] != char(0x70)) m_successive_count++;
                       else                       m_successive_count = 0;
            default: break;
            }

            break;
        case 2:
            // qDebug() << "laser heartbeat" << read.toHex(' ').toUpper();
            break;
        case 3:
            // qDebug() << "PTZ heartbeat" << read.toHex(' ').toUpper();
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
    if (port_data.fb && port_data.completed) {
        *(port_data.completed) = true;
        uchar *data = (uchar *)malloc(read.size() * sizeof(uchar));
        memcpy(data, read.constData(), read.size());
        *(port_data.data_ptr) = data;
    }
}

void ControlPortThread::share_port_from(ControlPortThread *port)
{
    m_shared_port = port;
    m_shared_from = port ? port->m_idx : -1;
}

int ControlPortThread::get_port_status()
{
    return (m_connected_to_tcp ? PortStatus::TCP_CONNECTED : PortStatus::NOT_CONNECTED) |
           (m_connected_to_serial ? PortStatus::SERIAL_CONNECTED : PortStatus::NOT_CONNECTED);
}

void ControlPortThread::apply_color_mode()
{
    set_port_label_state(m_lbl, m_connected_to_serial);
}

void ControlPortThread::quit_thread()
{
    m_thread_interrupt = true;
    quit();
    wait();
}

void ControlPortThread::send_data(PortData d)
{
    m_q.push(d);
}

void ControlPortThread::run()
{
    qDebug() << "com thread" << QThread::currentThread();
    qRegisterMetaType<PortData>("PortData");
//    connect(this, SIGNAL(send_port_data(PortData)), this, SLOT(communicate_display(PortData)));
    connect(this, SIGNAL(send_port_data(PortData)), this, SLOT(communicate_display(PortData)), Qt::DirectConnection);
    while (!m_thread_interrupt) {
        if (m_q.empty()) { QThread::msleep(10); continue; }
        PortData temp = m_q.front();
        m_q.pop();
        QByteArray read, read_byte(2, 0x00);
        if (temp.write.isEmpty()) {
            QIODevice *port = m_use_tcp ? (QIODevice*)m_tcp_port : (QIODevice*)m_serial_port;
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

TCUThread::TCUThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon, ScanParams *sp, QObject *parent, uint init_width) :
    ControlPortThread(label, edit, index, status_icon, parent),
    m_tcu_type(0),
    m_scan_mode(0),
    m_rep_freq(30),
    m_laser_width(init_width),
    m_delay_a(500),
    m_delay_b(500),
    m_gate_width_a(init_width),
    m_gate_width_b(init_width),
    m_ccd_freq(10),
    m_duty(5000),
    m_mcp(5),
    m_laser_on(0b0000),
    m_tcu_dist(1500),
#ifndef LVTONG
    m_dist_ns(3e8 * 1e-9 / 2),
#else
    m_dist_ns(3e8 * 1e-9 / 2 * 0.75),
#endif
    m_delay_n(0),
    m_gate_width_n(0),
    m_delay_dist(m_delay_a * m_dist_ns),
    m_depth_of_view(m_gate_width_a * m_dist_ns),
    m_auto_rep_freq(true),
    m_ab_lock(true),
    m_laser_offset(0),
    m_delay_offset(0),
    m_gate_width_offset(0),
    m_ps_step{40, 40, 40, 40},
    m_use_gw_lut(false),
    m_scan_params(sp)
{

}

int TCUThread::set_user_param(TCUThread::UserParams cmd, float val)
{
    if (cmd < 0x0100) {
        uint cycle;
        switch (m_tcu_type) {
        case 0: cycle = 8; break;
        case 1: cycle = 4; break;
        default: break;
        }

        switch (cmd) {
        case REPEATED_FREQ: m_rep_freq = val;     set_tcu_param(cmd, (1e6 / cycle) / m_rep_freq); break;
        case LASER_WIDTH  : m_laser_width = val;  set_tcu_param(cmd, m_laser_width / cycle); break;
        case DELAY_A      : m_delay_a = val;      set_tcu_param(cmd, m_delay_a); break;
        case GATE_WIDTH_A : m_gate_width_a = val; set_tcu_param(cmd, m_gate_width_a); break;
        case DELAY_B      : m_delay_b = val;      set_tcu_param(cmd, m_delay_b); break;
        case GATE_WIDTH_B : m_gate_width_b = val; set_tcu_param(cmd, m_gate_width_b); break;
        case CCD_FREQ     : m_ccd_freq = val;     set_tcu_param(cmd, (1e9 / cycle) / m_ccd_freq); break;
        case DUTY         : m_duty = val;         set_tcu_param(cmd, m_duty * 1e3 / cycle); break;
        case MCP          : m_mcp = val;          set_tcu_param(cmd, val); break;
        case LASER1       :                       set_tcu_param(cmd, val); break;
        case LASER2       :                       set_tcu_param(cmd, val); break;
        case LASER3       :                       set_tcu_param(cmd, val); break;
        case LASER4       :                       set_tcu_param(cmd, val); break;
        case DIST         : m_tcu_dist = val;     set_tcu_param(cmd, val); break;
        case TOGGLE_LASER :                       set_tcu_param(cmd, val); break;
        default: break;
        }
    }
    else {
        switch (cmd) {
        case DELAY_N:
            m_delay_n = val;
            break;
        case GATE_WIDTH_N:
            m_gate_width_n = val;
            break;
        case LASER_USR:
            m_laser_width = val;
            set_user_param(TCUThread::LASER_WIDTH, val + m_laser_offset);
            break;
        case EST_DIST: {
            m_delay_dist = val;
            m_delay_a = m_delay_dist / m_dist_ns;
            m_delay_b = m_delay_a + m_delay_n;

            if (m_scan_mode) m_rep_freq = m_scan_params->rep_freq;
            else if (m_auto_rep_freq) {
                // change repeated frequency according to delay: rep frequency (kHz) <= 1s / delay (μs)
                m_rep_freq = m_delay_dist ? 1e6 / (m_delay_dist / m_dist_ns + m_depth_of_view / m_dist_ns + 1000) : 30;
                if (m_rep_freq > 30) m_rep_freq = 30;
//                        if (m_rep_freq < 10) m_rep_freq = 10;
                set_tcu_param(TCUThread::REPEATED_FREQ, std::round(1.25e5 / m_rep_freq));
            }
            set_tcu_param(TCUThread::DELAY_A, m_delay_a + m_delay_offset);
            set_tcu_param(TCUThread::DELAY_B, m_delay_b + m_delay_offset);

            break;
        }
        case EST_DOV: {
            int gw = std::round(val / m_dist_ns), gw_corrected_a = gw + m_gate_width_offset, gw_corrected_b = gw + m_gate_width_n + m_gate_width_offset;
            if (gw_corrected_a < 0 || gw_corrected_b < 0) return -1;
            if (gw_corrected_a < 100) gw_corrected_a = m_gw_lut[gw_corrected_a];
            if (gw_corrected_b < 100) gw_corrected_b = m_gw_lut[gw_corrected_b];
            if (gw_corrected_a == -1 || gw_corrected_b == -1) return -1;

            m_gate_width_a = (m_depth_of_view = val) / m_dist_ns;
            m_gate_width_b = m_gate_width_a + m_gate_width_n;
            set_tcu_param(TCUThread::GATE_WIDTH_A, m_use_gw_lut ? gw_corrected_a : m_gate_width_a);
            set_tcu_param(TCUThread::GATE_WIDTH_B, m_use_gw_lut ? gw_corrected_b : m_gate_width_b);
            break;
        }
        case LASER_ON: {
            int change = m_laser_on ^ int(val);
            for(int i = 0; i < 4; i++) if ((change >> i) & 1) set_tcu_param(TCUThread::UserParams(TCUThread::LASER1 + i), int(val) & (1 << i) ? 8 : 4);
            m_laser_on = val;
            break;
        }
        default: break;
        }
    }

    return 0;
}

TCUDataGroup TCUThread::get_tcu_params()
{
    return TCUDataGroup{m_rep_freq, m_laser_width, m_delay_a, m_gate_width_a, m_delay_b, m_gate_width_b, m_ccd_freq, m_duty, m_mcp};
}

int TCUThread::set_tcu_param(TCUThread::UserParams param, float val)
{
    uint integer_part, decimal_part;

    switch (m_tcu_type) {
    case 0: {
        integer_part = std::round(val);
        decimal_part = 0;

        QByteArray out(7, 0x00);
        out[0] = 0x88;
        out[1] = param;
        out[6] = 0x99;

        out[5] = integer_part & 0xFF; integer_part >>= 8;
        out[4] = integer_part & 0xFF; integer_part >>= 8;
        out[3] = integer_part & 0xFF; integer_part >>= 8;
        out[2] = integer_part & 0xFF;

//        communicate_display(out, 7, 1, false);
        m_q.push(PortData{out, 7, 1, false, false});
        return false;
    }
    case 1:
    case 2:
    case 3:
    case 4:
    {
#if 0 // 8-byte ps-TCU
        switch (param) {
        case DELAY_A:
        case DELAY_B:
        case GATE_WIDTH_A:
        case GATE_WIDTH_B:
            integer_part = (val >= m_tcu_type - 1) ? std::round(val - (uint(val) - m_tcu_type + 1) % 4) : 0;
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
        m_q.push(PortData{out, 8, 1, false, false});
        return false;
#endif
        TCUThread::UserParams param_ps = TCUThread::UserParams::NO_PARAM;
        int step = 0;
        switch (param) {
        case DELAY_A:
            param_ps = DELAY_A_PS;
            step = m_ps_step[0];
            integer_part = int(val + 0.001) / 4;
            decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
            m_delay_a = integer_part * 4 + decimal_part * step / 1000.;
            break;
        case DELAY_B:
            param_ps = DELAY_B_PS;
            step = m_ps_step[1];
            integer_part = int(val + 0.001) / 4;
            decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
            m_delay_b = integer_part * 4 + decimal_part * step / 1000.;
            break;
        case GATE_WIDTH_A:
            param_ps = GW_A_PS;
            step = m_ps_step[2];
            integer_part = int(val + 0.001) / 4;
            decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
            m_gate_width_a = integer_part * 4 + decimal_part * step / 1000.;
            break;
        case GATE_WIDTH_B:
            param_ps = GW_B_PS;
            step = m_ps_step[3];
            integer_part = int(val + 0.001) / 4;
            decimal_part = std::round((val - integer_part * 4 + 0.001) * 1000 / step);
            m_gate_width_b = integer_part * 4 + decimal_part * step / 1000.;
            break;
        default:
            integer_part = int(val);
            decimal_part = 0;
            break;
        }

        QByteArray out_1(7, 0x00);
        out_1[0] = 0x88;
        out_1[1] = param;
        out_1[6] = 0x99;

        out_1[5] = integer_part & 0xFF; integer_part >>= 8;
        out_1[4] = integer_part & 0xFF; integer_part >>= 8;
        out_1[3] = integer_part & 0xFF; integer_part >>= 8;
        out_1[2] = integer_part & 0xFF;

        m_q.push(PortData{out_1, 7, 1, false, false});

        if (param_ps != TCUThread::UserParams::NO_PARAM) {
            QByteArray out_2(7, 0x00);
            out_2[0] = 0x88;
            out_2[1] = param_ps;
            out_2[6] = 0x99;

            out_2[5] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[4] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[3] = decimal_part & 0xFF; decimal_part >>= 8;
            out_2[2] = decimal_part & 0xFF;

            m_q.push(PortData{out_2, 7, 1, false, false});
        }
        return false;
    }
    default: return false;
    }
}

void TCUThread::try_communicate()
{
    ControlPortThread::try_communicate();

    // TODO: disable self-check system
    if (!m_connected_to_serial && !m_connected_to_tcp) return;

    static uchar buf0[] = {0x88, 0x15, 0x00, 0x00, 0x00, 0x00, 0x99};
    static QByteArray write[1] = {QByteArray((char*)buf0, 7)};
    m_q.push(PortData{write[m_write_idx], 7, 1, true, true, nullptr, nullptr});
//    QByteArray read = communicate_display(write, 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) m_successive_count++;
//    else                             m_successive_count = 0;
}

LensThread::LensThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon, QObject *parent, int init_speed) :
    ControlPortThread(label, edit, index, status_icon, parent)
{
    m_zoom = m_focus = m_laser_radius = 0;
    m_lens_speed = init_speed;

    m_hold = 1;
}

int LensThread::lens_control(Params command, uint *val)
{
    static uchar buf_stop[]   = {0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01};
    static uchar buf_zi[]     = {0xFF, 0x01, 0x00, 0x40, 0x00, 0x00, 0x41};
    static uchar buf_zo[]     = {0xFF, 0x01, 0x00, 0x20, 0x00, 0x00, 0x21};
    static uchar buf_ff[]     = {0xFF, 0x01, 0x00, 0x80, 0x00, 0x00, 0x81};
    static uchar buf_fn[]     = {0xFF, 0x01, 0x01, 0x00, 0x00, 0x00, 0x02};
    static uchar buf_ru[]     = {0xFF, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03};
    static uchar buf_rd[]     = {0xFF, 0x01, 0x04, 0x00, 0x00, 0x00, 0x05};
    static uchar buf_zp[]     = {0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56};
    static uchar buf_fp[]     = {0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57};
    static uchar buf_lr[]     = {0xFF, 0x01, 0x00, 0x57, 0x00, 0x00, 0x58};
    static QByteArray write[ ] = {QByteArray((char*)buf_stop, 7), // stop
                                  QByteArray((char*)buf_zi, 7),   // zoom in
                                  QByteArray((char*)buf_zo, 7),   // zoom out
                                  QByteArray((char*)buf_ff, 7),   // focus far
                                  QByteArray((char*)buf_fn, 7),   // focus near
                                  QByteArray((char*)buf_ru, 7),   // radius up
                                  QByteArray((char*)buf_rd, 7),   // radius down
                                  QByteArray((char*)buf_zp, 7),   // zoom pos
                                  QByteArray((char*)buf_fp, 7),   // focus pos
                                  QByteArray((char*)buf_lr, 7),   // laser radius
                                  };
    switch (command) {
    case STOP:
    case ZOOM_IN:
    case ZOOM_OUT:
    case FOCUS_FAR:
    case FOCUS_NEAR:
    case RADIUS_UP:
    case RADIUS_DOWN:
        m_q.push(PortData{write[command], 7, 0, false, false}); break;
    case ZOOM_POS:
    case FOCUS_POS:
    case LASER_RADIUS:
    {
        if (!val) return -2;
        bool completed = false;
        uchar *data = nullptr;
        m_q.push(PortData{write[command], 7, 7, true, false, &completed, &data});
        while (!completed) QThread::msleep(2);
        *val = (uint(data[4]) << 8) + uint(data[5]);
        qDebug() << "lens feedback" << *val << QByteArray((char*)data, 7).toHex(' ').toUpper();
        switch (command) {
        case ZOOM_POS:             m_zoom = *val; break;
        case FOCUS_POS:           m_focus = *val; break;
        case LASER_RADIUS: m_laser_radius = *val; break;
        }
        break;
    }
    case SET_ZOOM:
    case SET_FOCUS:
    case SET_RADIUS:
    {
        if (!val) return -1;
        unsigned sum = 0;
        uchar out[7];
        out[0] = 0xFF;
        out[1] = 0x01;
        out[2] = 0x00;
        switch (command) {
        case SET_ZOOM:   out[3] = 0x4F;         m_zoom = *val; out[4] = (        m_zoom >> 8) & 0xFF; out[5] =         m_zoom & 0xFF; break;
        case SET_FOCUS:  out[3] = 0x4E;        m_focus = *val; out[4] = (       m_focus >> 8) & 0xFF; out[5] =        m_focus & 0xFF; break;
        case SET_RADIUS: out[3] = 0x81; m_laser_radius = *val; out[4] = (m_laser_radius >> 8) & 0xFF; out[5] = m_laser_radius & 0xFF; break;
        }
        for (int i = 1; i < 6; i++)
            sum += out[i];
        out[6] = sum & 0xFF;
        m_q.push(PortData{QByteArray((char *)out, 7), 7, 0, false, false});
        break;
    }
    case STEPPING:
    {
        if (!val) return -1;
        m_lens_speed = *val;
        uchar out_data[9] = {0};
        out_data[0] = 0xB0;
        out_data[1] = 0x01;
        out_data[2] = 0xA0;
        out_data[3] = 0x01;
        out_data[4] = m_lens_speed;
        out_data[5] = m_lens_speed;
        out_data[6] = m_lens_speed;
        out_data[7] = m_lens_speed;

        out_data[8] = (4 * uint(m_lens_speed) + 0xA2) & 0xFF;
        m_q.push(PortData{QByteArray((char*)out_data, 9), 9, 0, false, false});
        break;
    }
    case ADDRESS:
    default: break;
    }
    return 0;
}

void LensThread::try_communicate()
{
    if (m_hold) return;

    // TODO: disable self-check system
    if (!m_connected_to_serial && !m_connected_to_tcp) return;

    static uchar buf_zp[] = {0xFF, 0x01, 0x00, 0x55, 0x00, 0x00, 0x56};
    static uchar buf_fp[] = {0xFF, 0x01, 0x00, 0x56, 0x00, 0x00, 0x57};
    static uchar buf_lr[] = {0xFF, 0x01, 0x00, 0x57, 0x00, 0x00, 0x58};
    static QByteArray write[3] = {QByteArray((char*)buf_zp, 7),
                                  QByteArray((char*)buf_fp, 7),
                                  QByteArray((char*)buf_lr, 7)};

    m_q.push(PortData{write[m_write_idx], 7, 7, true, true, nullptr, nullptr});
    m_write_idx = (m_write_idx + 1) % 3;

//    QByteArray read = communicate_display(write[m_write_idx], 7, 7, true, false);
//    if (read != QByteArray(1, 0x15)) m_successive_count++;
//    else                             m_successive_count = 0;
//    read = communicate_display(write[m_write_idx], 7, 7, true, false);
//    if (read != QByteArray(1, 0x15)) m_successive_count++;
//    else                             m_successive_count = 0;
//    read = communicate_display(write[m_write_idx], 7, 7, true, false);
//    if (read != QByteArray(1, 0x15)) m_successive_count++;
//    else                             m_successive_count = 0;
}

LaserThread::LaserThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon, QObject *parent) :
    ControlPortThread(label, edit, index, status_icon, parent) {}

void LaserThread::try_communicate()
{return;
    ControlPortThread::try_communicate();

    // TODO: disable self-check system
    if (!m_connected_to_serial && !m_connected_to_tcp) return;

    static uchar buf[] = {0x88, 0x15, 0x00, 0x00, 0x00, 0x00, 0x99};
    static QByteArray write = QByteArray((char*)buf, 7);
    QByteArray read = communicate_display(write, 7, 7, true, false);
    if (read != QByteArray(1, 0x15)) m_successive_count++;
    else                             m_successive_count = 0;
}

PTZThread::PTZThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon, QObject *parent, int init_speed) :
    ControlPortThread(label, edit, index, status_icon, parent)
{
    m_angle_h = m_angle_v = 0;
    m_ptz_speed = init_speed;
}

int PTZThread::ptz_control(Params param, float *val)
{
    switch(param){
    case STOP: { uchar buf[] = {0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01}; m_q.push(PortData{QByteArray((char*)buf, 7), 7, 1, false, false}); break; }
    case UP_LEFT:    send_ctrl_cmd(0x08); send_ctrl_cmd(0x04); break;
    case UP:         send_ctrl_cmd(0x08);                      break;
    case UP_RIGHT:   send_ctrl_cmd(0x08); send_ctrl_cmd(0x02); break;
    case LEFT:       send_ctrl_cmd(0x04);                      break;
    case SELF_CHECK:
    {
        if (QMessageBox::warning(nullptr, "PTZ", "Initialize?", QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel) == QMessageBox::StandardButton::Ok) {
            uchar buffer_out[7] = {0xFF, 0x01, 0x00, 0x07, 0x00, 0x77, 0x7F};
            m_q.push(PortData{QByteArray((char*)buffer_out, 7), 7, 0, false, false});
        }
        break;
    }
    case RIGHT:      send_ctrl_cmd(0x02);                      break;
    case DOWN_LEFT:  send_ctrl_cmd(0x10); send_ctrl_cmd(0x04); break;
    case DOWN:       send_ctrl_cmd(0x10);                      break;
    case DOWN_RIGHT: send_ctrl_cmd(0x10); send_ctrl_cmd(0x02); break;
    case ANGLE_H:
    case ANGLE_V:
    {
        if (!val) return -2;
        uchar buffer_out[7] = {0};
        buffer_out[0] = 0xFF;
        buffer_out[1] = 0x01;
        buffer_out[2] = 0x00;
        buffer_out[3] = param == ANGLE_H ? 0x51 : 0x53;
        buffer_out[4] = 0x00;
        buffer_out[5] = 0x00;
        buffer_out[6] = param == ANGLE_H ? 0x52 : 0x54;
        bool completed = false;
        uchar *data = nullptr;
        m_q.push(PortData{QByteArray((char*)buffer_out, 7), 7, 7, true, false, &completed, &data});
        if (!val) return -1;
        switch (param) {
        case ANGLE_H: m_angle_h = ((uint(data[4]) << 8) + uint(data[5])) / 100.0; m_angle_h += 360 * (int)((-m_angle_h) / 360); *val = m_angle_h; break;
        case ANGLE_V: m_angle_v = ((uint(data[4]) << 8) + uint(data[5])) / 100.0; m_angle_v -= m_angle_v > 40 ? 360 : 0;        *val = m_angle_v; break;
        }
        break;
    }
    case SET_H:
    {
        if (!val) return -2;
        int angle = (m_angle_h + 360 * (int)floor((-m_angle_h) / 360)) * 100;
        angle *= 100;
        uchar buffer_out[7] = {0};
        buffer_out[0] = 0xFF;
        buffer_out[1] = 0x01;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0x4B;
        buffer_out[4] = (angle >> 8) & 0xFF;
        buffer_out[5] = angle & 0xFF;
        uint sum = 0;
        for (int i = 1; i < 6; i++) sum += buffer_out[i];
        buffer_out[6] = sum & 0xFF;
        m_q.push(PortData{QByteArray((char*)buffer_out, 7), 7, 0, false, false});
    }
    case SET_V:
    {
        if (!val) return -2;
        int angle = std::max(std::min(m_angle_v, -40.f), 40.f) * 100;
        if (angle < 0) angle += 36000;
        uchar buffer_out[7] = {0};
        buffer_out[0] = 0xFF;
        buffer_out[1] = 0x01;
        buffer_out[2] = 0x00;
        buffer_out[3] = 0x4D;
        buffer_out[4] = (angle >> 8) & 0xFF;
        buffer_out[5] = angle & 0xFF;
        uint sum = 0;
        for (int i = 1; i < 6; i++) sum += buffer_out[i];
        buffer_out[6] = sum & 0xFF;
        m_q.push(PortData{QByteArray((char*)buffer_out, 7), 7, 0, false, false});
    }
    case SPEED: if (!val) return -1; m_ptz_speed = *val; m_ptz_speed = std::max(std::min(m_ptz_speed, 1), 64);
    case ADDRESS: break;
    default: break;
    }
}

void PTZThread::try_communicate()
{
    ControlPortThread::try_communicate();

    // TODO: disable self-check system
    if (!m_connected_to_serial && !m_connected_to_tcp) return;

    static uchar buf_h[] = {0xFF, 0x01, 0x00, 0x51, 0x00, 0x00, 0x52};
    static uchar buf_v[] = {0xFF, 0x01, 0x00, 0x53, 0x00, 0x00, 0x54};
    static QByteArray write[2] = {QByteArray((char*)buf_h, 7),
                                  QByteArray((char*)buf_v, 7)};

    m_q.push(PortData{write[m_write_idx], 7, 7, true, true, nullptr, nullptr});
    m_write_idx = (m_write_idx + 1) % 2;

//    QByteArray read = communicate_display(write[m_write_idx], 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) m_successive_count++;
//    else                             m_successive_count = 0;
//    read = communicate_display(write[m_write_idx], 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) m_successive_count++;
    //    else                             m_successive_count = 0;
}

void PTZThread::send_ctrl_cmd(uchar dir)
{
    static uchar buffer_out[7] = {0};
    buffer_out[0] = 0xFF;
    buffer_out[1] = 0x01;
    buffer_out[2] = 0x00;
    buffer_out[3] = dir;
    buffer_out[4] = dir < 5 ? m_ptz_speed : 0x00;
    buffer_out[5] = dir > 5 ? m_ptz_speed : 0x00;
    int sum = 0;
    for (int i = 1; i < 6; i++) sum += buffer_out[i];
    buffer_out[6] = sum & 0xFF;

    m_q.push(PortData{QByteArray((char*)buffer_out, 7), 7, 0, false, false});
}

InclinThread::InclinThread(QLabel *label, QLineEdit *edit, int index, DotIndicator *status_icon, QObject *parent) :
    ControlPortThread(label, edit, index, status_icon, parent)
{
//    m_time_interval = 990;
}

void InclinThread::try_communicate()
{
    ControlPortThread::try_communicate();

    // TODO: disable self-check system
    if (!m_connected_to_serial && !m_connected_to_tcp) return;

    m_q.push(PortData{QByteArray(), 0, 44, true, true, nullptr, nullptr});

//    QByteArray read = communicate_display(write[m_write_idx], 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) m_successive_count++;
//    else                             m_successive_count = 0;
//    read = communicate_display(write[m_write_idx], 7, 1, true, false);
//    if (read != QByteArray(1, 0x15)) m_successive_count++;
//    else                             m_successive_count = 0;
}
