#ifndef TIMEDQUEUE_H
#define TIMEDQUEUE_H

#include <QThread>
#include <QMutex>
#include <queue>

class StatusIcon;

class TimedQueue : public QThread {
    Q_OBJECT
public:
    TimedQueue(double expiration_time);
    ~TimedQueue();

    void set_display_label(StatusIcon *lbl);
    void add_to_q();
    void empty_q();
    int count();
    void stop();

protected:
    void run();

private:
    QMutex mtx;
    bool _quit;
    double expiration_time;
    std::queue<struct timespec> q;
    StatusIcon* fps_status;
};

#endif // TIMEDQUEUE_H
