#include "usbcan.h"

inline void print_can_frame(CAN_OBJ frame, bool transmit)
{
    qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << (transmit ? "transmit" : "receive");
    qDebug() << "frame ID:" << hex << frame.ID;
    qDebug() << "length:  " << frame.DataLen;
    qDebug() << "content: " << QByteArray((char*)frame.Data, frame.DataLen).toHex(' ');
    qDebug() << "";
}

USBCAN::USBCAN():
    QObject{nullptr},
    m_device_type(USBCAN2),
    m_device_index(0),
    m_can_index(CAN1),
    m_timer_r5(nullptr),
    m_timer_t20(nullptr),
    m_timer_t200(nullptr),
    m_timer_u100(nullptr),
    m_power(0x00),
    m_zero_step(0),
    m_pos_step(0),
    m_pit_step(0),
    m_position(0),
    m_pitch(0)
{
    connect(this, &USBCAN::connect_to, this, &USBCAN::connect_to_device);
    connect(this, &USBCAN::disconnect_from, this, &USBCAN::disconnect_from_device);
    connect(this, &USBCAN::transmit, this, &USBCAN::transmit_data);
    connect(this, &USBCAN::control, this, &USBCAN::device_control);

    m_timer_r5 = new QTimer(this);
    m_timer_r5->setTimerType(Qt::PreciseTimer);
    m_timer_r5->setInterval(5);
    connect(m_timer_r5, &QTimer::timeout, this, &USBCAN::handle_received);

    m_timer_t20 = new QTimer(this);
    m_timer_t20->setTimerType(Qt::PreciseTimer);
    m_timer_t20->setInterval(20);
    connect(m_timer_t20, &QTimer::timeout, this, &USBCAN::transmit_ptz_status);

    m_timer_t200 = new QTimer(this);
    m_timer_t200->setTimerType(Qt::PreciseTimer);
    m_timer_t200->setInterval(200);
    connect(m_timer_t200, &QTimer::timeout, this, &USBCAN::transmit_power_status);

    m_timer_u100 = new QTimer(this);
    m_timer_u100->setTimerType(Qt::PreciseTimer);
    m_timer_u100->setInterval(100);
    connect(m_timer_u100, &QTimer::timeout, this, [this]() { emit angle_updated(m_position, m_pitch); });
}

USBCAN::~USBCAN()
{
    m_timer_r5->stop();
    m_timer_r5->deleteLater();
}

bool USBCAN::connect_to_device()
{
    if (m_connected) return false;
    if (OpenDevice(m_device_type, m_device_index, 0) != STATUS_OK) {
        qDebug() << "Connect to device failed";
        emit connection_status_changed(false);
        return false;
    }
//    qDebug() << "GCAN connected";
    INIT_CONFIG init_config;
    memset(&init_config, 0, sizeof(init_config));
    init_config.AccCode = 0;
    init_config.AccMask = 0xFFFFFFFF;
    init_config.Filter = 1;
    init_config.Timing0 = 0x01;
    init_config.Timing1 = 0x1C;
    init_config.Mode = 0;
    if (InitCAN(m_device_type, m_device_index, m_can_index, &init_config) != STATUS_OK) {
        qDebug() << "CAN1 init failed";
        CloseDevice(m_device_type, m_device_index);
        emit connection_status_changed(false);
        return false;
    }
//    qDebug() << "CAN1 initiated";
    if (StartCAN(m_device_type, m_device_index, m_can_index) != STATUS_OK) {
        qDebug() << "CAN1 start failed";
        CloseDevice(m_device_type, m_device_index);
        emit connection_status_changed(false);
        return false;
    }
//    qDebug() << "CAN1 connected";

//    qDebug() << "open port in" << QThread::currentThread();
    m_connected = true;
    m_timer_r5->stop();
    m_timer_r5->start();
    emit connection_status_changed(true);
    qDebug() << "gcan connected";
    return true;
}

void USBCAN::disconnect_from_device()
{
    if (!m_connected) return;
    CloseDevice(m_device_type, m_device_index);
    m_connected = false;
    m_timer_r5->stop();
    emit connection_status_changed(false);
    qDebug() << "gcan disconnected";
}

