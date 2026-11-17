#include "util/timedqueue.h"

#include <QElapsedTimer>
#include <QMetaObject>
#include <QString>
#include <ctime>

TimedQueue::TimedQueue(double time) : QThread(), m_quit(false), m_expiration_time(time), m_fps_status(nullptr) {}
TimedQueue::~TimedQueue()
{
    m_quit = true;
    while (!m_q.empty()) m_q.pop();
}

void TimedQueue::set_display_label(QLabel *lbl) { m_fps_status = lbl; }

void TimedQueue::add_to_q()
{
    struct timespec t;
    timespec_get(&t, TIME_UTC);
    m_mtx.lock();
    m_q.push(t);
    m_mtx.unlock();
}

void TimedQueue::empty_q()
{
    m_mtx.lock();
    std::queue<struct timespec>().swap(m_q);
    m_mtx.unlock();
}

int TimedQueue::count() { return m_q.size(); }

void TimedQueue::stop()
{
    m_quit = true;
    quit();
    wait();
}

// return difference in milliseconds
inline double diff_timespec(struct timespec &t1, struct timespec &t2)
{
    return (t2.tv_sec - t1.tv_sec) * 1e3 + (t2.tv_nsec - t1.tv_nsec) / 1e6;
}

void TimedQueue::run()
{
    QElapsedTimer et;
    while (!m_quit) {
#if 1
        QThread::msleep(5);
        struct timespec t;
        timespec_get(&t, TIME_UTC);
        if (m_q.empty()) { continue; }
        m_mtx.lock();
        if (diff_timespec(m_q.front(), t) > m_expiration_time * 1e3) m_q.pop();
        if (et.isValid() && et.elapsed() < 200) { m_mtx.unlock(); continue; }
        // the label belongs to the GUI thread
        QMetaObject::invokeMethod(m_fps_status, "setText", Qt::QueuedConnection,
                                  Q_ARG(QString, QString::number(m_q.size() ? ((m_q.size() - 1) / diff_timespec(m_q.front(), m_q.back())) * 1000 : 0.f, 'f', 2) + " fps"));
        m_mtx.unlock();
        et.restart();
#endif
    }
}
