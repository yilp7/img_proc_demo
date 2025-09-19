#include "udpptz.h"

UDPPTZ::UDPPTZ():
    QObject{nullptr},
    udp_socket(nullptr),
    target_ip(QHostAddress("192.168.1.10")),
    target_port(30000),
    local_ip(QHostAddress("192.168.1.100")),
    local_port(30001),
    connected(false),
    timer_tx(nullptr),
    timer_rx(nullptr),
    current_operation(STANDBY),
    horizontal_angle(0),
    vertical_angle(0),
    target_horizontal_angle(0),
    target_vertical_angle(0),
    horizontal_velocity(0),
    vertical_velocity(0),
    scan_range_horizontal(0),
    scan_range_vertical(0),
    servo_status(STANDBY),
    last_received_time(0)
{
    connect(this, &UDPPTZ::connect_to, this, &UDPPTZ::connect_to_device, Qt::QueuedConnection);
    connect(this, &UDPPTZ::disconnect_from, this, &UDPPTZ::disconnect_from_device, Qt::QueuedConnection);
    connect(this, &UDPPTZ::transmit, this, &UDPPTZ::transmit_data, Qt::QueuedConnection);
    connect(this, &UDPPTZ::control, this, &UDPPTZ::device_control, Qt::QueuedConnection);

    timer_tx = new QTimer(this);
    timer_tx->setTimerType(Qt::PreciseTimer);
    timer_tx->setInterval(20); // 20ms transmission cycle
    connect(timer_tx, &QTimer::timeout, this, &UDPPTZ::transmit_status);

    timer_rx = new QTimer(this);
    timer_rx->setTimerType(Qt::PreciseTimer);
    timer_rx->setInterval(5); // 5ms receive check
    connect(timer_rx, &QTimer::timeout, this, &UDPPTZ::handle_received);
}

UDPPTZ::~UDPPTZ()
{
    if (timer_tx) {
        timer_tx->stop();
        timer_tx->deleteLater();
    }
    if (timer_rx) {
        timer_rx->stop();
        timer_rx->deleteLater();
    }
    if (udp_socket) {
        udp_socket->close();
        udp_socket->deleteLater();
    }
}

void UDPPTZ::set_target(QHostAddress target, quint16 port)
{
    target_ip = target;
    target_port = port;
}

void UDPPTZ::set_listen(QHostAddress listen, quint16 port)
{
    local_ip = listen;
    local_port = port;
}

bool UDPPTZ::connect_to_device()
{
    if (connected) return false;

    udp_socket = new QUdpSocket(this);

    // Bind to local address for receiving status
    if (!udp_socket->bind(local_ip, local_port)) {
        qDebug() << "Failed to bind to" << local_ip.toString() << ":" << local_port;
        udp_socket->deleteLater();
        udp_socket = nullptr;
        emit connection_status_changed(false);
        return false;
    }

    connected = true;
    last_received_time = 0;
    timer_rx->start();
    timer_tx->start();

    emit connection_status_changed(true);
    qDebug() << "UDP PTZ connected";
    return true;
}

void UDPPTZ::disconnect_from_device()
{
    if (!connected) return;

    // Acquire mutex to ensure timer callbacks aren't running
    socket_mutex.lock();

    // Now safe to stop timers - callbacks can't be executing
    timer_tx->stop();
    timer_rx->stop();

    socket_mutex.unlock();  // Release before cleanup

    if (udp_socket) {
        udp_socket->close();
        udp_socket->deleteLater();
        udp_socket = nullptr;
    }

    connected = false;
    emit connection_status_changed(false);
    qDebug() << "UDP PTZ disconnected";
}