void USBCAN::transmit_data(qint32 op)
{
    CAN_OBJ frame;
    ZeroMemory(&frame, sizeof(frame));
    frame.SendType = 1;
    frame.RemoteFlag = 0;
    frame.ExternFlag = 0;
    frame.TimeFlag = 0;
    frame.TimeStamp = 0;

    BYTE data[8] = {0};
    switch (op) {
    case START:
        frame.ID = 0x705;
        frame.DataLen = 1;
        data[0] = 0x00;
        break;
    case OFF:
        transmit_data(Operation::START);
        frame.ID = 0x604;
        frame.DataLen = 8;
        data[0] = 0x2F; data[1] = 0x04; data[2] = 0x20; data[3] = 0x04;
        data[4] = 0x00; data[5] = 0x00; data[6] = 0x00; data[7] = 0x00;
        m_power = 0x7F;
        m_timer_t20->start();
        m_timer_t200->start();
        break;
    case SELF_CHECK:
        transmit_data(Operation::START);
        frame.ID = 0x604;
        frame.DataLen = 8;
        data[0] = 0x2F; data[1] = 0x04; data[2] = 0x20; data[3] = 0x04;
        data[4] = 0x07; data[5] = 0x00; data[6] = 0x00; data[7] = 0x00;
        m_timer_u100->start();
        break;
    case SERVO:
        transmit_data(Operation::START);
        frame.ID = 0x604;
        frame.DataLen = 8;
        data[0] = 0x2F; data[1] = 0x04; data[2] = 0x20; data[3] = 0x04;
        data[4] = 0x01; data[5] = 0x00; data[6] = 0x00; data[7] = 0x00;
        m_power = 0x05;
        break;
    case LOCK:
        transmit_data(Operation::START);
        frame.ID = 0x604;
        frame.DataLen = 8;
        data[0] = 0x2F; data[1] = 0x04; data[2] = 0x20; data[3] = 0x04;
        data[4] = 0x03; data[5] = 0x00; data[6] = 0x00; data[7] = 0x00;
        m_power = 0x05;
        break;
    case LOCK_LOCK:
        frame.ID = 0x204;
        frame.DataLen = 1;
        data[0] = 0x03;
        break;
    case FOLLOW:
        frame.ID = 0x204;
        frame.DataLen = 1;
        data[0] = 0x04;
        break;
    case POSITION:
        transmit_data(Operation::FOLLOW);
        frame.ID = 0x604;
        frame.DataLen = 8;
        data[0] = 0x23; data[1] = 0x04; data[2] = 0x20; data[3] = 0x05;
        memcpy(data + 4, &m_position, 4);
        break;
    case PITCH:
        transmit_data(Operation::FOLLOW);
        frame.ID = 0x604;
        frame.DataLen = 8;
        data[0] = 0x23; data[1] = 0x04; data[2] = 0x20; data[3] = 0x06;
        memcpy(data + 4, &m_pitch, 4);
        break;
    default:
        return;
    }

    memcpy(frame.Data, data, frame.DataLen);

    m_read_mutex.lock();
    DWORD sent = Transmit(m_device_type, m_device_index, m_can_index, &frame, 1);
//    print_can_frame(frame, true);
    m_read_mutex.unlock();

//    qDebug() << "sent" << sent;
}

void USBCAN::ptz_move(int direction, int speed)
{
    float spd = (float)speed;
    switch (direction) {
    case DIR_UP_LEFT:    emit control(LEFT, spd); emit control(UP, spd); break;
    case DIR_UP:         emit control(UP, spd); break;
    case DIR_UP_RIGHT:   emit control(RIGHT, spd); emit control(UP, spd); break;
    case DIR_LEFT:       emit control(LEFT, spd); break;
    case DIR_RIGHT:      emit control(RIGHT, spd); break;
    case DIR_DOWN_LEFT:  emit control(LEFT, spd); emit control(DOWN, spd); break;
    case DIR_DOWN:       emit control(DOWN, spd); break;
    case DIR_DOWN_RIGHT: emit control(RIGHT, spd); emit control(DOWN, spd); break;
    default: break;
    }
}

void USBCAN::ptz_stop()
{
    emit control(STOP, 0);
}

void USBCAN::ptz_set_angle(float h, float v)
{
    emit control(POSITION, h);
    emit control(PITCH, v);
}

void USBCAN::ptz_set_angle_h(float h) { emit control(POSITION, h); }
void USBCAN::ptz_set_angle_v(float v) { emit control(PITCH, v); }

void USBCAN::device_control(qint32 op, float val)
{
    switch (op) {
    case STOP:
        transmit_data(LOCK_LOCK);
        m_pos_step = 0;
        m_pit_step = 0;
        break;
    case LEFT:
        val = std::max(0.f, std::min(120.f, val));
        transmit_data(LOCK_LOCK);
        m_pos_step = -val;
        return;
    case RIGHT:
        val = std::max(0.f, std::min(120.f, val));
        transmit_data(LOCK_LOCK);
        m_pos_step = val;
        return;
    case UP:
        val = std::max(0.f, std::min(120.f, val));
        transmit_data(LOCK_LOCK);
        m_pit_step = -val;
        return;
    case DOWN:
        val = std::max(0.f, std::min(120.f, val));
        transmit_data(LOCK_LOCK);
        m_pit_step = val;
        return;
    case POSITION:
        val = std::max(-360.f, std::min(360.f, val));
        m_position = val;
        transmit_data(POSITION);
        return;
    case PITCH:
        val = std::max(-120.f, std::min(120.f, val));
        m_pitch = val;
        transmit_data(PITCH);
        return;
    default:
        break;
    }
}

