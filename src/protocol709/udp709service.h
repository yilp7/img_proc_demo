#ifndef UDP709SERVICE_H
#define UDP709SERVICE_H

#include <QObject>
#include <QHostAddress>

#include "protocol709/packetcodec.h"

class QUdpSocket;

namespace Udp709 {

class Service : public QObject
{
    Q_OBJECT

public:
    explicit Service(QObject *parent = nullptr);

    bool bindEndpoint(const QString &localIp, quint16 localPort,
                      const QString &peerIp, quint16 peerPort,
                      QString *error = nullptr);
    void close();
    bool isBound() const;

    QString localIp() const;
    quint16 localPort() const;
    QString peerIp() const;
    quint16 peerPort() const;

    Q_DECL_DEPRECATED_X("UDP709 G2 transmission is deprecated")
    bool sendElectromagneticStatus(const ElectromagneticStatusPayload &payload,
                                   QString *error = nullptr);
    bool sendDeviceStatus(const DeviceStatusPayload &payload,
                          QString *error = nullptr);
    bool sendElectroOpticalTarget(const ElectroOpticalTargetPayload &payload,
                                  QString *error = nullptr);

    static bool isTransmitId(InfoId infoId);
    static bool isReceiveId(InfoId infoId);

signals:
    void boundChanged(bool bound, QString description);
    void packetReceived(Udp709::DecodedPacket packet, QString senderIp, quint16 senderPort);
    void packetSent(Udp709::DecodedPacket packet);
    void transportError(QString message);

private slots:
    void readPendingDatagrams();

private:
    bool sendEncoded(const QByteArray &datagram, QString *error);
    quint8 nextSequence() const { return m_sequence; }
    void consumeSequence() { ++m_sequence; }

    QUdpSocket *m_socket;
    QHostAddress m_localAddress;
    quint16 m_localPort = 0;
    QHostAddress m_peerAddress;
    quint16 m_peerPort = 0;
    quint8 m_sequence = 0;
};

} // namespace Udp709

Q_DECLARE_METATYPE(Udp709::DecodedPacket)

#endif // UDP709SERVICE_H
