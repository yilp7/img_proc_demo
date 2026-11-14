#ifndef UDPPTZ_H
#define UDPPTZ_H

#include "util/util.h"
#include "iptzcontroller.h"

class UDPPTZ : public QObject, public IPTZController
{
    Q_OBJECT
public:
    // Device protocol variant. Frame layout and op codes are shared; the variant
    // decides how the scan-range bytes (8-9) are filled and the TX cadence.
    enum Protocol {
        PROTOCOL_500 = 500, // bytes 8-9 carry scan_range; 20 ms TX cadence
        PROTOCOL_530 = 530, // bytes 8-9 always 0x7F7F; 10 ms TX cadence
    };

    enum Operation {
        // PTZ operations
        STOP            = 0x00,
        STANDBY         = 0x01,
        RETURN_ZERO     = 0x02,
        SCAN            = 0x03,
        MANUAL_SEARCH   = 0x04,
        FOLLOW          = 0x05,
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
    void set_protocol(Protocol p);
    Protocol get_protocol() const { return m_protocol; }
    bool is_connected() const
    {
        return m_connected && m_udp_socket != nullptr;
        // TODO: Implement real-time connection check based on received data
        // qint64 current_time = QDateTime::currentMSecsSinceEpoch();
        // return m_connected && (current_time - m_last_received_time) < CONNECTION_TIMEOUT_MS;
    }
    void update_timer_interval(uint interval = 0);

    // IPTZController interface
    void ptz_move(int direction, int speed) override;
    void ptz_move_2d(int direction, int h_speed, int v_speed) override;
    void ptz_move_velocity(float h_velocity, float v_velocity);
    void ptz_stop() override;
    void ptz_set_angle(float h, float v) override;
    void ptz_set_angle_h(float h) override;
    void ptz_set_angle_v(float v) override;
    void ptz_follow(float h, float v);
    float ptz_get_angle_h() const override { return m_horizontal_angle; }
    float ptz_get_angle_v() const override { return m_vertical_angle; }
    bool ptz_is_connected() const override { return is_connected(); }
    QObject *ptz_qobject() override { return this; }
    void communicate(QByteArray write, uint read_size = 0, uint read_timeout = 40, bool heartbeat = false);

public slots:
    bool connect_to_device();
    void disconnect_from_device();
    void transmit_data(qint32 op);
    void device_control(qint32 op, float val);
    void apply_manual_velocity(float h_vel, float v_vel);
    void apply_follow_target(float h, float v);
    void apply_angle_target(float h, float v);
    void handle_received();
    void transmit_status();

signals:
    void connect_to();
    void disconnect_from();
    void transmit(qint32 param);
    void control(qint32 param, float val);
    void angle_updated(float h, float v);
    void status_updated(qint32 status);
    void connection_status_changed(bool connected);

private:
//    QByteArray create_command_frame(qint32 op, float horizontal = 0, float vertical = 0, float scan_range = 326.39);
    QByteArray create_command_frame(qint32 op, float horizontal = 0, float vertical = 0, float scan_range = 0);
    uchar calculate_checksum(const QByteArray &data);
    void parse_status_frame(const QByteArray &data);
    qint16 float_to_int16(float value);
    float int16_to_float(qint16 value);
    void dispatch_manual_velocity(float h_vel, float v_vel, Qt::ConnectionType connection);
    void dispatch_follow_target(float h, float v, Qt::ConnectionType connection);
    void dispatch_angle_target(float h, float v, Qt::ConnectionType connection);
    void set_velocities(float h_vel, float v_vel) { m_horizontal_velocity = h_vel; m_vertical_velocity = v_vel; }

private:
    QUdpSocket *m_udp_socket;
    QHostAddress m_target_ip;
    quint16 m_target_port;
    QHostAddress m_local_ip;
    quint16 m_local_port;

    bool m_connected;
    QMutex m_socket_mutex;
    QMutex m_retrieve_mutex;

    QTimer *m_timer_tx;
    QTimer *m_timer_rx;

    qint32 m_current_operation;
    float m_horizontal_angle;        // Current angle reported by device
    float m_vertical_angle;          // Current angle reported by device
    float m_target_horizontal_angle; // Target angle set by user
    float m_target_vertical_angle;   // Target angle set by user
    float m_horizontal_velocity;
    float m_vertical_velocity;
    float m_scan_range_horizontal;
    float m_scan_range_vertical;
    qint32 m_servo_status;

    QByteArray m_received;
    QByteArray m_last_read;

    // Connection monitoring
    qint64 m_last_received_time;
    static const qint64 CONNECTION_TIMEOUT_MS = 3000; // 3 seconds timeout

    Protocol m_protocol;
};

#endif // UDPPTZ_H