void USBCAN::handle_received()
{
    DWORD amount_left;
    CAN_OBJ frame[100];
    ZeroMemory(frame, sizeof(CAN_OBJ) * 100);

    m_read_mutex.lock();
    amount_left = Receive(m_device_type, m_device_index, CAN1, frame, 100, 0);

    if (GetReceiveNum(m_device_type, m_device_index, CAN1) > 0) ClearBuffer(m_device_type, m_device_index, CAN1);
    m_read_mutex.unlock();

    for (int i = 0; i < 100; i++) {
        if (!frame[i].DataLen) continue;
        switch (frame[i].ID) {
        case 0x284:
            memcpy(&m_position, frame[i].Data, 4);
//            qDebug() << "position" << m_position;
            break;
        case 0x384:
            memcpy(&m_pitch, frame[i].Data, 4);
//            qDebug() << "pitch" << m_pitch;
            break;
        default: break;
        }
//        print_can_frame(frame[i], false);
    }
}

void USBCAN::transmit_power_status()
{
    CAN_OBJ frame;
    ZeroMemory(&frame, sizeof(CAN_OBJ));
    frame.ID = 0x705;
    frame.DataLen = 1;
    frame.SendType = 1;
    frame.RemoteFlag = 0;
    frame.ExternFlag = 0;
    frame.TimeFlag = 0;
    frame.TimeStamp = 0;
    frame.Data[0] = m_power;

    m_read_mutex.lock();
    DWORD sent = Transmit(m_device_type, m_device_index, m_can_index, &frame, 1);
    m_read_mutex.unlock();
//    if (sent == 1) print_can_frame(frame, true);
}

void USBCAN::transmit_ptz_status()
{
    static long long data_22d = 0x0007D0000007D004;
    static long long data_32d = 0x0000000000000004;
    CAN_OBJ frame[6];
    ZeroMemory(&frame, sizeof(CAN_OBJ) * 6);

//    // ptz status -> lock
//    frame[0].ID = 0x204;
//    frame[0].DataLen = 1;
//    frame[0].SendType = 1;
//    frame[0].RemoteFlag = 0;
//    frame[0].ExternFlag = 0;
//    frame[0].TimeFlag = 0;
//    frame[0].TimeStamp = 0;
//    frame[0].Data[0] = 0x03;

    // position
    frame[1].ID = 0x304;
    frame[1].DataLen = 4;
    frame[1].SendType = 1;
    frame[1].RemoteFlag = 0;
    frame[1].ExternFlag = 0;
    frame[1].TimeFlag = 0;
    frame[1].TimeStamp = 0;
    memcpy(frame[1].Data, &m_pos_step, 4);

    // pitch
    frame[2].ID = 0x404;
    frame[2].DataLen = 4;
    frame[2].SendType = 1;
    frame[2].RemoteFlag = 0;
    frame[2].ExternFlag = 0;
    frame[2].TimeFlag = 0;
    frame[2].TimeStamp = 0;
    memcpy(frame[2].Data, &m_pit_step, 4);

    // roll
    frame[3].ID = 0x504;
    frame[3].DataLen = 4;
    frame[3].SendType = 1;
    frame[3].RemoteFlag = 0;
    frame[3].ExternFlag = 0;
    frame[3].TimeFlag = 0;
    frame[3].TimeStamp = 0;

//    // 22d
//    frame[4].ID = 0x22D;
//    frame[4].DataLen = 8;
//    frame[4].SendType = 1;
//    frame[4].RemoteFlag = 0;
//    frame[4].ExternFlag = 0;
//    frame[4].TimeFlag = 0;
//    frame[4].TimeStamp = 0;
//    memcpy(frame[4].Data, &data_22d, 8);

//    // 32d
//    frame[5].ID = 0x32D;
//    frame[5].DataLen = 8;
//    frame[5].SendType = 1;
//    frame[5].RemoteFlag = 0;
//    frame[5].ExternFlag = 0;
//    frame[5].TimeFlag = 0;
//    frame[5].TimeStamp = 0;
//    memcpy(frame[5].Data, &data_32d, 8);

    DWORD sent;
    m_read_mutex.lock();
//    sent = Transmit(m_device_type, m_device_index, m_can_index, frame + 0, 1);
//    if (sent == 1) print_can_frame(frame[0], true);
    sent = Transmit(m_device_type, m_device_index, m_can_index, frame + 1, 1);
//    if (sent == 1) print_can_frame(frame[1], true);
    sent = Transmit(m_device_type, m_device_index, m_can_index, frame + 2, 1);
//    if (sent == 1) print_can_frame(frame[2], true);
    sent = Transmit(m_device_type, m_device_index, m_can_index, frame + 3, 1);
//    if (sent == 1) print_can_frame(frame[3], true);
//    sent = Transmit(m_device_type, m_device_index, m_can_index, frame + 4, 1);
//    if (sent == 1) print_can_frame(frame[4], true);
//    sent = Transmit(m_device_type, m_device_index, m_can_index, frame + 5, 1);
//    if (sent == 1) print_can_frame(frame[5], true);
    m_read_mutex.unlock();
}
