#include "usbcan.h"

USBCAN::USBCAN():
    QObject{nullptr},
    timer(nullptr),
    interval(10)
{
    timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);
    timer->setInterval(interval);
}

USBCAN::~USBCAN()
{
    timer->stop();
    timer->deleteLater();
}
