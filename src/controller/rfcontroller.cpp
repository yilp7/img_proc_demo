#include "controller/rfcontroller.h"

RFController::RFController(QObject *parent)
    : QObject(parent)
{
}

void RFController::update_distance(double distance)
{
    emit distance_text_updated(QString::asprintf("%.2f m", distance));
}
