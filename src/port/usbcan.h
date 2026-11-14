#ifndef USBCAN_H
#define USBCAN_H

#include "ECanVci.h"
#include "util/util.h"
#include "iptzcontroller.h"

class USBCAN : public QObject, public IPTZController
{
    Q_OBJECT
public:
    enum Operation {
        START      = 0x00,
        OFF        = 0x01,
        SELF_CHECK = 0x02,
        SERVO      = 0x03,
        LOCK       = 0x04,
        LOCK_LOCK  = 0x05,
        FOLLOW     = 0x06,
        STOP       = 0x07,
        LEFT       = 0x08,
        RIGHT      = 0x09,
        UP         = 0x0A,
        DOWN       = 0x0B,
        POSITION   = 0x0C,
        PITCH      = 0x0D,
    };

    explicit USBCAN();
    ~USBCAN();

    bool is_connected() const { return m_connected; }
    void update_timer_interval(uint interval = 0);

    // IPTZController interface
    void ptz_move(int direction, int speed) override;
    void ptz_stop() override;
    void ptz_set_angle(float h, float v) override;
    void ptz_set_angle_h(float h) override;
    void ptz_set_angle_v(float v) override;
    float ptz_get_angle_h() const override { return m_position; }
    float ptz_get_angle_v() const override { return m_pitch; }
    bool ptz_is_connected() const override { return m_connected; }
    QObject *ptz_qobject() override { return this; }
    void communicate(QByteArray write, uint read_size = 0, uint read_timeout = 40, bool heartbeat = false);

public slots:
    bool connect_to_device();
    void disconnect_from_device();
    void transmit_data(qint32 op);
    void device_control(qint32 op, float val);
    void handle_received();
    void transmit_power_status();
    void transmit_ptz_status();

signals:
    void connect_to();
    void disconnect_from();
    void transmit(qint32 param);
    void control(qint32 param, float val);
    void angle_updated(float h, float v);
    void connection_status_changed(bool connected);

private:
    DWORD m_device_type;
    DWORD m_device_index;
    DWORD m_can_index;

    bool m_connected;
    QMutex m_write_mutex;
    QMutex m_read_mutex;
    QMutex m_retrieve_mutex;

    QTimer *m_timer_r5;
    QTimer *m_timer_t20;
    QTimer *m_timer_t200;
    QTimer *m_timer_u100;

    uchar m_power;
    float m_zero_step = 0;
    float m_pos_step;
    float m_pit_step;
    float m_position;
    float m_pitch;

    QByteArray m_received;
    QByteArray m_last_read;
};

#endif // USBCAN_H
