#include "controller/devicemanager.h"
#include "visual/preferences.h"
#include "ui_user_panel.h"
#include "ui_preferences.h"
#include "util/util.h"

#include <cmath>

DeviceManager::DeviceManager(Config *config, QObject *parent)
    : QObject(parent),
      m_config(config),
      m_ui(nullptr),
      m_pref(nullptr),
      p_tcu(nullptr),
      p_lens(nullptr),
      p_laser(nullptr),
      p_ptz(nullptr),
      p_rf(nullptr),
      p_usbcan(nullptr),
      p_udpptz(nullptr),
      active_ptz(nullptr),
      th_tcu(nullptr),
      th_lens(nullptr),
      th_laser(nullptr),
      th_ptz(nullptr),
      th_rf(nullptr),
      th_usbcan(nullptr),
      th_udpptz(nullptr),
      serial_port{nullptr},
      serial_port_connected{false},
      tcp_port{nullptr},
      use_tcp{false},
      share_serial_port(false),
      com_label{nullptr},
      com_edit{nullptr},
      ptz_grp(nullptr),
      vid_camera_grp(nullptr),
      ptz_speed(32),
      angle_h(0),
      angle_v(0)
{
}

DeviceManager::~DeviceManager()
{
    th_tcu->quit();
    th_lens->quit();
    th_laser->quit();
    th_ptz->quit();
    th_rf->quit();

    // Wait for I/O threads to finish - 100ms is sufficient for serial/TCP operations
    th_tcu->wait(100);
    th_lens->wait(100);
    th_laser->wait(100);
    th_ptz->wait(100);
    th_rf->wait(100);

    th_tcu->deleteLater();
    th_lens->deleteLater();
    th_laser->deleteLater();
    th_ptz->deleteLater();
    th_rf->deleteLater();
    p_tcu->deleteLater();
    p_lens->deleteLater();
    p_laser->deleteLater();
    p_ptz->deleteLater();
    p_rf->deleteLater();

    th_usbcan->quit();
    th_usbcan->wait(100);
    th_usbcan->deleteLater();
    p_usbcan->deleteLater();

    th_udpptz->quit();
    th_udpptz->wait(100);
    th_udpptz->deleteLater();
    p_udpptz->deleteLater();
}

