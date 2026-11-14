#include "controlport.h"

ControlPort::ControlPort(int index) :
    QObject{nullptr},
    m_idx(index),
    m_timer(nullptr),
    m_interval(1000),
    m_use_tcp(false),
    m_serial_port(nullptr),
    m_tcp_port(nullptr),
    m_shared_port(nullptr),
    m_successive_count(0),
    m_connected_to_serial(false),
    m_connected_to_tcp(false)
{
    m_serial_port = new QSerialPort(this);
    m_tcp_port = new QTcpSocket(this);
    // Bypass the system proxy: when Windows reports an HTTP proxy, Qt refuses raw TCP
    // with "The proxy type is invalid for this operation" and sends nothing.
    m_tcp_port->setProxy(QNetworkProxy::NoProxy);
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(m_interval);

    connect(m_serial_port, &QSerialPort::readyRead, this, &ControlPort::handle_received);
    connect(m_tcp_port, &QTcpSocket::readyRead, this, &ControlPort::handle_received);
    connect(m_timer, &QTimer::timeout, this, &ControlPort::try_communicate);
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
    if (m_serial_port->isOpen()) m_serial_port->close();
    m_serial_port->deleteLater();

    m_timer->stop();
    m_timer->deleteLater();
}

qint32 ControlPort::get_baudrate()
{
    return m_serial_port->isOpen() ? m_serial_port->baudRate() : 0;
}

void ControlPort::set_baudrate(qint32 baudrate)
{
    if (m_serial_port->isOpen()) m_serial_port->setBaudRate(baudrate);
}

bool ControlPort::get_use_tcp()
{
    return m_tcp_port->isOpen() ? m_use_tcp : false;
}

void ControlPort::set_use_tcp(bool use_tcp)
{
    this->m_use_tcp = use_tcp;
}

int ControlPort::get_port_status() const
{
    return (m_connected_to_tcp ? PortStatus::TCP_CONNECTED : PortStatus::NOT_CONNECTED) |
           (m_connected_to_serial ? PortStatus::SERIAL_CONNECTED : PortStatus::NOT_CONNECTED);
}

void ControlPort::share_port_from(ControlPort *port)
{
    m_shared_port = port;
}

QByteArray ControlPort::generate_ba(uchar *data, int len)
{
    QByteArray ba((char*)data, len);
    delete[] data;
    return ba;
}

bool ControlPort::connect_to_serial_port(QString port_num, qint32 baudrate)
{
    if (m_serial_port->isOpen()) m_serial_port->close();

    QString port_name;
#if WIN32
    port_name = "COM" + port_num;
#else
#endif
    m_serial_port->setPortName(port_name);
    m_serial_port->setBaudRate(baudrate); // default: QSerialPort::Baud9600
    m_serial_port->setDataBits(QSerialPort::Data8);
    m_serial_port->setParity(QSerialPort::NoParity);
    m_serial_port->setStopBits(QSerialPort::OneStop);
    m_serial_port->setFlowControl(QSerialPort::NoFlowControl);

//    qDebug() << "open port in" << QThread::currentThread();
    m_timer->stop();
    if (m_serial_port->open(QIODevice::ReadWrite)) {
        m_timer->start();
        m_serial_port->clear();
        qDebug("%s connected", qPrintable(port_name));
        m_connected_to_serial = true;
//        emit serial_port_status_updated(m_connected_to_serial = true);
        m_successive_count = 0;
    }
    else {
        m_timer->stop();
        m_connected_to_serial = false;
//        emit serial_port_status_updated(m_connected_to_serial = false);
    }
    emit port_status_updated();

    // COM port saving will be handled by UserPanel through config
    return m_connected_to_serial;
}

