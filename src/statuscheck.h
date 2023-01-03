#ifndef STATUSCHECKTHREAD_H
#define STATUSCHECKTHREAD_H

#include <QThread>

class StatusCheckThread : public QThread
{
public:
    StatusCheckThread(void *info);
    ~StatusCheckThread();

protected:
    void run();

private:
    QMutex* port_mutex;
    int     time_interval;

};

#endif // STATUSCHECKTHREAD_H