void DeviceManager::init(Ui::UserPanel *ui, Preferences *pref)
{
    m_ui = ui;
    m_pref = pref;

    // COM label/edit arrays
    com_label[0] = ui->TCU_COM;
    com_label[1] = ui->LENS_COM;
    com_label[2] = ui->LASER_COM;
    com_label[3] = ui->PTZ_COM;
    com_label[4] = ui->RANGE_COM;

    com_edit[0] = ui->TCU_COM_EDIT;
    com_edit[1] = ui->LENS_COM_EDIT;
    com_edit[2] = ui->LASER_COM_EDIT;
    com_edit[3] = ui->PTZ_COM_EDIT;
    com_edit[4] = ui->RANGE_COM_EDIT;

    // Serial/TCP port objects
    for (int i = 0; i < 5; i++) serial_port[i] = new QSerialPort(parent());
    for (int i = 0; i < 5; i++) tcp_port[i] = new QTcpSocket(parent());

    // PTZ button group
    ptz_grp = new QButtonGroup(parent());
    ui->UP_LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up_left"));
    ptz_grp->addButton(ui->UP_LEFT_BTN, 0);
    ui->UP_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up"));
    ptz_grp->addButton(ui->UP_BTN, 1);
    ui->UP_RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/up_right"));
    ptz_grp->addButton(ui->UP_RIGHT_BTN, 2);
    ui->LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/left"));
    ptz_grp->addButton(ui->LEFT_BTN, 3);
    ui->SELF_TEST_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/self_test"));
    ptz_grp->addButton(ui->SELF_TEST_BTN, 4);
    ui->RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/right"));
    ptz_grp->addButton(ui->RIGHT_BTN, 5);
    ui->DOWN_LEFT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down_left"));
    ptz_grp->addButton(ui->DOWN_LEFT_BTN, 6);
    ui->DOWN_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down"));
    ptz_grp->addButton(ui->DOWN_BTN, 7);
    ui->DOWN_RIGHT_BTN->setIcon(QIcon(":/directions/" + QString(app_theme ? "light" : "dark") + "/down_right"));
    ptz_grp->addButton(ui->DOWN_RIGHT_BTN, 8);
    connect(ptz_grp, SIGNAL(buttonPressed(int)), this, SLOT(ptz_button_pressed(int)));
    connect(ptz_grp, SIGNAL(buttonReleased(int)), this, SLOT(ptz_button_released(int)));

    // VID_PAGE camera control buttons
    vid_camera_grp = new QButtonGroup(parent());
    vid_camera_grp->addButton(ui->VL_ZOOM_OUT_BTN, UDPPTZ::VL_ZOOM_OUT);
    vid_camera_grp->addButton(ui->VL_ZOOM_IN_BTN, UDPPTZ::VL_ZOOM_IN);
    vid_camera_grp->addButton(ui->VL_FOCUS_FAR_BTN, UDPPTZ::VL_FOCUS_FAR);
    vid_camera_grp->addButton(ui->VL_FOCUS_NEAR_BTN, UDPPTZ::VL_FOCUS_NEAR);
    vid_camera_grp->addButton(ui->IR_ZOOM_OUT_BTN, UDPPTZ::IR_ZOOM_OUT);
    vid_camera_grp->addButton(ui->IR_ZOOM_IN_BTN, UDPPTZ::IR_ZOOM_IN);
    vid_camera_grp->addButton(ui->IR_FOCUS_FAR_BTN, UDPPTZ::IR_FOCUS_FAR);
    vid_camera_grp->addButton(ui->IR_FOCUS_NEAR_BTN, UDPPTZ::IR_FOCUS_NEAR);
    connect(vid_camera_grp, SIGNAL(buttonPressed(int)), this, SLOT(vid_camera_pressed(int)));
    connect(vid_camera_grp, SIGNAL(buttonReleased(int)), this, SLOT(vid_camera_released()));

    // VID_PAGE toggle buttons
    connect(ui->VL_DEFOG_BTN, &QPushButton::toggled, this, &DeviceManager::vid_defog_toggled);
    connect(ui->IR_POWER_BTN, &QPushButton::toggled, this, &DeviceManager::vid_ir_power_toggled);
    connect(ui->IR_AUTO_FOCUS_BTN, &QPushButton::clicked, this, &DeviceManager::vid_ir_auto_focus_clicked);
    connect(ui->LDM_BTN, &QPushButton::toggled, this, &DeviceManager::vid_ldm_toggled);
    connect(ui->VIDEO_SOURCE_BTN, &QPushButton::toggled, this, &DeviceManager::vid_video_source_toggled);
    connect(ui->OSD_BTN, &QPushButton::toggled, this, &DeviceManager::vid_osd_toggled);

    // PTZ speed slider
    ui->PTZ_SPEED_SLIDER->setMinimum(1);
    ui->PTZ_SPEED_SLIDER->setMaximum(64);
    ui->PTZ_SPEED_SLIDER->setSingleStep(1);
    ui->PTZ_SPEED_SLIDER->setValue(32);
    ui->PTZ_SPEED_EDIT->setText("32");
    connect(ui->PTZ_SPEED_SLIDER, &QSlider::valueChanged, this, [this](int value) {
        emit send_ptz_msg(PTZ::SPEED, ptz_speed = value);
        m_ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed));
    });
    connect(ui->PTZ_SPEED_EDIT, &QLineEdit::editingFinished, this, [this]() {
        ptz_speed = m_ui->PTZ_SPEED_EDIT->text().toInt();
        if (ptz_speed > 64) ptz_speed = 64;
        if (ptz_speed < 1)  ptz_speed = 1;
        emit send_ptz_msg(PTZ::SPEED, ptz_speed);
        m_ui->PTZ_SPEED_SLIDER->setValue(ptz_speed);
        m_ui->PTZ_SPEED_EDIT->setText(QString::number(ptz_speed));
    });
    connect(ui->GET_ANGLE_BTN, &QPushButton::clicked, this, [this]() {
        emit send_ptz_msg(PTZ::ANGLE_H);
        emit send_ptz_msg(PTZ::ANGLE_V);
    });
    connect(ui->STOP_BTN, &QPushButton::clicked, this, [this]() {
        if (m_config->get_data().device.ptz_type == 0)
            emit send_ptz_msg(PTZ::STOP);
        else if (active_ptz)
            active_ptz->ptz_stop();
    });

    // Create port objects and threads
    p_tcu   = new TCU(0);
    p_lens  = new Lens(1);
    p_laser = new Laser(2);
    p_ptz   = new PTZ(3);
    p_rf    = new RangeFinder(4);
    th_tcu   = new QThread(parent());
    th_lens  = new QThread(parent());
    th_laser = new QThread(parent());
    th_ptz   = new QThread(parent());
    th_rf    = new QThread(parent());
    p_tcu  ->moveToThread(th_tcu);
    p_lens ->moveToThread(th_lens);
    p_laser->moveToThread(th_laser);
    p_ptz  ->moveToThread(th_ptz);
    p_rf   ->moveToThread(th_rf);
    th_tcu  ->start();
    th_lens ->start();
    th_laser->start();
    th_ptz  ->start();
    th_rf   ->start();

    // TCU connections
    connect(ui->TCU_COM_EDIT, &QLineEdit::returnPressed, this, [this]() {
        connect_port_edit(m_ui->TCU_COM_EDIT, p_tcu, m_config->get_data().com_tcu.port);
    });
    connect(p_tcu, &ControlPort::port_status_updated, this, [this]() { update_port_status(p_tcu, m_ui->TCU_COM); });
    connect(p_tcu, &ControlPort::port_io_log, this, &DeviceManager::port_io_log, Qt::QueuedConnection);
    connect(p_tcu, &TCU::tcu_param_updated, this, &DeviceManager::tcu_param_updated);
    connect(this, SIGNAL(send_double_tcu_msg(qint32, double)), p_tcu, SLOT(set_user_param(qint32, double)), Qt::QueuedConnection);
    connect(this, SIGNAL(send_uint_tcu_msg(qint32, uint)), p_tcu, SLOT(set_user_param(qint32, uint)), Qt::QueuedConnection);

    // Lens connections
    connect(ui->LENS_COM_EDIT, &QLineEdit::returnPressed, this, [this]() {
        connect_port_edit(m_ui->LENS_COM_EDIT, p_lens, m_config->get_data().com_lens.port);
    });
    connect(p_lens, &ControlPort::port_status_updated, this, [this]() { update_port_status(p_lens, m_ui->LENS_COM); });
    connect(p_lens, &ControlPort::port_io_log, this, &DeviceManager::port_io_log, Qt::QueuedConnection);
    connect(p_lens, &Lens::lens_param_updated, this, &DeviceManager::lens_param_updated);
    connect(this, SIGNAL(send_lens_msg(qint32, uint)), p_lens, SLOT(lens_control(qint32, uint)), Qt::QueuedConnection);
    connect(this, SIGNAL(set_lens_pos(qint32, uint)), p_lens, SLOT(set_pos_temp(qint32, uint)), Qt::QueuedConnection);

    // Laser connections
    connect(ui->LASER_COM_EDIT, &QLineEdit::returnPressed, this, [this]() {
        connect_port_edit(m_ui->LASER_COM_EDIT, p_laser, m_config->get_data().com_laser.port);
    });
    connect(p_laser, &ControlPort::port_status_updated, this, [this]() { update_port_status(p_laser, m_ui->LASER_COM); });
    connect(this, SIGNAL(send_laser_msg(QString)), p_laser, SLOT(laser_control(QString)), Qt::QueuedConnection);

    // PTZ connections
    connect(ui->PTZ_COM_EDIT, &QLineEdit::returnPressed, this, [this]() {
        switch (m_config->get_data().device.ptz_type) {
        case 0:
            if (m_ui->PTZ_COM_EDIT->text().isEmpty()) p_ptz->emit disconnect_from(true);
            else p_ptz->emit connect_to_serial(m_ui->PTZ_COM_EDIT->text());
            break;
        case 1:
            if (m_ui->PTZ_COM_EDIT->text().isEmpty()) p_usbcan->emit disconnect_from();
            else p_usbcan->emit connect_to();
            break;
        case 2:
            if (m_ui->PTZ_COM_EDIT->text().isEmpty()) p_udpptz->emit disconnect_from();
            else p_udpptz->emit connect_to();
            break;
        }
        QString port_text = m_ui->PTZ_COM_EDIT->text();
#ifdef WIN32
        m_config->get_data().com_ptz.port = port_text.isEmpty() ? "" : "COM" + port_text;
#else
        m_config->get_data().com_ptz.port = port_text;
#endif
        m_config->auto_save();
    });
    connect(p_ptz, &ControlPort::port_status_updated, this, [this]() {
        update_port_status(p_ptz, m_ui->PTZ_COM);
        bool connected = p_ptz->get_port_status() & (ControlPort::SERIAL_CONNECTED | ControlPort::TCP_CONNECTED);
        if (connected) active_ptz = p_ptz;
        m_pref->set_ptz_type_enabled(!connected);
    });
    connect(p_ptz, &ControlPort::port_io_log, this, &DeviceManager::port_io_log, Qt::QueuedConnection);
    connect(p_ptz, &PTZ::ptz_param_updated, this, &DeviceManager::update_ptz_params);
    connect(p_ptz, &PTZ::angle_updated, this, &DeviceManager::update_ptz_angle);
    connect(this, SIGNAL(send_ptz_msg(qint32, double)), p_ptz, SLOT(ptz_control(qint32, double)), Qt::QueuedConnection);

    // RangeFinder connections
    connect(p_rf, &RangeFinder::distance_updated, this, &DeviceManager::distance_updated);

    // USBCAN
    p_usbcan = new USBCAN();
    th_usbcan = new QThread(parent());
    p_usbcan->moveToThread(th_usbcan);
    th_usbcan->start();
    connect(p_usbcan, &USBCAN::angle_updated, this, &DeviceManager::update_ptz_angle);
    connect(p_usbcan, &USBCAN::connection_status_changed, this, [this](bool connected) {
        if (connected) active_ptz = p_usbcan;
        m_pref->set_ptz_type_enabled(!connected);
    });
    connect(p_usbcan, &USBCAN::connection_status_changed, this, &DeviceManager::update_ptz_status);
    connect(p_usbcan, &USBCAN::connect_to, this, [this]() { m_pref->set_ptz_type_enabled(false); });
    connect(p_usbcan, &USBCAN::disconnect_from, this, [this]() { m_pref->set_ptz_type_enabled(true); });

    // UDPPTZ
    p_udpptz = new UDPPTZ();
    th_udpptz = new QThread(parent());
    p_udpptz->moveToThread(th_udpptz);
    th_udpptz->start();
    connect(p_udpptz, &UDPPTZ::angle_updated, this, &DeviceManager::update_ptz_angle);
    connect(p_udpptz, &UDPPTZ::connection_status_changed, this, [this](bool connected) {
        if (connected) active_ptz = p_udpptz;
        m_pref->set_ptz_type_enabled(!connected);
    });
    connect(p_udpptz, &UDPPTZ::connection_status_changed, this, &DeviceManager::update_ptz_status);
    connect(p_udpptz, &UDPPTZ::connect_to, this, [this]() { m_pref->set_ptz_type_enabled(false); });
    connect(p_udpptz, &UDPPTZ::disconnect_from, this, [this]() { m_pref->set_ptz_type_enabled(true); });

    active_ptz = nullptr;

    // Initialize PTZ status display
    update_ptz_status();
}

