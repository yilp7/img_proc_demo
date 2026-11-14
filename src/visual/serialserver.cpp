#include "serialserver.h"
#include "ui_serialserver.h"
#include <QDebug>

SerialServer::SerialServer(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SerialServer)
{
    m_ui->setupUi(this);

#if 0 //TODO:
    QStringList types = QStringList() << "None" << "TCU" << "Lens" << "Lens-2" << "RF" << "PTZ" << "Webcam";
    m_port_type_lists[0] = m_ui->LST_PORT_TYPE_1;
    m_port_type_lists[1] = m_ui->LST_PORT_TYPE_2;
    m_port_type_lists[2] = m_ui->LST_PORT_TYPE_3;
    m_port_type_lists[3] = m_ui->LST_PORT_TYPE_4;
    m_ui->LST_PORT_TYPE_1->addItems(types);
    m_ui->LST_PORT_TYPE_2->addItems(types);
    m_ui->LST_PORT_TYPE_3->addItems(types);
    m_ui->LST_PORT_TYPE_4->addItems(types);

    m_ui->LST_PORT_TYPE_1->setCurrentIndex(0);
    m_ui->LST_PORT_TYPE_2->setCurrentIndex(1);
    m_ui->LST_PORT_TYPE_3->setCurrentIndex(3);
    m_ui->LST_PORT_TYPE_4->setCurrentIndex(4);
#endif

    m_ui->LST_PORT_TYPE_1->addItem("TCU");
    m_ui->LST_PORT_TYPE_2->addItem("Lens");
    m_ui->LST_PORT_TYPE_3->addItems(QStringList() << "Lens" << "RF");
    m_ui->LST_PORT_TYPE_3->setCurrentIndex(0);
    m_ui->LST_PORT_TYPE_4->addItems(QStringList() << "PTZ" << "Webcam");
    m_ui->LST_PORT_TYPE_4->setCurrentIndex(0);
    m_ui->LST_PORT_TYPE_1->installEventFilter(this);
    m_ui->LST_PORT_TYPE_2->installEventFilter(this);
    m_ui->LST_PORT_TYPE_3->installEventFilter(this);
    m_ui->LST_PORT_TYPE_4->installEventFilter(this);
}

SerialServer::~SerialServer()
{
    delete m_ui;
}

bool SerialServer::eventFilter(QObject *obj, QEvent *event)
{
    if (qobject_cast<QComboBox*>(obj) && event->type() == QEvent::MouseMove) return true;
    return QDialog::eventFilter(obj, event);
}
