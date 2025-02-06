#include "serialserver.h"
#include "ui_serialserver.h"
#include <QDebug>

SerialServer::SerialServer(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SerialServer)
{
    ui->setupUi(this);

#if 0 //TODO:
    QStringList types = QStringList() << "None" << "TCU" << "Lens" << "Lens-2" << "RF" << "PTZ" << "Webcam";
    port_type_lists[0] = ui->LST_PORT_TYPE_1;
    port_type_lists[1] = ui->LST_PORT_TYPE_2;
    port_type_lists[2] = ui->LST_PORT_TYPE_3;
    port_type_lists[3] = ui->LST_PORT_TYPE_4;
    ui->LST_PORT_TYPE_1->addItems(types);
    ui->LST_PORT_TYPE_2->addItems(types);
    ui->LST_PORT_TYPE_3->addItems(types);
    ui->LST_PORT_TYPE_4->addItems(types);

    ui->LST_PORT_TYPE_1->setCurrentIndex(0);
    ui->LST_PORT_TYPE_2->setCurrentIndex(1);
    ui->LST_PORT_TYPE_3->setCurrentIndex(3);
    ui->LST_PORT_TYPE_4->setCurrentIndex(4);
#endif

    ui->LST_PORT_TYPE_1->addItem("TCU");
    ui->LST_PORT_TYPE_2->addItem("Lens");
    ui->LST_PORT_TYPE_3->addItems(QStringList() << "Lens" << "RF");
    ui->LST_PORT_TYPE_3->setCurrentIndex(0);
    ui->LST_PORT_TYPE_4->addItems(QStringList() << "PTZ" << "Webcam");
    ui->LST_PORT_TYPE_4->setCurrentIndex(0);
    ui->LST_PORT_TYPE_1->installEventFilter(this);
    ui->LST_PORT_TYPE_2->installEventFilter(this);
    ui->LST_PORT_TYPE_3->installEventFilter(this);
    ui->LST_PORT_TYPE_4->installEventFilter(this);
}

SerialServer::~SerialServer()
{
    delete ui;
}

bool SerialServer::eventFilter(QObject *obj, QEvent *event)
{
    if (qobject_cast<QComboBox*>(obj) && event->type() == QEvent::MouseMove) return true;
    return QDialog::eventFilter(obj, event);
}