bool ControlPort::connect_to_tcp_port(QString ip, ushort port)
{
    if (m_tcp_port->isOpen()) m_tcp_port->close();

    qDebug() << "[ControlPort" << m_idx << "] connecting to" << ip << ":" << port;
    m_tcp_port->connectToHost(ip, port);
    m_use_tcp = m_connected_to_tcp = m_tcp_port->waitForConnected(1000);
    qDebug() << "[ControlPort" << m_idx << "] connected=" << m_connected_to_tcp
             << "state=" << m_tcp_port->state()
             << "local=" << m_tcp_port->localAddress().toString() << ":" << m_tcp_port->localPort()
             << "peer="  << m_tcp_port->peerAddress().toString()  << ":" << m_tcp_port->peerPort()
             << "err="   << m_tcp_port->errorString();
    if (m_connected_to_tcp) m_timer->start();
    else                    m_timer->stop(), m_tcp_port->close();
//    emit tcp_port_status_updated(m_use_tcp = m_connected_to_tcp = m_tcp_port->waitForConnected(1000));
    emit port_status_updated();
    return m_use_tcp;
}

void ControlPort::disconnect_from_port(bool tcp)
{
    if (tcp) m_tcp_port->close(), m_use_tcp = m_connected_to_tcp = false;
    else     m_serial_port->close(), m_connected_to_serial = false;
    m_timer->stop();
    emit port_status_updated();
}

void ControlPort::try_communicate()
{
//    qDebug() << "triggered at " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    return;
}

void ControlPort::update_timer_interval(uint interval)
{
    m_timer->stop();
    if (!interval) return;
    m_timer->setInterval(interval);
    m_timer->start();
}

void ControlPort::communicate(QByteArray write, uint read_size, uint read_timeout, bool heartbeat)
{
    if (m_shared_port) return m_shared_port->emit send_data(write, read_size, read_timeout, heartbeat);

//    qDebug() << "port communication in" << QThread::currentThread();
    QByteArray read;
    QString str_s("sent    "), str_r("received");

    for (int i = 0; i < write.size(); i++) str_s += QString::asprintf(" %02X", (uchar)write[i]);
    if (!heartbeat) emit port_io_log(str_s);

    m_read_mutex.lock();
    m_received.clear();
    m_read_mutex.unlock();

    m_write_mutex.lock();
    QIODevice *port = m_use_tcp ? (QIODevice*)m_tcp_port : (QIODevice*)m_serial_port;

    if (!port->isOpen()) {
        m_write_mutex.unlock();
        return;
    }
//    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "sent    " << write.toHex(' ').toUpper();

    port->write(write.constData(), write.size());
//    while (port->waitForBytesWritten(10)) ;
    m_write_mutex.unlock();

    static QElapsedTimer read_timer;
    if (read_timer.elapsed()) read_timer.restart();
    else                      read_timer.start();
    while ((read_size == 0 || m_received.length() < read_size) && read_timer.elapsed() < read_timeout) port->waitForReadyRead(5);

    m_read_mutex.lock();
    if (read_size) {
        read = m_received.left(read_size);
//        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "received" << read.toHex(' ').toUpper();
//        qDebug() << m_received.toHex(' ').toUpper();
        m_received = m_received.right(m_received.length() - read.size());
//        qDebug() << m_received.toHex(' ').toUpper();
    }
    else {
        read = m_received;
//        qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "received" << read.toHex(' ').toUpper();
        m_received.clear();
    }
    m_read_mutex.unlock();

    m_retrieve_mutex.lock();
    m_last_read = read;
    m_retrieve_mutex.unlock();
    for (int i = 0; i < read.size(); i++) str_r += QString::asprintf(" %02X", (uchar)read[i]);
    if (!heartbeat) emit port_io_log(str_r);

    QThread::msleep(10);

    return;
}

void ControlPort::handle_received()
{
    QByteArray read;
    m_read_mutex.lock();
    if (m_use_tcp) read = m_tcp_port->readAll();
    else           read = m_serial_port->readAll();
    m_read_mutex.unlock();
    //    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "received" << read.toHex(' ').toUpper();
    m_received += read;
    //    qDebug() << "received in" << QThread::currentThread();
}
