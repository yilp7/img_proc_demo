#ifndef USBCAN_H
#define USBCAN_H

#include "ECanVci.h"
#include "util/util.h"

class USBCAN : public QObject
{
    Q_OBJECT
public:
    enum OPERATION {
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

    bool is_connected() const { return connected; }
    void update_timer_interval(uint interval = 0);
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
    void angle_updated(float _h, float _v);
    void connection_status_changed(bool connected);

private:
    DWORD device_type;
    DWORD device_index;
    DWORD CAN_index;

    bool connected;
    QMutex write_mutex;
    QMutex read_mutex;
    QMutex retrieve_mutex;

    QTimer *timer_r5;
    QTimer *timer_t20;
    QTimer *timer_t200;
    QTimer *timer_u100;

    uchar power;
    float zero_step = 0;
    float pos_step;
    float pit_step;
    float position;
    float pitch;

    QByteArray received;
    QByteArray last_read;
};

#endif // USBCAN_H
