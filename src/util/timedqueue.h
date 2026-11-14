#ifndef TIMEDQUEUE_H
#define TIMEDQUEUE_H

#include <QThread>
#include <QMutex>
#include <QLabel>
#include <queue>

class TimedQueue : public QThread
{
    Q_OBJECT
public:
    TimedQueue(double expiration_time);
    ~TimedQueue();

    void set_display_label(QLabel *lbl);
    void add_to_q();
    void empty_q();
    int count();
    void stop();

protected:
    void run();

private:
    QMutex m_mtx;
    bool m_quit;
    double m_expiration_time;
    std::queue<struct timespec> m_q;
    QLabel *m_fps_status;
};

#endif // TIMEDQUEUE_H
