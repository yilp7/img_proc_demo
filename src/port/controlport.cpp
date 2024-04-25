#include "controlport.h"

ControlPort::ControlPort(int index) :
    QObject{nullptr},
    idx(index),
    timer(nullptr),
    interval(1000),
    use_tcp(false),
    serial_port(nullptr),
    tcp_port(nullptr),
    shared_port(nullptr),
    successive_count(0),
    connected_to_serial(false),
    connected_to_tcp(false)
{
    serial_port = new QSerialPort(this);
    tcp_port = new QTcpSocket(this);
    timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);
    timer->setInterval(interval);

    connect(serial_port, &QSerialPort::readyRead, this, &ControlPort::handle_received);
    connect(tcp_port, &QTcpSocket::readyRead, this, &ControlPort::handle_received);
    connect(timer, &QTimer::timeout, this, &ControlPort::try_communicate);
    connect(this, &ControlPort::connect_to_serial, this, &ControlPort::connect_to_serial_port);
    connect(this, &ControlPort::connect_to_tcp, this, &ControlPort::connect_to_tcp_port);
    connect(this, &ControlPort::disconnect_from, this, &ControlPort::disconnect_from_port);
    connect(this, &ControlPort::send_data, this, &ControlPort::communicate, Qt::QueuedConnection);

#if ENABLE_PORT_JSON
    connect(this, &ControlPort::load_json, this, &ControlPort::load_from_json);
#endif
}

ControlPort::~ControlPort()
{
    if (serial_port->isOpen()) serial_port->close();
    serial_port->deleteLater();

    timer->stop();
    timer->deleteLater();
}

qint32 ControlPort::get_baudrate()
{
    return serial_port->isOpen() ? serial_port->baudRate() : 0;
}

void ControlPort::set_baudrate(qint32 baudrate)
{
    if (serial_port->isOpen()) serial_port->setBaudRate(baudrate);
}

bool ControlPort::get_use_tcp()
{
    return tcp_port->isOpen() ? use_tcp : false;
}

void ControlPort::set_use_tcp(bool use_tcp)
{
    this->use_tcp = use_tcp;
}

int ControlPort::get_port_status()
{
    return (connected_to_tcp ? PortStatus::TCP_CONNECTED : PortStatus::NOT_CONNECTED) |
           (connected_to_serial ? PortStatus::SERIAL_CONNECTED : PortStatus::NOT_CONNECTED);
}

void ControlPort::share_port_from(ControlPort *port)
{
    shared_port = port;
}

QByteArray ControlPort::generate_ba(uchar *data, int len)
{
    QByteArray ba((char*)data, len);
    delete[] data;
    return ba;
}

bool ControlPort::connect_to_serial_port(QString port_num, qint32 baudrate)
{
    if (serial_port->isOpen()) serial_port->close();

    QString port_name;
#if WIN32
    port_name = "COM" + port_num;
#else
#endif
    serial_port->setPortName(port_name);
    serial_port->setBaudRate(baudrate); // default: QSerialPort::Baud9600
    serial_port->setDataBits(QSerialPort::Data8);
    serial_port->setParity(QSerialPort::NoParity);
    serial_port->setStopBits(QSerialPort::OneStop);
    serial_port->setFlowControl(QSerialPort::NoFlowControl);

//    qDebug() << "open port in" << QThread::currentThread();
    timer->stop();
    if (serial_port->open(QIODevice::ReadWrite)) {
        timer->start();
        serial_port->clear();
        qDebug("%s connected", qPrintable(port_name));
        connected_to_serial = true;
//        emit serial_port_status_updated(connected_to_serial = true);
        successive_count = 0;
    }
    else {
        timer->stop();
        connected_to_serial = false;
//        emit serial_port_status_updated(connected_to_serial = false);
    }
    emit port_status_updated();

#if ENABLE_USER_DEFAULT
    // TODO: test for user utilities
    if (idx < 0) return connected_to_serial;
    FILE *f = fopen("user_default", "rb+");
    if (!f) return connected_to_serial;
    uchar port_num_uchar = port_num.toUInt() & 0xFF;
    fseek(f, 5 + idx, SEEK_SET);
    fwrite(&port_num_uchar, 1, 1, f);
    fclose(f);
#endif
    return connected_to_serial;
}

bool ControlPort::connect_to_tcp_port(QString ip, ushort port)
{
    if (tcp_port->isOpen()) tcp_port->close();

    tcp_port->connectToHost(ip, port);
    use_tcp = connected_to_tcp = tcp_port->waitForConnected(1000);
    if (connected_to_tcp) timer->start();
    else                  timer->stop(), tcp_port->close();
//    emit tcp_port_status_updated(use_tcp = connected_to_tcp = tcp_port->waitForConnected(1000));
    emit port_status_updated();
    return use_tcp;
}

void ControlPort::disconnect_from_port(bool tcp)
{
    if (tcp) tcp_port->close(), use_tcp = connected_to_tcp = false;
    else     serial_port->close(), connected_to_serial = false;
    timer->stop();
    emit port_status_updated();
}

void ControlPort::try_communicate()
{
    qDebug() << "triggered at " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    return;
}

void ControlPort::update_timer_interval(uint interval)
{
    timer->stop();
    if (!interval) return;
    timer->setInterval(interval);
    timer->start();
}

void ControlPort::communicate(QByteArray write, uint read_size, uint read_timeout, bool heartbeat)
{
    if (shared_port) return shared_port->emit send_data(write, read_size, read_timeout, heartbeat);

//    qDebug() << "port communication in" << QThread::currentThread();
    QByteArray read;
    QString str_s("sent    "), str_r("received");

    for (int i = 0; i < write.size(); i++) str_s += QString::asprintf(" %02X", (uchar)write[i]);
    if (!heartbeat) emit port_io_log(str_s);

    read_mutex.lock();
    received.clear();
    read_mutex.unlock();

    write_mutex.lock();
    QIODevice *port = use_tcp ? (QIODevice*)tcp_port : (QIODevice*)serial_port;

    if (!port->isOpen()) {
        write_mutex.unlock();
        return;
    }
    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "sent    " << write.toHex(' ').toUpper();

    port->write(write.constData(), write.size());
//    port->waitForBytesWritten(10);
    write_mutex.unlock();

    static QElapsedTimer read_timer;
    if (read_timer.elapsed()) read_timer.restart();
    else                      read_timer.start();
    while ((read_size == 0 || received.length() < read_size) && read_timer.elapsed() < read_timeout) port->waitForReadyRead(5);

    read_mutex.lock();
    if (read_size) {
        read = received.left(read_size);
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "received" << read.toHex(' ').toUpper();
//        qDebug() << received.toHex(' ').toUpper();
        received = received.right(received.length() - read.size());
//        qDebug() << received.toHex(' ').toUpper();
    }
    else {
        read = received;
        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "received" << read.toHex(' ').toUpper();
        received.clear();
    }
    read_mutex.unlock();

    retrieve_mutex.lock();
    last_read = read;
    retrieve_mutex.unlock();
    for (int i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
    if (!heartbeat) emit port_io_log(str_r);

    return;
}

void ControlPort::handle_received()
{
    QByteArray read;
    read_mutex.lock();
    if (use_tcp) read = tcp_port->readAll();
    else         read = serial_port->readAll();
    read_mutex.unlock();
    //    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "received" << read.toHex(' ').toUpper();
    received += read;
    //    qDebug() << "received in" << QThread::currentThread();
}
