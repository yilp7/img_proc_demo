#ifndef UDPPTZ_H
#define UDPPTZ_H

#include "util/util.h"

class UDPPTZ : public QObject
{
    Q_OBJECT
public:
    enum OPERATION {
        STANDBY         = 0x01,
        RETURN_ZERO     = 0x02,
        SCAN            = 0x03,
        MANUAL_SEARCH   = 0x04,
        ANGLE_POSITION  = 0x07,
    };

    explicit UDPPTZ();
    ~UDPPTZ();

    bool is_connected() const { 
        return connected && udp_socket != nullptr;
        // TODO: Implement real-time connection check based on received data
        // qint64 current_time = QDateTime::currentMSecsSinceEpoch();
        // return connected && (current_time - last_received_time) < CONNECTION_TIMEOUT_MS; 
    }
    void set_velocities(float h_vel, float v_vel) { horizontal_velocity = h_vel; vertical_velocity = v_vel; }
    void update_timer_interval(uint interval = 0);
    void communicate(QByteArray write, uint read_size = 0, uint read_timeout = 40, bool heartbeat = false);

public slots:
    bool connect_to_device();
    void disconnect_from_device();
    void transmit_data(qint32 op);
    void device_control(qint32 op, float val);
    void handle_received();
    void transmit_status();

signals:
    void connect_to();
    void disconnect_from();
    void transmit(qint32 param);
    void control(qint32 param, float val);
    void angle_updated(float _h, float _v);
    void status_updated(qint32 status);
    void connection_status_changed(bool connected);

private:
    QByteArray create_command_frame(qint32 op, float horizontal = 0, float vertical = 0, float scan_range = 0);
    uchar calculate_checksum(const QByteArray &data);
    void parse_status_frame(const QByteArray &data);
    qint16 float_to_int16(float value);
    float int16_to_float(qint16 value);

private:
    QUdpSocket *udp_socket;
    QHostAddress target_ip;
    quint16 target_port;
    QHostAddress local_ip;
    quint16 local_port;
    
    bool connected;
    QMutex socket_mutex;
    QMutex retrieve_mutex;

    QTimer *timer_tx;
    QTimer *timer_rx;

    qint32 current_operation;
    float horizontal_angle;        // Current angle reported by device
    float vertical_angle;          // Current angle reported by device
    float target_horizontal_angle; // Target angle set by user
    float target_vertical_angle;   // Target angle set by user
    float horizontal_velocity;
    float vertical_velocity;
    float scan_range_horizontal;
    float scan_range_vertical;
    qint32 servo_status;

    QByteArray received;
    QByteArray last_read;
    
    // Connection monitoring
    qint64 last_received_time;
    static const qint64 CONNECTION_TIMEOUT_MS = 3000; // 3 seconds timeout
};

#endif // UDPPTZ_H