void DeviceManager::connect_port_edit(QLineEdit *edit, ControlPort *port, QString &config_port)
{
    port->emit connect_to_serial(edit->text());
    QString port_text = edit->text();
#ifdef WIN32
    config_port = port_text.isEmpty() ? "" : "COM" + port_text;
#else
    config_port = port_text;
#endif
    m_config->auto_save();
}

void DeviceManager::setup_serial_port(QSerialPort **port, int id, QString port_num, int baud_rate)
{
    (*port)->setPortName("COM" + port_num);

    if ((*port)->open(QIODevice::ReadWrite)) {
        QThread::msleep(10);
        (*port)->clear();
        qDebug("COM%s connected\n", qPrintable(port_num));
        serial_port_connected[id] = true;
        com_label[id]->setStyleSheet(app_theme ? "color: #180D1C;" : "color: #B0C4DE;");

        (*port)->setBaudRate(baud_rate);
        (*port)->setDataBits(QSerialPort::Data8);
        (*port)->setParity(QSerialPort::NoParity);
        (*port)->setStopBits(QSerialPort::OneStop);
        (*port)->setFlowControl(QSerialPort::NoFlowControl);

        // send initial data
        switch (id) {
        case 0:
            emit send_double_tcu_msg(TCU::LASER_USR, p_tcu->get(TCU::LASER_WIDTH));
            // NOTE: update_delay() and update_gate_width() are in TCUController
            // They will be called via signal after TCUController is extracted
            break;
        case 2:
            m_ui->GET_LENS_PARAM_BTN->click();
            // NOTE: change_focus_speed() is in LensController
            break;
        default:
            break;
        }
        m_pref->display_baudrate(id, baud_rate);
    }
    else {
        serial_port_connected[id] = false;
        com_label[id]->setStyleSheet("color: #CD5C5C;");
    }
}

