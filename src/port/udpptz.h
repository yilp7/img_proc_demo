#ifndef UDPPTZ_H
#define UDPPTZ_H

#include "util/util.h"

class UDPPTZ : public QObject
{
    Q_OBJECT
public:
    enum OPERATION {
        // PTZ operations
        STOP            = 0x00,
        STANDBY         = 0x01,
        RETURN_ZERO     = 0x02,
        SCAN            = 0x03,
        MANUAL_SEARCH   = 0x04,
        ANGLE_POSITION  = 0x07,
        ANGLE_H         = 0x08,
        ANGLE_V         = 0x09,

        // Visible light operations
        VL_DEFOG_ON     = 0x23,
        VL_DEFOG_OFF    = 0x24,
        VL_ZOOM_IN      = 0x25,
        VL_ZOOM_OUT     = 0x26,
        VL_FOCUS_FAR    = 0x27,
        VL_FOCUS_NEAR   = 0x28,

        // Laser ranging
        LASER_ON        = 0x2D,
        LASER_OFF       = 0x2E,

        // Video tracking gates
        TRACK_GATE_128  = 0x30,
        TRACK_GATE_64   = 0x31,
        TRACK_GATE_32   = 0x32,

        // Video source switching
        SWITCH_VL       = 0x33,
        SWITCH_IR       = 0x34,
        OSD_FULL        = 0x35,
        OSD_HALF        = 0x36,
        OSD_HIDE        = 0x37,

        // Infrared operations
        IR_ZOOM_IN      = 0x41,
        IR_ZOOM_OUT     = 0x42,
        IR_FOCUS_FAR    = 0x43,
        IR_FOCUS_NEAR   = 0x44,
        IR_POWER_ON     = 0x47,
        IR_POWER_OFF    = 0x48,
        IR_AUTO_FOCUS   = 0x49,
        IR_WHITE_HOT    = 0x87,
        IR_BLACK_HOT    = 0x88,
    };

    explicit UDPPTZ();
    ~UDPPTZ();

    void set_target(QHostAddress target, quint16 port);
    void set_listen(QHostAddress listen, quint16 port);
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
//    QByteArray create_command_frame(qint32 op, float horizontal = 0, float vertical = 0, float scan_range = 326.39);
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