void UDPPTZ::transmit_data(qint32 op)
{
    if (!connected || !udp_socket) return;

    QByteArray frame;
    current_operation = op;

    switch (op) {
    case STANDBY:
        frame = create_command_frame(STANDBY);
        break;
    case RETURN_ZERO:
        frame = create_command_frame(RETURN_ZERO);
        break;
    case SCAN:
        frame = create_command_frame(SCAN, scan_range_horizontal, vertical_angle, scan_range_vertical);
        break;
    case MANUAL_SEARCH:
        frame = create_command_frame(MANUAL_SEARCH, horizontal_velocity, vertical_velocity);
        break;
    case ANGLE_POSITION:
        frame = create_command_frame(ANGLE_POSITION, target_horizontal_angle, target_vertical_angle);
        break;
    default:
        return;
    }

    qint64 bytes_sent = udp_socket->writeDatagram(frame, target_ip, target_port);

    if (bytes_sent == -1) {
        qDebug() << "Failed to send UDP datagram:" << udp_socket->errorString();
    }
}

void UDPPTZ::device_control(qint32 op, float val)
{
    switch (op) {
    case STANDBY:
        current_operation = STANDBY;
        transmit_data(STANDBY);
        break;
    case RETURN_ZERO:
        current_operation = RETURN_ZERO;
        transmit_data(RETURN_ZERO);
        break;
    case SCAN:
        // val represents scan range for horizontal
        scan_range_horizontal = val;
        current_operation = SCAN;
        transmit_data(SCAN);
        break;
    case MANUAL_SEARCH:
        // For manual search, we need separate horizontal and vertical velocities
        // This is simplified - in real implementation you might need separate controls
        horizontal_velocity = val;
        current_operation = MANUAL_SEARCH;
        transmit_data(MANUAL_SEARCH);
        break;
    case ANGLE_POSITION:
        // UNUSED - should not be called directly through device_control
        Q_UNUSED(val);
        break;
    case ANGLE_H:
        // Set horizontal angle only
        if (val > 180.0f) {
            target_horizontal_angle = val - 360.0f;
        } else {
            target_horizontal_angle = val;
        }
        current_operation = ANGLE_POSITION;
        transmit_data(ANGLE_POSITION);
        break;
    case ANGLE_V:
        // Set vertical angle only, clamp to -90 to 90 range
        target_vertical_angle = std::max(-90.0f, std::min(90.0f, val));
        current_operation = ANGLE_POSITION;
        transmit_data(ANGLE_POSITION);
        break;
    default:
        break;
    }
}

void UDPPTZ::handle_received()
{
    if (!connected || !udp_socket) return;

    socket_mutex.lock();
    while (udp_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udp_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 sender_port;

        qint64 bytes_received = udp_socket->readDatagram(datagram.data(), datagram.size(), &sender, &sender_port);

        if (bytes_received == 14) { // Expected frame size
            parse_status_frame(datagram);

            // Update last received time for future real-time connection check
            last_received_time = QDateTime::currentMSecsSinceEpoch();
            // TODO: Uncomment below for real-time connection monitoring
            // if (!connected) {
            //     connected = true;
            //     qDebug() << "UDP PTZ device detected and connected";
            // }

            retrieve_mutex.lock();
            last_read = datagram;
            retrieve_mutex.unlock();
        }
    }
    socket_mutex.unlock();
}

void UDPPTZ::transmit_status()
{
    // Transmit current command based on current operation
    transmit_data(current_operation);
}

QByteArray UDPPTZ::create_command_frame(qint32 op, float horizontal, float vertical, float scan_range)
{
    QByteArray frame(14, 0x00);

    // Frame headers
    frame[0] = 0xEB;
    frame[1] = 0x90;
    frame[2] = 0x01;
    frame[3] = static_cast<uchar>(op);

    // Convert float values to 16-bit integers (big-endian)
    qint16 h_value = float_to_int16(horizontal);
    qint16 v_value = float_to_int16(vertical);
    qint16 scan_value = float_to_int16(scan_range);

    // Horizontal data (bytes 4-5)
    frame[4] = static_cast<uchar>((h_value >> 8) & 0xFF);
    frame[5] = static_cast<uchar>(h_value & 0xFF);

    // Vertical data (bytes 6-7)
    frame[6] = static_cast<uchar>((v_value >> 8) & 0xFF);
    frame[7] = static_cast<uchar>(v_value & 0xFF);

    // Scan range (bytes 8-9)
    frame[8] = static_cast<uchar>((scan_value >> 8) & 0xFF);
    frame[9] = static_cast<uchar>(scan_value & 0xFF);

    // Reserved bytes (10-12) already initialized to 0x00

    // Calculate and set checksum
    frame[13] = calculate_checksum(frame);

    return frame;
}