void DeviceManager::update_port_status(ControlPort *port, QLabel *lbl)
{
    qint32 port_status = port->get_port_status();
    QString style = QString(port_status & ControlPort::SERIAL_CONNECTED ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;") +
                    QString(port_status & ControlPort::TCP_CONNECTED ? "text-decoration:underline;" : "");
    lbl->setStyleSheet(style);
}

void DeviceManager::update_port_status(int connected_status)
{
    qDebug() << connected_status;
}

void DeviceManager::update_ptz_params(qint32 ptz_param, double val)
{
    Q_UNUSED(val);
    switch (ptz_param)
    {
        case PTZ::NO_PARAM:
            update_ptz_angle((float)p_ptz->get(PTZ::ANGLE_H), (float)p_ptz->get(PTZ::ANGLE_V));
            m_ui->PTZ_SPEED_SLIDER->setValue(std::round(p_ptz->get(PTZ::SPEED)));
            break;
        default:break;
    }
}

void DeviceManager::update_ptz_angle(float _h, float _v)
{
    angle_h.store(_h);
    angle_v.store(_v);
    float display_h = _h < 0 ? _h + 360.0f : _h;
    if (!m_ui->ANGLE_H_EDIT->hasFocus()) m_ui->ANGLE_H_EDIT->setText(QString::asprintf("%06.2f", display_h));
    if (!m_ui->ANGLE_V_EDIT->hasFocus()) m_ui->ANGLE_V_EDIT->setText(QString::asprintf("%05.2f", _v));
    emit ptz_angle_changed(_h, _v);
}

void DeviceManager::update_ptz_status()
{
    QString style;
    bool connected = false;
    bool is_network = false;

    switch (m_config->get_data().device.ptz_type) {
    case 0: // pelco-p - use existing ControlPort status
        return; // ControlPort handles this automatically
    case 1: // usbcan - treat as serial connection when connected
        connected = p_usbcan->is_connected();
        is_network = false;
        break;
    case 2: // udp-scw - treat as network connection when connected
        connected = p_udpptz->is_connected();
        is_network = true;
        break;
    }

    // Apply the same styling as other ControlPort labels
    style = QString(connected && !is_network ? (app_theme ? "color: #180D1C;" : "color: #B0C4DE;") : "color: #CD5C5C;") +
            QString(connected && is_network ? "text-decoration:underline;" : "");

    m_ui->PTZ_COM->setStyleSheet(style);
}

void DeviceManager::ptz_button_pressed(int id)
{
    int ptz_type = m_config->get_data().device.ptz_type;

    if (id == 4) {
        if (QMessageBox::warning(nullptr, "PTZ", QObject::tr("Initialize?"), QMessageBox::StandardButton::Ok | QMessageBox::StandardButton::Cancel) != QMessageBox::StandardButton::Ok) return;
        // Self-check sequences are type-specific
        switch (ptz_type) {
        case 0: emit send_ptz_msg(PTZ::SELF_CHECK); break;
        case 1:
            p_usbcan->emit transmit(USBCAN::OFF);
            QThread::msleep(1000);
            p_usbcan->emit transmit(USBCAN::SELF_CHECK);
            QThread::msleep(1000);
            p_usbcan->emit transmit(USBCAN::LOCK);
            QThread::msleep(1000);
            break;
        case 2: p_udpptz->emit transmit_data(UDPPTZ::RETURN_ZERO); break;
        }
        return;
    }

    int speed = m_ui->PTZ_SPEED_SLIDER->value();
    if (ptz_type == 0) {
        emit send_ptz_msg(PTZ::SPEED, speed);
        emit send_ptz_msg(id + 1); // direction enum offset by 1 for Pelco-D
    } else if (active_ptz) {
        active_ptz->ptz_move(id, speed);
    }
}

void DeviceManager::ptz_button_released(int id)
{
    if (id == 4) return;
    if (m_config->get_data().device.ptz_type == 0)
        emit send_ptz_msg(PTZ::STOP);
    else if (active_ptz)
        active_ptz->ptz_stop();
}

void DeviceManager::set_ptz_angle()
{
    if (m_config->get_data().device.ptz_type == 0) {
        emit send_ptz_msg(PTZ::SET_H, angle_h.load());
        emit send_ptz_msg(PTZ::SET_V, angle_v.load());
    } else if (active_ptz) {
        active_ptz->ptz_set_angle(angle_h.load(), angle_v.load());
    }
}

void DeviceManager::send_ptz_angle(float h, float v)
{
    if (m_config->get_data().device.ptz_type == 0) {
        emit send_ptz_msg(PTZ::SET_H, h);
        emit send_ptz_msg(PTZ::SET_V, v);
    } else if (active_ptz) {
        active_ptz->ptz_set_angle(h, v);
    }
}

void DeviceManager::send_ptz_angle_h(float h)
{
    if (m_config->get_data().device.ptz_type == 0)
        emit send_ptz_msg(PTZ::SET_H, h);
    else if (active_ptz)
        active_ptz->ptz_set_angle_h(h);
}

void DeviceManager::send_ptz_angle_v(float v)
{
    if (m_config->get_data().device.ptz_type == 0)
        emit send_ptz_msg(PTZ::SET_V, v);
    else if (active_ptz)
        active_ptz->ptz_set_angle_v(v);
}

void DeviceManager::point_ptz_to_target(QPoint target)
{
    static int display_width, display_height;
    // TODO config params for max zoom
    static float tot_h = 0.82, tot_v = 0.57;// small FOV
    display_width = m_ui->SOURCE_DISPLAY->width();
    display_height = m_ui->SOURCE_DISPLAY->height();
    angle_h.store(angle_h.load() + target.x() * tot_h / display_width - tot_h / 2);
    angle_v.store(angle_v.load() + target.y() * tot_v / display_height - tot_v / 2);

    // Ensure horizontal angle is always positive (0 to 360)
    angle_h.store(fmod(angle_h.load() + 360.0, 360.0));

    set_ptz_angle();
}

void DeviceManager::vid_camera_pressed(int operation)
{
    if (!p_udpptz || !p_udpptz->is_connected()) return;

    // Change button text to "x" when pressed
    QPushButton* btn = qobject_cast<QPushButton*>(vid_camera_grp->button(operation));
    if (btn) {
        btn->setText("x");
    }

    p_udpptz->emit transmit_data(operation);
}

void DeviceManager::vid_camera_released()
{
    if (!p_udpptz || !p_udpptz->is_connected()) return;

    // Restore button text when released
    m_ui->VL_ZOOM_OUT_BTN->setText("-");
    m_ui->VL_ZOOM_IN_BTN->setText("+");
    m_ui->VL_FOCUS_FAR_BTN->setText("-");
    m_ui->VL_FOCUS_NEAR_BTN->setText("+");
    m_ui->IR_ZOOM_OUT_BTN->setText("-");
    m_ui->IR_ZOOM_IN_BTN->setText("+");
    m_ui->IR_FOCUS_FAR_BTN->setText("-");
    m_ui->IR_FOCUS_NEAR_BTN->setText("+");

    p_udpptz->emit transmit_data(UDPPTZ::STOP);
}

void DeviceManager::vid_defog_toggled(bool checked)
{
    if (!p_udpptz || !p_udpptz->is_connected()) return;
    p_udpptz->emit transmit_data(checked ? UDPPTZ::VL_DEFOG_ON : UDPPTZ::VL_DEFOG_OFF);
    m_ui->VL_DEFOG_BTN->setStyleSheet(checked ? "background-color: #5B8C5B;" : "");
}

void DeviceManager::vid_ir_power_toggled(bool checked)
{
    if (!p_udpptz || !p_udpptz->is_connected()) return;

    // Confirm before powering off IR
    if (!checked) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            qobject_cast<QWidget*>(parent()), "Confirm", "Power off IR?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            m_ui->IR_POWER_BTN->blockSignals(true);
            m_ui->IR_POWER_BTN->setChecked(true);
            m_ui->IR_POWER_BTN->blockSignals(false);
            return;
        }
    }

    p_udpptz->emit transmit_data(checked ? UDPPTZ::IR_POWER_ON : UDPPTZ::IR_POWER_OFF);
    m_ui->IR_POWER_BTN->setStyleSheet(checked ? "background-color: #5B8C5B;" : "");
}

