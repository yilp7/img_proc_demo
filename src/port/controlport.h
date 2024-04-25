#ifndef CONTROLPORT_H
#define CONTROLPORT_H

#include "util/util.h"

class ControlPort : public QObject
{
    Q_OBJECT
public:
    enum ErrorCode {
        NO_ERR          = 0x00,
        SERIAL_NOT_OPEN = 0x01,
    };

    enum PortStatus {
        NOT_CONNECTED    = 0x00,
        SERIAL_CONNECTED = 0x01,
        TCP_CONNECTED    = 0x10,
        ALL_CONNECTED    = SERIAL_CONNECTED | TCP_CONNECTED,
    };

    explicit ControlPort(int index = -1);
    ~ControlPort();

    qint32 get_baudrate();
    void   set_baudrate(qint32 baudrate);
    bool   get_use_tcp();
    void   set_use_tcp(bool use_tcp);
    qint32 get_port_status();

    void share_port_from(ControlPort *port);

#if ENABLE_PORT_JSON
    virtual nlohmann::json to_json() = 0;
#endif

signals:
    void connect_to_serial(QString port_num, qint32 baudrate = QSerialPort::Baud9600);
    void connect_to_tcp(QString ip, ushort port);
    void disconnect_from(bool tcp);
    void port_status_updated();
    void serial_port_status_updated(bool connected);
    void tcp_port_status_updated(bool connected);
    void port_io_log(QString text);
    void send_data(QByteArray write, uint read_size = 0, uint read_timeout = 40, bool heartbeat = false);

#if ENABLE_PORT_JSON
    virtual void load_json(const nlohmann::json &j);
#endif

protected:
    QByteArray generate_ba(uchar *data, int len);

public slots:

protected slots:
    virtual bool connect_to_serial_port(QString port_num, qint32 baudrate = QSerialPort::Baud9600);
    virtual bool connect_to_tcp_port(QString ip, ushort port);
    virtual void disconnect_from_port(bool tcp = false);
    virtual void try_communicate();
    void update_timer_interval(uint interval = 0);
    void communicate(QByteArray write, uint read_size = 0, uint read_timeout = 40, bool heartbeat = false);
    void handle_received();

#if ENABLE_PORT_JSON
    virtual void load_from_json(const nlohmann::json &j) = 0;
#endif

protected:
    int idx;

    QMutex write_mutex;
    QMutex read_mutex;
    QMutex retrieve_mutex;

    QTimer *timer;
    uint   interval;

    bool        use_tcp;
    QSerialPort *serial_port;
    QTcpSocket  *tcp_port;
    ControlPort *shared_port;

    QByteArray received;
    QByteArray last_read;

    uint successive_count;
    bool connected_to_serial;
    bool connected_to_tcp;

};

#endif // CONTROLPORT_H