uchar UDPPTZ::calculate_checksum(const QByteArray &data)
{
    uint sum = 0;
    for (int i = 2; i <= 12; i++) {
        sum += static_cast<uchar>(data[i]);
    }
    return static_cast<uchar>(sum & 0xFF);
}

void UDPPTZ::parse_status_frame(const QByteArray &data)
{
    if (data.size() != 14) return;

    // Verify frame headers
    if (data[0] != static_cast<char>(0xEB) ||
        data[1] != static_cast<char>(0x90) ||
        data[2] != 0x01) {
        return;
    }

    // Verify checksum
    if (static_cast<uchar>(data[13]) != calculate_checksum(data)) {
        qDebug() << "Checksum mismatch in status frame";
        return;
    }

    // Parse servo status
    servo_status = static_cast<uchar>(data[3]);

    // Parse horizontal frame angle (bytes 4-5)
    qint16 h_raw = (static_cast<uchar>(data[4]) << 8) | static_cast<uchar>(data[5]);
    float device_h_angle = int16_to_float(h_raw);
    // Convert device angle (-180 to 180) to UI angle (0-360)
    if (device_h_angle < 0.0f) {
        horizontal_angle = device_h_angle + 360.0f;
    } else {
        horizontal_angle = device_h_angle;
    }

    // Parse vertical frame angle (bytes 6-7)
    qint16 v_raw = (static_cast<uchar>(data[6]) << 8) | static_cast<uchar>(data[7]);
    vertical_angle = int16_to_float(v_raw);
    // Clamp vertical angle to -90 to 90 range
    if (vertical_angle > 90.0f) vertical_angle = 90.0f;
    if (vertical_angle < -90.0f) vertical_angle = -90.0f;

    // Parse horizontal angular rate (bytes 8-9)
    qint16 h_vel_raw = (static_cast<uchar>(data[8]) << 8) | static_cast<uchar>(data[9]);
    horizontal_velocity = int16_to_float(h_vel_raw);

    // Parse vertical angular rate (bytes 10-11)
    qint16 v_vel_raw = (static_cast<uchar>(data[10]) << 8) | static_cast<uchar>(data[11]);
    vertical_velocity = int16_to_float(v_vel_raw);

    // Emit signals
    emit angle_updated(horizontal_angle, vertical_angle);
    emit status_updated(servo_status);
}

qint16 UDPPTZ::float_to_int16(float value)
{
    // Convert degrees to 0.01 degree units
    int scaled_value = static_cast<int>(value * 100.0f);
    // Clamp to 16-bit signed integer range
    scaled_value = std::max(-32768, std::min(32767, scaled_value));
    return static_cast<qint16>(scaled_value);
}

float UDPPTZ::int16_to_float(qint16 value)
{
    // Convert from 0.01 degree units to degrees
    return static_cast<float>(value) / 100.0f;
}

void UDPPTZ::update_timer_interval(uint interval)
{
    if (interval > 0) {
        timer_tx->setInterval(interval);
    }
}

void UDPPTZ::communicate(QByteArray write, uint read_size, uint read_timeout, bool heartbeat)
{
    // This method is kept for interface compatibility with USBCAN
    // For UDP implementation, we use the timer-based approach instead
    Q_UNUSED(write)
    Q_UNUSED(read_size)
    Q_UNUSED(read_timeout)
    Q_UNUSED(heartbeat)
}
