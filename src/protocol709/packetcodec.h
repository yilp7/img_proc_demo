#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

namespace Udp709 {

enum class InfoId : quint8 {
    Attitude = 0xb0,
    IntegratedNavigation = 0xb1,
    TimeCode = 0xb3,

    // Final one-byte wire identifiers for G0-G6.
    SituationDistribution = 0x90,
    ManualTargetGuidance = 0x91,
    ElectromagneticStatus = 0x92, // Deprecated TX type; retained for codec compatibility.
    TurretControl = 0x93,
    LaserNirControl = 0x94,
    DeviceStatus = 0x95,
    ElectroOpticalTarget = 0x96,

    Unknown = 0xff
};

struct AttitudePayload {
    quint16 validityFlags = 0;
    double headingDegrees = 0.0;
    double pitchDegrees = 0.0;
    double rollDegrees = 0.0;
};

struct IntegratedNavigationPayload {
    quint16 validityFlags = 0xb040;
    double relativeSpeedKnots = 0.0;
    double absoluteSpeedKnots = 0.0;
    double trackAngleDegrees = 0.0;
    double eastSpeedKnots = 0.0;
    double northSpeedKnots = 0.0;
    double longitudeDegrees = 0.0;
    double latitudeDegrees = 0.0;
    double verticalSpeedMetersPerSecond = 0.0;
    QByteArray timeBytes = QByteArray(8, '\0');
    double trueWindSpeedMetersPerSecond = 0.0;
    double trueWindDirectionDegrees = 0.0;
    double relativeWindSpeedMetersPerSecond = 0.0;
    double relativeWindDirectionDegrees = 0.0;
    double temperatureCelsius = 0.0;
    quint16 relativeHumidityPercent = 0;
    quint16 atmosphericPressureHpa = 0;
    quint16 seaCurrentSpeedRaw = 0;
    quint16 seaCurrentDirectionRaw = 0;
    double seaDepthMeters = 0.0;
    quint16 seaStateRaw = 0;
};

struct TimeCodePayload {
    int second = 0;
    int minute = 0;
    int hour = 0;
    bool westOfUtcPlus8 = false;
    quint8 timeZoneOffsetHours = 0;
    int day = 1;
    int month = 1;
    int year = 2000;
};

struct SituationSource {
    quint16 sourceCode = 0;
    quint16 associatedTargetBatch = 0;
};

struct SituationDistributionPayload {
    quint32 timestampMilliseconds = 0;
    quint16 fusionBatch = 0;
    quint8 targetType = 0;
    quint8 targetCategory = 0;
    quint8 militaryCivilAttribute = 0;
    quint8 friendFoeAttribute = 0;
    quint16 countryRegion = 1023;
    quint32 targetRangeRaw = 0;
    quint32 targetAzimuthRaw = 0;
    qint32 targetElevationRaw = 0;
    double longitudeDegrees = 0.0;
    double latitudeDegrees = 0.0;
    quint32 depthHeightRaw = 0xffffffffu;
    quint32 relativeHeadingRaw = 0;
    quint32 absoluteHeadingRaw = 0;
    qint32 relativeSpeedRaw = 0;
    qint32 absoluteSpeedRaw = 0;
    qint32 velocityXRaw = 0;
    qint32 velocityYRaw = 0;
    qint32 velocityZRaw = 0;
    quint32 reserved1 = 0;
    quint32 reserved2 = 0;
    QVector<SituationSource> sources;
};

struct ManualTargetGuidancePayload {
    quint32 timestampRaw = 0;
    quint16 targetBatch = 0;
    quint32 targetRangeRaw = 0;
    quint32 targetAzimuthRaw = 0;
    qint32 targetElevationRaw = 0;
    quint16 command = 0;
};

struct ElectromagneticStatusPayload {
    quint32 timestampRaw = 0;
    quint8 switchState = 0;
    quint16 spectrumStartRaw = 0;
    quint16 spectrumEndRaw = 0;
    quint8 reserved = 0;
};

struct TurretControlPayload {
    quint32 timestampRaw = 0;
    quint8 visibleLightState = 0;
    quint8 midWaveInfraredState = 0;
    quint8 laserNirState = 0;
    // Interpret as unsigned in fixed-angle mode or two's-complement in manual-search mode.
    quint16 azimuthRaw = 0;
    qint16 elevationHundredths = 0;
    quint8 trackingState = 0;
    quint8 turretState = 0;
    quint8 imagingLensControl = 0;
    quint32 reserved = 0;
};

struct LaserNirControlPayload {
    quint32 timestampRaw = 0;
    quint8 laserEnabled = 0;
    quint16 laserIntensity = 0;
    quint8 illuminationLensControl = 0;
    quint32 workingDistanceMeters = 0;
    quint16 imagingDepthMeters = 0;
    quint8 imagingGain = 0;
    quint8 imageProcessing = 0;
    quint32 reserved = 0;
};

struct DeviceStatusPayload {
    quint32 timestampRaw = 0;
    quint8 visibleLightState = 0;
    quint8 midWaveInfraredState = 0;
    quint8 laserNirState = 0;
    quint16 azimuthHundredths = 0;
    qint16 elevationHundredths = 0;
    quint8 trackingState = 0;
    quint16 turretState = 0;
};

struct ElectroOpticalTargetPayload {
    quint32 timestampTenthsOfMillisecond = 0;
    quint8 reportType = 1;
    quint16 targetBatch = 0;
    quint8 targetType = 0;
    quint8 targetCategory = 0;
    quint32 targetRangeMeters = 0;
    quint32 targetAzimuthHundredths = 0;
    qint32 targetElevationHundredths = 0;
    QByteArray reserved = QByteArray(8, '\0');
};

struct DecodedPacket {
    quint8 frameNumber = 0;
    InfoId infoId = InfoId::Unknown;
    quint8 rawInfoId = 0xff;
    QString infoName;
    int datagramLength = 0;
    int unitLength = 0;
    int payloadLength = 0;
    QJsonObject fields;
    QByteArray rawPayload;
    QDateTime receivedAt;
};

class PacketCodec
{
public:
    static constexpr int HeaderSize = 4;
    static constexpr int VariableUnitLength = -1;