void DeviceManager::vid_ir_auto_focus_clicked()
{
    if (!p_udpptz || !p_udpptz->is_connected()) return;
    p_udpptz->emit transmit_data(UDPPTZ::IR_AUTO_FOCUS);
}

void DeviceManager::vid_ldm_toggled(bool checked)
{
    if (!p_udpptz || !p_udpptz->is_connected()) return;
    p_udpptz->emit transmit_data(checked ? UDPPTZ::LASER_ON : UDPPTZ::LASER_OFF);
    m_ui->LDM_BTN->setStyleSheet(checked ? "background-color: #5B8C5B;" : "");
}

void DeviceManager::vid_video_source_toggled(bool checked)
{
    if (!p_udpptz || !p_udpptz->is_connected()) return;
    p_udpptz->emit transmit_data(checked ? UDPPTZ::SWITCH_IR : UDPPTZ::SWITCH_VL);
    m_ui->VIDEO_SOURCE_BTN->setText(checked ? "Switch to VL" : "Switch to IR");
}

void DeviceManager::vid_osd_toggled(bool checked)
{
    if (!p_udpptz || !p_udpptz->is_connected()) return;
    p_udpptz->emit transmit_data(checked ? UDPPTZ::OSD_FULL : UDPPTZ::OSD_HIDE);
    m_ui->OSD_BTN->setStyleSheet(checked ? "background-color: #5B8C5B;" : "");
}

