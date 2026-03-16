#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QThread>
#include <QSerialPort>
#include <QTcpSocket>
#include <QLabel>
#include <QLineEdit>
#include <QButtonGroup>
#include <QMessageBox>
#include <atomic>

#include "port/tcu.h"
#include "port/lens.h"
#include "port/laser.h"
#include "port/ptz.h"
#include "port/rangefinder.h"
#include "port/usbcan.h"
#include "port/udpptz.h"
#include "port/iptzcontroller.h"
#include "util/config.h"

QT_BEGIN_NAMESPACE
namespace Ui { class UserPanel; }
QT_END_NAMESPACE

class Preferences;

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceManager(Config *config, QObject *parent = nullptr);
    ~DeviceManager();

    // Initialize port objects, threads, and signal connections
    void init(Ui::UserPanel *ui, Preferences *pref);

    // Accessors for other controllers
    TCU*            tcu()               { return p_tcu; }
    Lens*           lens()              { return p_lens; }
    Laser*          laser()             { return p_laser; }
    PTZ*            ptz()               { return p_ptz; }
    RangeFinder*    rf()                { return p_rf; }
    USBCAN*         usbcan()            { return p_usbcan; }
    UDPPTZ*         udpptz()            { return p_udpptz; }
    IPTZController* active_ptz_ctrl()   { return active_ptz; }

    QSerialPort*    serial(int idx)     { return serial_port[idx]; }
    bool            serial_connected(int idx) const { return serial_port_connected[idx]; }
    QLineEdit*      com_edit_widget(int idx) { return com_edit[idx]; }
    QLabel*         com_label_widget(int idx) { return com_label[idx]; }
    QTcpSocket*     tcp(int idx) { return tcp_port[idx]; }

    // Port management
    void connect_port_edit(QLineEdit *edit, ControlPort *port, QString &config_port);
    void setup_serial_port(QSerialPort **port, int id, QString port_num, int baud_rate);

    // Port status
    void update_port_status(ControlPort *port, QLabel *lbl);
    void update_port_status(int connected_status);
    void update_ptz_status();

    // PTZ (non-slot)
    void set_ptz_angle();
    void send_ptz_angle(float h, float v);
    void send_ptz_angle_h(float h);
    void send_ptz_angle_v(float v);
    void point_ptz_to_target(QPoint target);
    float get_angle_h() const { return angle_h.load(); }
    float get_angle_v() const { return angle_v.load(); }
    void set_angle_h(float h) { angle_h.store(h); }
    void set_angle_v(float v) { angle_v.store(v); }

public slots:
    // PTZ slots (connected via SIGNAL/SLOT)
    void update_ptz_angle(float _h, float _v);
    void update_ptz_params(qint32 ptz_param, double val);
    void ptz_button_pressed(int id);
    void ptz_button_released(int id);

    // VID camera control slots
    void vid_camera_pressed(int operation);
    void vid_camera_released();
    void vid_defog_toggled(bool checked);
    void vid_ir_power_toggled(bool checked);
    void vid_ir_auto_focus_clicked();
    void vid_ldm_toggled(bool checked);
    void vid_video_source_toggled(bool checked);
    void vid_osd_toggled(bool checked);

    // Port configuration
    void set_baudrate(int idx, int baudrate);
    void set_tcp_status_on_port(int idx, bool use_tcp);
    void set_tcu_as_shared_port(bool share);
    void com_write_data(int com_idx, QByteArray data);
    void display_port_info(int idx);

    // TCP server
    void connect_to_serial_server_tcp();
    void disconnect_from_serial_server_tcp();

signals:
    // Forwarded to port objects
    void send_data(QByteArray data, uint read_size = 0, uint read_timeout = 100);
    void send_double_tcu_msg(qint32 tcu_param, double val);
    void send_uint_tcu_msg(qint32 tcu_param, uint val);
    void send_lens_msg(qint32 lens_param, uint val = 0);
    void set_lens_pos(qint32 lens_param, uint val);
    void send_laser_msg(QString msg);
    void send_ptz_msg(qint32 ptz_param, double val = 0);

    // Forwarded from port objects
    void tcu_param_updated(qint32 tcu_param);
    void lens_param_updated(qint32 lens_param, uint val);
    void distance_updated(double distance);
    void ptz_angle_changed(float h, float v);
    void port_io_log(QString str);

    // PTZ speed changed (for UI sync)
    void ptz_speed_changed(int speed);

private:
    Config*         m_config;
    Ui::UserPanel*  m_ui;
    Preferences*    m_pref;

    // Port objects
    TCU*            p_tcu;
    Lens*           p_lens;
    Laser*          p_laser;
    PTZ*            p_ptz;
    RangeFinder*    p_rf;
    USBCAN*         p_usbcan;
    UDPPTZ*         p_udpptz;
    IPTZController* active_ptz;

    // Worker threads
    QThread*        th_tcu;
    QThread*        th_lens;
    QThread*        th_laser;
    QThread*        th_ptz;
    QThread*        th_rf;
    QThread*        th_usbcan;
    QThread*        th_udpptz;

    // Serial/TCP ports (legacy, partially used)
    QSerialPort*    serial_port[5];
    bool            serial_port_connected[5];
    QTcpSocket*     tcp_port[5];
    bool            use_tcp[5];
    bool            share_serial_port;

    // COM UI widgets
    QLabel*         com_label[5];
    QLineEdit*      com_edit[5];

    // PTZ UI groups and state
    QButtonGroup*   ptz_grp;
    QButtonGroup*   vid_camera_grp;
    int             ptz_speed;
    std::atomic<float> angle_h{0.0f};
    std::atomic<float> angle_v{0.0f};
};

#endif // DEVICEMANAGER_H