    static QByteArray encodeRaw(quint8 frameNumber,
                                InfoId infoId,
                                const QByteArray &payload,
                                QString *error = nullptr);
    static QByteArray encodeAttitude(quint8 frameNumber, const AttitudePayload &payload, QString *error = nullptr);
    static QByteArray encodeIntegratedNavigation(quint8 frameNumber,
                                                 const IntegratedNavigationPayload &payload,
                                                 QString *error = nullptr);
    static QByteArray encodeTimeCode(quint8 frameNumber, const TimeCodePayload &payload, QString *error = nullptr);
    static QByteArray encodeSituationDistribution(quint8 frameNumber,
                                                  const SituationDistributionPayload &payload,
                                                  QString *error = nullptr);
    static QByteArray encodeManualTargetGuidance(quint8 frameNumber,
                                                 const ManualTargetGuidancePayload &payload,
                                                 QString *error = nullptr);
    static QByteArray encodeElectromagneticStatus(quint8 frameNumber,
                                                  const ElectromagneticStatusPayload &payload,
                                                  QString *error = nullptr);
    static QByteArray encodeTurretControl(quint8 frameNumber, const TurretControlPayload &payload, QString *error = nullptr);
    static QByteArray encodeLaserNirControl(quint8 frameNumber,
                                           const LaserNirControlPayload &payload,
                                           QString *error = nullptr);
    static QByteArray encodeDeviceStatus(quint8 frameNumber, const DeviceStatusPayload &payload, QString *error = nullptr);
    static QByteArray encodeElectroOpticalTarget(quint8 frameNumber,
                                                const ElectroOpticalTargetPayload &payload,
                                                QString *error = nullptr);

    static bool decode(const QByteArray &datagram, DecodedPacket *packet, QString *error = nullptr);
    static bool parseAttitude(const DecodedPacket &packet, AttitudePayload *payload, QString *error = nullptr);
    static bool parseIntegratedNavigation(const DecodedPacket &packet,
                                          IntegratedNavigationPayload *payload,
                                          QString *error = nullptr);
    static bool parseTimeCode(const DecodedPacket &packet, TimeCodePayload *payload, QString *error = nullptr);
    static bool parseSituationDistribution(const DecodedPacket &packet,
                                           SituationDistributionPayload *payload,
                                           QString *error = nullptr);
    static bool parseManualTargetGuidance(const DecodedPacket &packet,
                                          ManualTargetGuidancePayload *payload,
                                          QString *error = nullptr);
    static bool parseElectromagneticStatus(const DecodedPacket &packet,
                                           ElectromagneticStatusPayload *payload,
                                           QString *error = nullptr);
    static bool parseTurretControl(const DecodedPacket &packet, TurretControlPayload *payload, QString *error = nullptr);
    static bool parseLaserNirControl(const DecodedPacket &packet,
                                    LaserNirControlPayload *payload,
                                    QString *error = nullptr);
    static bool parseDeviceStatus(const DecodedPacket &packet, DeviceStatusPayload *payload, QString *error = nullptr);
    static bool parseElectroOpticalTarget(const DecodedPacket &packet,
                                         ElectroOpticalTargetPayload *payload,
                                         QString *error = nullptr);

    static bool validateUnitLength(InfoId infoId, int unitLength, QString *error = nullptr);
    static QString infoIdLabel(InfoId infoId);
    static QString infoIdLabel(quint8 rawInfoId);
    static InfoId infoIdFromRaw(quint8 rawInfoId);
    static quint8 rawInfoId(InfoId infoId);
    static int expectedUnitLength(InfoId infoId);
};

} // namespace Udp709