void DeviceManager::set_baudrate(int idx, int baudrate)
{
    switch (idx) {
        case 0: p_tcu->set_baudrate(baudrate); break;
        case 1: break;
        case 2: p_lens->set_baudrate(baudrate); break;
        case 3: p_laser->set_baudrate(baudrate); break;
        case 4: p_ptz->set_baudrate(baudrate); break;
        default: break;
    }
}

void DeviceManager::set_tcp_status_on_port(int idx, bool use_tcp)
{
    switch (idx) {
        case 0: p_tcu->set_use_tcp(use_tcp); break;
        case 1: break;
        case 2: p_lens->set_use_tcp(use_tcp); break;
        case 3: p_laser->set_use_tcp(use_tcp); break;
        case 4: p_ptz->set_use_tcp(use_tcp); break;
        default: break;
    }
}

void DeviceManager::set_tcu_as_shared_port(bool share)
{
    p_lens->share_port_from(share ? p_tcu : nullptr);
    p_ptz->share_port_from(share ? p_tcu : nullptr);
}

void DeviceManager::com_write_data(int com_idx, QByteArray data)
{
    ControlPort *temp = nullptr;
    switch (com_idx)
    {
        case 0: temp = p_tcu; break;
        case 1: break;
        case 2: temp = p_lens; break;
        case 3: temp = p_laser; break;
        case 4: temp = p_ptz; break;
        default: break;
    }
    if (temp) temp->emit send_data(data, 0, 100);
}

