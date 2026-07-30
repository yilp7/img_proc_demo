#include "protocol709/packetcodec.h"

#include <QCoreApplication>
#include <QDebug>

#include <cmath>

using namespace Udp709;

namespace {
int failures = 0;

void expect(bool condition, const QString &message)
{
    if (!condition) {
        qCritical().noquote() << "FAIL:" << message;
        ++failures;
    }
}

void expectNear(double actual, double expected, double tolerance, const QString &message)
{
    expect(std::abs(actual - expected) <= tolerance,
           QStringLiteral("%1 (actual=%2 expected=%3)").arg(message).arg(actual).arg(expected));
}

void testIdsAndLengths()
{
    expect(PacketCodec::rawInfoId(InfoId::Attitude) == 0xb0, QStringLiteral("B0 wire ID"));
    expect(PacketCodec::rawInfoId(InfoId::TimeCode) == 0xb3, QStringLiteral("B3 wire ID"));
    expect(PacketCodec::rawInfoId(InfoId::SituationDistribution) == 0x90,
           QStringLiteral("G0 wire ID"));
    expect(PacketCodec::rawInfoId(InfoId::ElectroOpticalTarget) == 0x96,
           QStringLiteral("G6 wire ID"));
    expect(PacketCodec::infoIdFromRaw(0xf0) == InfoId::Unknown,
           QStringLiteral("Temporary F-series IDs are rejected"));
    expect(PacketCodec::expectedUnitLength(InfoId::ManualTargetGuidance) == 24,
           QStringLiteral("Calculated G1 length"));
    expect(PacketCodec::expectedUnitLength(InfoId::ElectroOpticalTarget) == 33,
           QStringLiteral("Calculated G6 length"));
}

void testAttitude()
{
    AttitudePayload input;
    input.validityFlags = 0x1234;
    input.headingDegrees = 180.0;

    QString error;
    const QByteArray datagram = PacketCodec::encodeAttitude(0x12, input, &error);
    expect(error.isEmpty(), QStringLiteral("B0 encoding error: %1").arg(error));
    expect(datagram.toHex() == QByteArray("12b000121234400000000000000000000000"),
           QStringLiteral("B0 big-endian wire bytes"));

    DecodedPacket decoded;
    expect(PacketCodec::decode(datagram, &decoded, &error), QStringLiteral("B0 decode: %1").arg(error));
    AttitudePayload output;
    expect(PacketCodec::parseAttitude(decoded, &output, &error), QStringLiteral("B0 parse: %1").arg(error));
    expect(output.validityFlags == input.validityFlags, QStringLiteral("B0 validity flags"));
    expectNear(output.headingDegrees, 180.0, 0.000001, QStringLiteral("B0 heading"));
}

void testTimeCode()
{
    TimeCodePayload input;
    input.second = 58;
    input.minute = 59;
    input.hour = 23;
    input.westOfUtcPlus8 = true;
    input.timeZoneOffsetHours = 8;
    input.day = 11;
    input.month = 7;
    input.year = 2026;

    QString error;
    const QByteArray datagram = PacketCodec::encodeTimeCode(0x34, input, &error);
    expect(datagram.toHex() == QByteArray("34b3000c5859238811072620"),
           QStringLiteral("B3 BCD and timezone bytes"));

    DecodedPacket decoded;
    TimeCodePayload output;
    expect(PacketCodec::decode(datagram, &decoded, &error), QStringLiteral("B3 decode: %1").arg(error));
    expect(PacketCodec::parseTimeCode(decoded, &output, &error), QStringLiteral("B3 parse: %1").arg(error));
    expect(output.year == 2026 && output.month == 7 && output.day == 11,
           QStringLiteral("B3 decoded date"));
    expect(output.westOfUtcPlus8 && output.timeZoneOffsetHours == 8,
           QStringLiteral("B3 decoded timezone correction"));
}

void testCalculatedFixedLengths()
{
    QString error;
    ManualTargetGuidancePayload guidance;
    guidance.timestampRaw = 0x01020304;
    guidance.targetBatch = 0x1122;
    guidance.command = 1;
    const QByteArray guidancePacket = PacketCodec::encodeManualTargetGuidance(1, guidance, &error);
    expect(guidancePacket.size() == 24, QStringLiteral("G1 calculated 24-byte packet"));
    expect(guidancePacket.mid(1, 3).toHex() == QByteArray("910018"),
           QStringLiteral("G1 final ID and calculated length header"));

    ElectroOpticalTargetPayload target;
    target.timestampTenthsOfMillisecond = 1234;
    target.reportType = 3;
    target.targetBatch = 42;
    const QByteArray targetPacket = PacketCodec::encodeElectroOpticalTarget(2, target, &error);
    expect(targetPacket.size() == 33, QStringLiteral("G6 calculated 33-byte packet"));
    expect(targetPacket.mid(1, 3).toHex() == QByteArray("960021"),
           QStringLiteral("G6 final ID and calculated length header"));

    DecodedPacket decoded;
    expect(PacketCodec::decode(targetPacket, &decoded, &error), QStringLiteral("G6 decode: %1").arg(error));
    ElectroOpticalTargetPayload parsed;
    expect(PacketCodec::parseElectroOpticalTarget(decoded, &parsed, &error),
           QStringLiteral("G6 parse: %1").arg(error));
    expect(parsed.reportType == 3 && parsed.targetBatch == 42, QStringLiteral("G6 decoded fields"));
}

void testVariableSituationLength()
{
    SituationDistributionPayload input;
    input.timestampMilliseconds = 1000;
    input.fusionBatch = 7;
    input.sources = {{1, 100}, {2, 200}};

    QString error;
    const QByteArray datagram = PacketCodec::encodeSituationDistribution(3, input, &error);
    expect(datagram.size() == 86, QStringLiteral("G0 78 + 4*N length"));
    expect(datagram.mid(1, 3).toHex() == QByteArray("900056"),
           QStringLiteral("G0 final ID and dynamic length header"));

    DecodedPacket decoded;
    SituationDistributionPayload output;
    expect(PacketCodec::decode(datagram, &decoded, &error), QStringLiteral("G0 decode: %1").arg(error));
    expect(PacketCodec::parseSituationDistribution(decoded, &output, &error),
           QStringLiteral("G0 parse: %1").arg(error));
    expect(output.sources.size() == 2, QStringLiteral("G0 source count"));
    expect(output.sources.at(1).sourceCode == 2 && output.sources.at(1).associatedTargetBatch == 200,
           QStringLiteral("G0 source fields"));

    const QByteArray invalidBody(16, '\0');
    expect(PacketCodec::encodeRaw(0, InfoId::ManualTargetGuidance, invalidBody, &error).isEmpty(),
           QStringLiteral("Fixed-length raw packet rejects wrong body size"));
}

void testTurretAzimuthModes()
{
    QString error;
    TurretControlPayload input;
    input.turretState = 0x02;
    input.azimuthRaw = static_cast<quint16>(static_cast<qint16>(-1234));
    input.elevationHundredths = -567;

    const QByteArray datagram = PacketCodec::encodeTurretControl(4, input, &error);
    DecodedPacket decoded;
    expect(PacketCodec::decode(datagram, &decoded, &error),
           QStringLiteral("G3 manual-search decode: %1").arg(error));
    expect(decoded.fields.value(QStringLiteral("azimuthRaw")).toInt() == -1234,
           QStringLiteral("G3 manual-search azimuth is displayed as signed velocity"));

    TurretControlPayload output;
    expect(PacketCodec::parseTurretControl(decoded, &output, &error),
           QStringLiteral("G3 manual-search parse: %1").arg(error));
    expect(static_cast<qint16>(output.azimuthRaw) == -1234
           && output.elevationHundredths == -567,
           QStringLiteral("G3 manual-search velocity bits are preserved"));

    input.turretState = 0x01;
    input.azimuthRaw = 35000;
    const QByteArray fixedAngleDatagram = PacketCodec::encodeTurretControl(5, input, &error);
    expect(PacketCodec::decode(fixedAngleDatagram, &decoded, &error),
           QStringLiteral("G3 fixed-angle decode: %1").arg(error));
    expect(decoded.fields.value(QStringLiteral("azimuthRaw")).toInt() == 35000,
           QStringLiteral("G3 fixed-angle azimuth remains unsigned"));
}

void testAllDefaultPacketLayouts()
{
    QString error;
    const auto verify = [&error](const QByteArray &datagram, InfoId expectedId, int expectedLength) {
        expect(datagram.size() == expectedLength,
               QStringLiteral("%1 encoded length").arg(PacketCodec::infoIdLabel(expectedId)));
        DecodedPacket decoded;
        expect(PacketCodec::decode(datagram, &decoded, &error),
               QStringLiteral("%1 default decode: %2").arg(PacketCodec::infoIdLabel(expectedId), error));
        expect(decoded.infoId == expectedId, QStringLiteral("%1 decoded ID").arg(PacketCodec::infoIdLabel(expectedId)));
    };

    verify(PacketCodec::encodeIntegratedNavigation(0, IntegratedNavigationPayload(), &error),
           InfoId::IntegratedNavigation, 58);
    verify(PacketCodec::encodeElectromagneticStatus(0, ElectromagneticStatusPayload(), &error),
           InfoId::ElectromagneticStatus, 14);
    verify(PacketCodec::encodeTurretControl(0, TurretControlPayload(), &error),
           InfoId::TurretControl, 22);
    verify(PacketCodec::encodeLaserNirControl(0, LaserNirControlPayload(), &error),
           InfoId::LaserNirControl, 24);
    verify(PacketCodec::encodeDeviceStatus(0, DeviceStatusPayload(), &error),
           InfoId::DeviceStatus, 18);
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    qInfo() << "Running ID and length tests";
    testIdsAndLengths();
    qInfo() << "Running attitude tests";
    testAttitude();
    qInfo() << "Running time-code tests";
    testTimeCode();
    qInfo() << "Running fixed-length tests";
    testCalculatedFixedLengths();
    qInfo() << "Running variable-length tests";
    testVariableSituationLength();
    qInfo() << "Running G3 turret-mode tests";
    testTurretAzimuthModes();
    qInfo() << "Running remaining packet-layout tests";
    testAllDefaultPacketLayouts();

    if (failures == 0) {
        qInfo() << "All PacketCodec tests passed";
    }
    return failures == 0 ? 0 : 1;
}
