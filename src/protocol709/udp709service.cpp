#include "protocol709/udp709service.h"

#include <QUdpSocket>

namespace Udp709 {
namespace {

void setError(QString *error, const QString &message)
{
    if (error) *error = message;
}

QString endpointText(const QHostAddress &address, quint16 port)
{
    return QStringLiteral("%1:%2").arg(address.toString()).arg(port);
}

} // namespace

Service::Service(QObject *parent)
    : QObject(parent),
      m_socket(new QUdpSocket(this))
{
    qRegisterMetaType<Udp709::DecodedPacket>("Udp709::DecodedPacket");
    connect(m_socket, &QUdpSocket::readyRead, this, &Service::readPendingDatagrams);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error),
            this, [this](QAbstractSocket::SocketError) {
        emit transportError(m_socket->errorString());
    });
}

bool Service::bindEndpoint(const QString &localIp, quint16 localPort,
                           const QString &peerIp, quint16 peerPort,
                           QString *error)
{
    QHostAddress localAddress;
    QHostAddress peerAddress;
    if (!localAddress.setAddress(localIp.trimmed())) {
        setError(error, tr("Invalid local IP address: %1").arg(localIp));
        return false;
    }
    if (!peerAddress.setAddress(peerIp.trimmed())) {
        setError(error, tr("Invalid peer IP address: %1").arg(peerIp));
        return false;
    }
    if (localPort == 0 || peerPort == 0) {
        setError(error, tr("UDP ports must be between 1 and 65535"));
        return false;
    }

    close();
    if (!m_socket->bind(localAddress, localPort, QUdpSocket::DontShareAddress)) {
        setError(error, tr("Cannot bind %1: %2")
                 .arg(endpointText(localAddress, localPort), m_socket->errorString()));
        emit boundChanged(false, m_socket->errorString());
        return false;
    }

    m_localAddress = localAddress;
    m_localPort = localPort;
    m_peerAddress = peerAddress;
    m_peerPort = peerPort;
    m_sequence = 0;
    emit boundChanged(true, tr("Bound %1, peer %2")
                      .arg(endpointText(m_localAddress, m_localPort),
                           endpointText(m_peerAddress, m_peerPort)));
    return true;
}

void Service::close()
{
    if (m_socket->state() == QAbstractSocket::UnconnectedState) return;
    m_socket->close();
    emit boundChanged(false, tr("Not connected"));
}

bool Service::isBound() const
{
    return m_socket->state() == QAbstractSocket::BoundState;
}

QString Service::localIp() const { return m_localAddress.toString(); }
quint16 Service::localPort() const { return m_localPort; }
QString Service::peerIp() const { return m_peerAddress.toString(); }
quint16 Service::peerPort() const { return m_peerPort; }

bool Service::sendElectromagneticStatus(const ElectromagneticStatusPayload &payload,
                                        QString *error)
{
    Q_UNUSED(payload)
    setError(error, tr("UDP709 G2 transmission is deprecated"));
    return false;
}

bool Service::sendDeviceStatus(const DeviceStatusPayload &payload, QString *error)
{
    const QByteArray datagram = PacketCodec::encodeDeviceStatus(nextSequence(), payload, error);
    return datagram.isEmpty() ? false : sendEncoded(datagram, error);
}

bool Service::sendElectroOpticalTarget(const ElectroOpticalTargetPayload &payload,
                                       QString *error)
{
    const QByteArray datagram = PacketCodec::encodeElectroOpticalTarget(nextSequence(), payload, error);
    return datagram.isEmpty() ? false : sendEncoded(datagram, error);
}

bool Service::isTransmitId(InfoId infoId)
{
    return infoId == InfoId::DeviceStatus
            || infoId == InfoId::ElectroOpticalTarget;
}

bool Service::isReceiveId(InfoId infoId)
{
    return infoId == InfoId::Attitude
            || infoId == InfoId::IntegratedNavigation
            || infoId == InfoId::TimeCode
            || infoId == InfoId::SituationDistribution
            || infoId == InfoId::ManualTargetGuidance
            || infoId == InfoId::TurretControl
            || infoId == InfoId::LaserNirControl;
}

bool Service::sendEncoded(const QByteArray &datagram, QString *error)
{
    if (!isBound()) {
        setError(error, tr("UDP 709 is not bound"));
        return false;
    }

    DecodedPacket decoded;
    QString decodeError;
    if (!PacketCodec::decode(datagram, &decoded, &decodeError) || !isTransmitId(decoded.infoId)) {
        setError(error, decodeError.isEmpty() ? tr("Packet is not allowed for transmission") : decodeError);
        return false;
    }

    const qint64 written = m_socket->writeDatagram(datagram, m_peerAddress, m_peerPort);
    consumeSequence();
    if (written != datagram.size()) {
        const QString message = tr("UDP send failed: %1").arg(m_socket->errorString());
        setError(error, message);
        emit transportError(message);
        return false;
    }

    emit packetSent(decoded);
    return true;
}

void Service::readPendingDatagrams()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        const qint64 received = m_socket->readDatagram(datagram.data(), datagram.size(),
                                                       &sender, &senderPort);
        if (received < 0) {
            emit transportError(tr("UDP receive failed: %1").arg(m_socket->errorString()));
            continue;
        }
        datagram.resize(static_cast<int>(received));

        DecodedPacket decoded;
        QString error;
        if (!PacketCodec::decode(datagram, &decoded, &error)) {
            emit transportError(tr("Rejected packet from %1: %2")
                                .arg(endpointText(sender, senderPort), error));
            continue;
        }
        if (!isReceiveId(decoded.infoId)) {
            emit transportError(tr("Unexpected RX packet %1 from %2")
                                .arg(decoded.infoName, endpointText(sender, senderPort)));
            continue;
        }
        emit packetReceived(decoded, sender.toString(), senderPort);
    }
}

} // namespace Udp709
