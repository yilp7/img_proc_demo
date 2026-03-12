#include "controller/rfcontroller.h"
#include "ui_user_panel.h"

RFController::RFController(QObject *parent)
    : QObject(parent),
      m_ui(nullptr)
{
}

void RFController::init(Ui::UserPanel *ui)
{
    m_ui = ui;
}

void RFController::update_distance(double distance)
{
    m_ui->DISTANCE->setText(QString::asprintf("%.2f m", distance));
}