void DeviceManager::display_port_info(int idx)
{
    ControlPort *cp = nullptr;
    switch (idx) {
    case 0: cp = p_tcu; break;
    case 1: m_pref->display_baudrate(idx, 0); return;
    case 2: cp = p_lens; break;
    case 3: cp = p_laser; break;
    case 4: cp = p_ptz; break;
    default: break;
    }
    if (cp) {
        m_pref->display_baudrate(idx, cp->get_baudrate());
        m_pref->ui->TCP_SERVER_CHK->setChecked(cp->get_use_tcp());
    }
}

void DeviceManager::connect_to_serial_server_tcp()
{
    p_tcu->emit connect_to_tcp(m_pref->ui->TCP_SERVER_IP_EDIT->text(), 10001);
    QThread::msleep(200);
    p_lens->emit connect_to_tcp(m_pref->ui->TCP_SERVER_IP_EDIT->text(), 10002);
    QThread::msleep(200);
    p_rf->emit connect_to_tcp(m_pref->ui->TCP_SERVER_IP_EDIT->text(), 10003);

    QTimer::singleShot(1000,
    [this]()
    {
        if (ControlPort::TCP_CONNECTED & p_tcu->get_port_status())
        {
            m_pref->ui->TCP_SERVER_CHK->setEnabled(true);
            m_pref->ui->TCP_SERVER_CHK->setChecked(true);
        }
        else QMessageBox::information(qobject_cast<QWidget*>(parent()), "serial port server", "cannot connect to server");
    });
}

void DeviceManager::disconnect_from_serial_server_tcp()
{
    p_tcu-> emit disconnect_from(true);
    p_lens->emit disconnect_from(true);
    m_pref->ui->TCP_SERVER_CHK->setEnabled(false);
    m_pref->ui->TCP_SERVER_CHK->setChecked(false);
}
