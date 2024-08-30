#ifndef USBCAN_H
#define USBCAN_H

#include "util/util.h"

class USBCAN : public QObject
{
    Q_OBJECT
public:
    explicit USBCAN();
    ~USBCAN();

    bool connect_to_device();
    void disconnect_from_device();
    void try_communicate();
    void update_timer_interval(uint interval = 0);
    void communicate(QByteArray write, uint read_size = 0, uint read_timeout = 40, bool heartbeat = false);
    void handle_received();

signals:

private:
    QMutex write_mutex;
    QMutex read_mutex;
    QMutex retrieve_mutex;

    QTimer *timer;
    uint   interval;

    QByteArray received;
    QByteArray last_read;
};

#endif // USBCAN_H
