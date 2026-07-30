#include <QtTest>
#include <QUdpSocket>

#include "protocol709/udp709service.h"

using namespace Udp709;

class TestUdp709Service : public QObject
{
    Q_OBJECT

private slots:
    void startsDisconnected()
    {
        Service service;
        QVERIFY(!service.isBound());
    }

    void transmitsOnlyConfiguredTypesWithOneSequence()
    {
        QUdpSocket peer;
        QVERIFY(peer.bind(QHostAddress::LocalHost, 0, QUdpSocket::DontShareAddress));

        QUdpSocket reservation;
        QVERIFY(reservation.bind(QHostAddress::LocalHost, 0, QUdpSocket::DontShareAddress));
        const quint16 localPort = reservation.localPort();
        reservation.close();

        Service service;
        QString error;
        QVERIFY2(service.bindEndpoint("127.0.0.1", localPort,
                                      "127.0.0.1", peer.localPort(), &error),
                 qPrintable(error));

        QVERIFY(!Service::isTransmitId(InfoId::ElectromagneticStatus));
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        QVERIFY(!service.sendElectromagneticStatus(ElectromagneticStatusPayload(), &error));
QT_WARNING_POP
        QVERIFY(error.contains("deprecated", Qt::CaseInsensitive));
        QVERIFY(!peer.waitForReadyRead(50));

        error.clear();
        QVERIFY(service.sendDeviceStatus(DeviceStatusPayload(), &error));
        QVERIFY(peer.waitForReadyRead(1000));
        QByteArray first;
        first.resize(static_cast<int>(peer.pendingDatagramSize()));
        QCOMPARE(peer.readDatagram(first.data(), first.size()), qint64(first.size()));
        QCOMPARE(static_cast<quint8>(first.at(0)), quint8(0));
        QCOMPARE(static_cast<quint8>(first.at(1)), quint8(0x95));

        QVERIFY(service.sendElectroOpticalTarget(ElectroOpticalTargetPayload(), &error));
        QVERIFY(peer.waitForReadyRead(1000));
        QByteArray second;
        second.resize(static_cast<int>(peer.pendingDatagramSize()));
        QCOMPARE(peer.readDatagram(second.data(), second.size()), qint64(second.size()));
        QCOMPARE(static_cast<quint8>(second.at(0)), quint8(1));
        QCOMPARE(static_cast<quint8>(second.at(1)), quint8(0x96));
    }

    void receivesOnlyConfiguredTypes()
    {
        QUdpSocket peer;
        QVERIFY(peer.bind(QHostAddress::LocalHost, 0, QUdpSocket::DontShareAddress));

        QUdpSocket reservation;
        QVERIFY(reservation.bind(QHostAddress::LocalHost, 0, QUdpSocket::DontShareAddress));
        const quint16 localPort = reservation.localPort();
        reservation.close();

        Service service;
        QString error;
        QVERIFY(service.bindEndpoint("127.0.0.1", localPort,
                                     "127.0.0.1", peer.localPort(), &error));
        QSignalSpy received(&service, &Service::packetReceived);
        QSignalSpy errors(&service, &Service::transportError);

        const QByteArray allowed = PacketCodec::encodeAttitude(7, AttitudePayload(), &error);
        QCOMPARE(peer.writeDatagram(allowed, QHostAddress::LocalHost, localPort), qint64(allowed.size()));
        QTRY_COMPARE_WITH_TIMEOUT(received.count(), 1, 1000);
        const DecodedPacket packet = qvariant_cast<DecodedPacket>(received.takeFirst().at(0));
        QCOMPARE(packet.infoId, InfoId::Attitude);

        const QByteArray rejected = PacketCodec::encodeElectromagneticStatus(
                    8, ElectromagneticStatusPayload(), &error);
        QCOMPARE(peer.writeDatagram(rejected, QHostAddress::LocalHost, localPort), qint64(rejected.size()));
        QTRY_VERIFY_WITH_TIMEOUT(errors.count() > 0, 1000);
        QCOMPARE(received.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestUdp709Service)
#include "test_udp709_service.moc"
