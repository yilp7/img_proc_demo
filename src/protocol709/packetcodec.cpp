#include "protocol709/packetcodec.h"

#include <QJsonArray>
#include <QJsonValue>

#include <cmath>
#include <limits>

namespace Udp709 {

namespace {
constexpr int kMaximumUdpPayload = 65507;
constexpr double kAttitudeHeadingLsb = 180.0 / static_cast<double>(quint64(1) << 30);
constexpr double kAttitudeAngleLsb = 90.0 / static_cast<double>(quint64(1) << 30);
constexpr double kNavigationTrackLsb = 180.0 / static_cast<double>(quint64(1) << 15);
constexpr double kNavigationDirectionLsb = 180.0 / static_cast<double>(quint64(1) << 14);
constexpr double kGeoAngleLsb = 90.0 / static_cast<double>(quint64(1) << 30);

void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

void appendU8(QByteArray &bytes, quint8 value)
{
    bytes.append(static_cast<char>(value));
}

void appendU16(QByteArray &bytes, quint16 value)
{
    bytes.append(static_cast<char>((value >> 8) & 0xff));
    bytes.append(static_cast<char>(value & 0xff));
}

void appendI16(QByteArray &bytes, qint16 value)
{
    appendU16(bytes, static_cast<quint16>(value));
}

void appendU32(QByteArray &bytes, quint32 value)
{
    bytes.append(static_cast<char>((value >> 24) & 0xff));
    bytes.append(static_cast<char>((value >> 16) & 0xff));
    bytes.append(static_cast<char>((value >> 8) & 0xff));
    bytes.append(static_cast<char>(value & 0xff));
}

void appendI32(QByteArray &bytes, qint32 value)
{
    appendU32(bytes, static_cast<quint32>(value));
}

class ByteReader
{
public:
    explicit ByteReader(const QByteArray &bytes)
        : m_bytes(bytes)
    {
    }

    bool u8(quint8 *value)
    {
        if (!take(1)) {
            return false;
        }
        *value = static_cast<quint8>(m_bytes.at(m_offset - 1));
        return true;
    }

    bool u16(quint16 *value)
    {
        if (!take(2)) {
            return false;
        }
        const auto first = static_cast<quint8>(m_bytes.at(m_offset - 2));
        const auto second = static_cast<quint8>(m_bytes.at(m_offset - 1));
        *value = static_cast<quint16>((static_cast<quint16>(first) << 8) | second);
        return true;
    }

    bool i16(qint16 *value)
    {
        quint16 raw = 0;
        if (!u16(&raw)) {
            return false;
        }
        *value = static_cast<qint16>(raw);
        return true;
    }

    bool u32(quint32 *value)
    {
        if (!take(4)) {
            return false;
        }
        const int start = m_offset - 4;
        *value = (static_cast<quint32>(static_cast<quint8>(m_bytes.at(start))) << 24)
            | (static_cast<quint32>(static_cast<quint8>(m_bytes.at(start + 1))) << 16)
            | (static_cast<quint32>(static_cast<quint8>(m_bytes.at(start + 2))) << 8)
            | static_cast<quint32>(static_cast<quint8>(m_bytes.at(start + 3)));
        return true;
    }

    bool i32(qint32 *value)
    {
        quint32 raw = 0;
        if (!u32(&raw)) {
            return false;
        }
        *value = static_cast<qint32>(raw);
        return true;
    }

    bool byteArray(int size, QByteArray *value)
    {
        if (!take(size)) {
            return false;
        }
        *value = m_bytes.mid(m_offset - size, size);
        return true;
    }

    bool atEnd() const { return m_offset == m_bytes.size(); }
    int remaining() const { return m_bytes.size() - m_offset; }

private:
    bool take(int size)
    {
        if (size < 0 || m_offset + size > m_bytes.size()) {
            return false;
        }
        m_offset += size;
        return true;
    }

    const QByteArray &m_bytes;
    int m_offset = 0;
};

bool checkPacket(const DecodedPacket &packet, InfoId expected, QString *error)
{
    if (packet.infoId != expected) {
        setError(error,
                 QStringLiteral("Expected %1 but got %2")
                     .arg(PacketCodec::infoIdLabel(expected), packet.infoName));
        return false;
    }
    return true;
}

bool readerFinished(const ByteReader &reader, QString *error)
{
    if (reader.atEnd()) {
        return true;
    }
    setError(error, QStringLiteral("Packet parser left %1 unread bytes").arg(reader.remaining()));
    return false;
}

bool encodeSigned16(double value,
                    double scale,
                    double minimum,
                    double maximum,
                    const QString &name,
                    qint16 *result,
                    QString *error)
{
    if (!std::isfinite(value) || value < minimum || value > maximum) {
        setError(error,
                 QStringLiteral("%1 must be between %2 and %3")
                     .arg(name)
                     .arg(minimum)
                     .arg(maximum));
        return false;
    }
    const qint64 raw = std::llround(value / scale);
    if (raw < std::numeric_limits<qint16>::min() || raw > std::numeric_limits<qint16>::max()) {
        setError(error, QStringLiteral("%1 cannot be represented by the protocol field").arg(name));
        return false;
    }
    *result = static_cast<qint16>(raw);
    return true;
}

bool encodeUnsigned16(double value,
                      double scale,
                      double minimum,
                      double maximum,
                      const QString &name,
                      quint16 *result,
                      QString *error)
{
    if (!std::isfinite(value) || value < minimum || value > maximum) {
        setError(error,
                 QStringLiteral("%1 must be between %2 and %3")
                     .arg(name)
                     .arg(minimum)
                     .arg(maximum));
        return false;
    }
    const qint64 raw = std::llround(value / scale);
    if (raw < 0 || raw > std::numeric_limits<quint16>::max()) {
        setError(error, QStringLiteral("%1 cannot be represented by the protocol field").arg(name));
        return false;
    }
    *result = static_cast<quint16>(raw);
    return true;
}

bool encodeSigned32(double value,
                    double scale,
                    double minimum,
                    double maximum,
                    const QString &name,
                    qint32 *result,
                    QString *error)
{
    if (!std::isfinite(value) || value < minimum || value > maximum) {
        setError(error,
                 QStringLiteral("%1 must be between %2 and %3")
                     .arg(name)
                     .arg(minimum)
                     .arg(maximum));
        return false;
    }
    const qint64 raw = std::llround(value / scale);
    if (raw < std::numeric_limits<qint32>::min() || raw > std::numeric_limits<qint32>::max()) {
        setError(error, QStringLiteral("%1 cannot be represented by the protocol field").arg(name));
        return false;
    }
    *result = static_cast<qint32>(raw);
    return true;
}

bool toBcd(int value, int minimum, int maximum, const QString &name, quint8 *result, QString *error)
{
    if (value < minimum || value > maximum) {
        setError(error,
                 QStringLiteral("%1 must be between %2 and %3")
                     .arg(name)
                     .arg(minimum)
                     .arg(maximum));
        return false;
    }
    *result = static_cast<quint8>(((value / 10) << 4) | (value % 10));
    return true;
}

bool fromBcd(quint8 raw, int minimum, int maximum, const QString &name, int *result, QString *error)
{
    const int high = (raw >> 4) & 0x0f;
    const int low = raw & 0x0f;
    const int value = high * 10 + low;
    if (high > 9 || low > 9 || value < minimum || value > maximum) {
        setError(error, QStringLiteral("Invalid BCD %1 value 0x%2")
                            .arg(name)
                            .arg(QString::number(raw, 16).rightJustified(2, QLatin1Char('0')).toUpper()));
        return false;
    }
    *result = value;
    return true;
}

QJsonValue jsonNumber(quint32 value)
{
    return QJsonValue(static_cast<double>(value));
}

QJsonObject attitudeFields(const AttitudePayload &p)
{
    return {
        {QStringLiteral("validityFlags"), static_cast<int>(p.validityFlags)},
        {QStringLiteral("headingDegrees"), p.headingDegrees},
        {QStringLiteral("pitchDegrees"), p.pitchDegrees},
        {QStringLiteral("rollDegrees"), p.rollDegrees}
    };
}

QJsonObject navigationFields(const IntegratedNavigationPayload &p)
{
    return {
        {QStringLiteral("validityFlags"), static_cast<int>(p.validityFlags)},
        {QStringLiteral("relativeSpeedKnots"), p.relativeSpeedKnots},
        {QStringLiteral("absoluteSpeedKnots"), p.absoluteSpeedKnots},
        {QStringLiteral("trackAngleDegrees"), p.trackAngleDegrees},
        {QStringLiteral("eastSpeedKnots"), p.eastSpeedKnots},
        {QStringLiteral("northSpeedKnots"), p.northSpeedKnots},
        {QStringLiteral("longitudeDegrees"), p.longitudeDegrees},
        {QStringLiteral("latitudeDegrees"), p.latitudeDegrees},
        {QStringLiteral("verticalSpeedMetersPerSecond"), p.verticalSpeedMetersPerSecond},
        {QStringLiteral("timeBytesHex"), QString::fromLatin1(p.timeBytes.toHex().toUpper())},
        {QStringLiteral("trueWindSpeedMetersPerSecond"), p.trueWindSpeedMetersPerSecond},
        {QStringLiteral("trueWindDirectionDegrees"), p.trueWindDirectionDegrees},
        {QStringLiteral("relativeWindSpeedMetersPerSecond"), p.relativeWindSpeedMetersPerSecond},
        {QStringLiteral("relativeWindDirectionDegrees"), p.relativeWindDirectionDegrees},
        {QStringLiteral("temperatureCelsius"), p.temperatureCelsius},
        {QStringLiteral("relativeHumidityPercent"), static_cast<int>(p.relativeHumidityPercent)},
        {QStringLiteral("atmosphericPressureHpa"), static_cast<int>(p.atmosphericPressureHpa)},
        {QStringLiteral("seaCurrentSpeedRaw"), static_cast<int>(p.seaCurrentSpeedRaw)},
        {QStringLiteral("seaCurrentDirectionRaw"), static_cast<int>(p.seaCurrentDirectionRaw)},
        {QStringLiteral("seaDepthMeters"), p.seaDepthMeters},
        {QStringLiteral("seaStateRaw"), static_cast<int>(p.seaStateRaw)}
    };
}

QJsonObject timeCodeFields(const TimeCodePayload &p)
{
    return {
        {QStringLiteral("second"), p.second},
        {QStringLiteral("minute"), p.minute},
        {QStringLiteral("hour"), p.hour},
        {QStringLiteral("westOfUtcPlus8"), p.westOfUtcPlus8},
        {QStringLiteral("timeZoneOffsetHours"), static_cast<int>(p.timeZoneOffsetHours)},
        {QStringLiteral("day"), p.day},
        {QStringLiteral("month"), p.month},
        {QStringLiteral("year"), p.year}
    };
}

QJsonObject situationFields(const SituationDistributionPayload &p)
{
    QJsonArray sources;
    for (const SituationSource &source : p.sources) {
        sources.append(QJsonObject{
            {QStringLiteral("sourceCode"), static_cast<int>(source.sourceCode)},
            {QStringLiteral("associatedTargetBatch"), static_cast<int>(source.associatedTargetBatch)}
        });
    }
    return {
        {QStringLiteral("timestampMilliseconds"), jsonNumber(p.timestampMilliseconds)},
        {QStringLiteral("fusionBatch"), static_cast<int>(p.fusionBatch)},
        {QStringLiteral("targetType"), static_cast<int>(p.targetType)},
        {QStringLiteral("targetCategory"), static_cast<int>(p.targetCategory)},
        {QStringLiteral("militaryCivilAttribute"), static_cast<int>(p.militaryCivilAttribute)},
        {QStringLiteral("friendFoeAttribute"), static_cast<int>(p.friendFoeAttribute)},
        {QStringLiteral("countryRegion"), static_cast<int>(p.countryRegion)},
        {QStringLiteral("targetRangeRaw"), jsonNumber(p.targetRangeRaw)},
        {QStringLiteral("targetAzimuthRaw"), jsonNumber(p.targetAzimuthRaw)},
        {QStringLiteral("targetElevationRaw"), p.targetElevationRaw},
        {QStringLiteral("longitudeDegrees"), p.longitudeDegrees},
        {QStringLiteral("latitudeDegrees"), p.latitudeDegrees},
        {QStringLiteral("depthHeightRaw"), jsonNumber(p.depthHeightRaw)},
        {QStringLiteral("relativeHeadingRaw"), jsonNumber(p.relativeHeadingRaw)},
        {QStringLiteral("absoluteHeadingRaw"), jsonNumber(p.absoluteHeadingRaw)},
        {QStringLiteral("relativeSpeedRaw"), p.relativeSpeedRaw},
        {QStringLiteral("absoluteSpeedRaw"), p.absoluteSpeedRaw},
        {QStringLiteral("velocityXRaw"), p.velocityXRaw},
        {QStringLiteral("velocityYRaw"), p.velocityYRaw},
        {QStringLiteral("velocityZRaw"), p.velocityZRaw},
        {QStringLiteral("sourceCount"), p.sources.size()},
        {QStringLiteral("sources"), sources}
    };
}

QJsonObject manualGuidanceFields(const ManualTargetGuidancePayload &p)
{
    return {
        {QStringLiteral("timestampRaw"), jsonNumber(p.timestampRaw)},
        {QStringLiteral("targetBatch"), static_cast<int>(p.targetBatch)},
        {QStringLiteral("targetRangeRaw"), jsonNumber(p.targetRangeRaw)},
        {QStringLiteral("targetAzimuthRaw"), jsonNumber(p.targetAzimuthRaw)},
        {QStringLiteral("targetElevationRaw"), p.targetElevationRaw},
        {QStringLiteral("command"), static_cast<int>(p.command)}
    };
}

QJsonObject electromagneticFields(const ElectromagneticStatusPayload &p)
{
    return {
        {QStringLiteral("timestampRaw"), jsonNumber(p.timestampRaw)},
        {QStringLiteral("switchState"), static_cast<int>(p.switchState)},
        {QStringLiteral("spectrumStartRaw"), static_cast<int>(p.spectrumStartRaw)},
        {QStringLiteral("spectrumEndRaw"), static_cast<int>(p.spectrumEndRaw)},
        {QStringLiteral("reserved"), static_cast<int>(p.reserved)}
    };
}

QJsonObject turretFields(const TurretControlPayload &p)
{
    const int azimuth = p.turretState == 0x02
            ? static_cast<int>(static_cast<qint16>(p.azimuthRaw))
            : static_cast<int>(p.azimuthRaw);
    return {
        {QStringLiteral("timestampRaw"), jsonNumber(p.timestampRaw)},
        {QStringLiteral("visibleLightState"), static_cast<int>(p.visibleLightState)},
        {QStringLiteral("midWaveInfraredState"), static_cast<int>(p.midWaveInfraredState)},
        {QStringLiteral("laserNirState"), static_cast<int>(p.laserNirState)},
        {QStringLiteral("azimuthRaw"), azimuth},
        {QStringLiteral("elevationHundredths"), p.elevationHundredths},
        {QStringLiteral("trackingState"), static_cast<int>(p.trackingState)},
        {QStringLiteral("turretState"), static_cast<int>(p.turretState)},
        {QStringLiteral("imagingLensControl"), static_cast<int>(p.imagingLensControl)},
        {QStringLiteral("reserved"), jsonNumber(p.reserved)}
    };
}

QJsonObject laserFields(const LaserNirControlPayload &p)
{
    return {
        {QStringLiteral("timestampRaw"), jsonNumber(p.timestampRaw)},
        {QStringLiteral("laserEnabled"), static_cast<int>(p.laserEnabled)},
        {QStringLiteral("laserIntensity"), static_cast<int>(p.laserIntensity)},
        {QStringLiteral("illuminationLensControl"), static_cast<int>(p.illuminationLensControl)},
        {QStringLiteral("workingDistanceMeters"), jsonNumber(p.workingDistanceMeters)},
        {QStringLiteral("imagingDepthMeters"), static_cast<int>(p.imagingDepthMeters)},
        {QStringLiteral("imagingGain"), static_cast<int>(p.imagingGain)},
        {QStringLiteral("imageProcessing"), static_cast<int>(p.imageProcessing)},
        {QStringLiteral("reserved"), jsonNumber(p.reserved)}
    };
}

QJsonObject deviceFields(const DeviceStatusPayload &p)
{
    return {
        {QStringLiteral("timestampRaw"), jsonNumber(p.timestampRaw)},
        {QStringLiteral("visibleLightState"), static_cast<int>(p.visibleLightState)},
        {QStringLiteral("midWaveInfraredState"), static_cast<int>(p.midWaveInfraredState)},
        {QStringLiteral("laserNirState"), static_cast<int>(p.laserNirState)},
        {QStringLiteral("azimuthHundredths"), static_cast<int>(p.azimuthHundredths)},
        {QStringLiteral("elevationHundredths"), p.elevationHundredths},
        {QStringLiteral("trackingState"), static_cast<int>(p.trackingState)},
        {QStringLiteral("turretState"), static_cast<int>(p.turretState)}
    };
}

QJsonObject targetFields(const ElectroOpticalTargetPayload &p)
{
    return {
        {QStringLiteral("timestampTenthsOfMillisecond"), jsonNumber(p.timestampTenthsOfMillisecond)},
        {QStringLiteral("reportType"), static_cast<int>(p.reportType)},
        {QStringLiteral("targetBatch"), static_cast<int>(p.targetBatch)},
        {QStringLiteral("targetType"), static_cast<int>(p.targetType)},
        {QStringLiteral("targetCategory"), static_cast<int>(p.targetCategory)},
        {QStringLiteral("targetRangeMeters"), jsonNumber(p.targetRangeMeters)},
        {QStringLiteral("targetAzimuthHundredths"), jsonNumber(p.targetAzimuthHundredths)},
        {QStringLiteral("targetElevationHundredths"), p.targetElevationHundredths},
        {QStringLiteral("reservedHex"), QString::fromLatin1(p.reserved.toHex().toUpper())}
    };
}
}

QByteArray PacketCodec::encodeRaw(quint8 frameNumber,
                                  InfoId infoId,
                                  const QByteArray &payload,
                                  QString *error)
{
    const int unitLength = HeaderSize + payload.size();
    if (unitLength > kMaximumUdpPayload) {
        setError(error, QStringLiteral("Packet is too large for one UDP datagram: %1 bytes").arg(unitLength));
        return {};
    }
    if (!validateUnitLength(infoId, unitLength, error)) {
        return {};
    }

    QByteArray datagram;
    datagram.reserve(unitLength);
    appendU8(datagram, frameNumber);
    appendU8(datagram, rawInfoId(infoId));
    appendU16(datagram, static_cast<quint16>(unitLength));
    datagram.append(payload);
    return datagram;
}

QByteArray PacketCodec::encodeAttitude(quint8 frameNumber, const AttitudePayload &payload, QString *error)
{
    qint32 heading = 0;
    qint32 pitch = 0;
    qint32 roll = 0;
    if (!encodeSigned32(payload.headingDegrees, kAttitudeHeadingLsb, 0.0,
                        360.0 - kAttitudeHeadingLsb, QStringLiteral("Heading"), &heading, error)
        || !encodeSigned32(payload.pitchDegrees, kAttitudeAngleLsb, -30.0, 30.0,
                        QStringLiteral("Pitch"), &pitch, error)
        || !encodeSigned32(payload.rollDegrees, kAttitudeAngleLsb, -45.0, 45.0,
                           QStringLiteral("Roll"), &roll, error)) {
        return {};
    }

    QByteArray body;
    body.reserve(14);
    appendU16(body, payload.validityFlags);
    appendI32(body, heading);
    appendI32(body, pitch);
    appendI32(body, roll);
    return encodeRaw(frameNumber, InfoId::Attitude, body, error);
}

QByteArray PacketCodec::encodeIntegratedNavigation(quint8 frameNumber,
                                                   const IntegratedNavigationPayload &payload,
                                                   QString *error)
{
    if (payload.timeBytes.size() != 8) {
        setError(error, QStringLiteral("Integrated navigation timeBytes must contain exactly 8 bytes"));
        return {};
    }

    qint16 relativeSpeed = 0;
    qint16 absoluteSpeed = 0;
    quint16 trackAngle = 0;
    qint16 eastSpeed = 0;
    qint16 northSpeed = 0;
    qint32 longitude = 0;
    qint32 latitude = 0;
    qint16 verticalSpeed = 0;
    qint16 trueWindSpeed = 0;
    qint16 trueWindDirection = 0;
    qint16 relativeWindSpeed = 0;
    qint16 relativeWindDirection = 0;
    qint16 temperature = 0;
    quint32 seaDepth = 0;

    if (!encodeSigned16(payload.relativeSpeedKnots, 0.01, -12.0, 40.0,
                        QStringLiteral("Relative speed"), &relativeSpeed, error)
        || !encodeSigned16(payload.absoluteSpeedKnots, 0.01, -40.0, 40.0,
                           QStringLiteral("Absolute speed"), &absoluteSpeed, error)
        || !encodeUnsigned16(payload.trackAngleDegrees, kNavigationTrackLsb, 0.0,
                             360.0 - kNavigationTrackLsb, QStringLiteral("Track angle"), &trackAngle, error)
        || !encodeSigned16(payload.eastSpeedKnots, 0.01, -40.0, 40.0,
                           QStringLiteral("East speed"), &eastSpeed, error)
        || !encodeSigned16(payload.northSpeedKnots, 0.01, -40.0, 40.0,
                           QStringLiteral("North speed"), &northSpeed, error)
        || !encodeSigned32(payload.longitudeDegrees, kGeoAngleLsb, -180.0,
                           180.0 - kGeoAngleLsb, QStringLiteral("Longitude"), &longitude, error)
        || !encodeSigned32(payload.latitudeDegrees, kGeoAngleLsb, -90.0, 90.0,
                           QStringLiteral("Latitude"), &latitude, error)
        || !encodeSigned16(payload.verticalSpeedMetersPerSecond, 0.01, -10.0, 10.0,
                           QStringLiteral("Vertical speed"), &verticalSpeed, error)
        || !encodeSigned16(payload.trueWindSpeedMetersPerSecond, 0.01, -10.0, 10.0,
                           QStringLiteral("True wind speed"), &trueWindSpeed, error)
        || !encodeSigned16(payload.trueWindDirectionDegrees, kNavigationDirectionLsb, 0.0,
                           360.0 - kNavigationDirectionLsb, QStringLiteral("True wind direction"),
                           &trueWindDirection, error)
        || !encodeSigned16(payload.relativeWindSpeedMetersPerSecond, 0.01, 0.0, 70.0,
                           QStringLiteral("Relative wind speed"), &relativeWindSpeed, error)
        || !encodeSigned16(payload.relativeWindDirectionDegrees, kNavigationDirectionLsb, 0.0,
                           360.0 - kNavigationDirectionLsb, QStringLiteral("Relative wind direction"),
                           &relativeWindDirection, error)
        || !encodeSigned16(payload.temperatureCelsius, 0.1, -40.0, 50.0,
                           QStringLiteral("Temperature"), &temperature, error)) {
        return {};
    }

    if (!std::isfinite(payload.seaDepthMeters) || payload.seaDepthMeters < 0.0
        || payload.seaDepthMeters > 2000.0) {
        setError(error, QStringLiteral("Sea depth must be between 0 and 2000 meters"));
        return {};
    }
    seaDepth = static_cast<quint32>(std::llround(payload.seaDepthMeters / 0.01));

    QByteArray body;
    body.reserve(54);
    appendU16(body, payload.validityFlags);
    appendI16(body, relativeSpeed);
    appendI16(body, absoluteSpeed);
    appendU16(body, trackAngle);
    appendI16(body, eastSpeed);
    appendI16(body, northSpeed);
    appendI32(body, longitude);
    appendI32(body, latitude);
    appendI16(body, verticalSpeed);
    body.append(payload.timeBytes);
    appendI16(body, trueWindSpeed);
    appendI16(body, trueWindDirection);
    appendI16(body, relativeWindSpeed);
    appendI16(body, relativeWindDirection);
    appendI16(body, temperature);
    appendU16(body, payload.relativeHumidityPercent);
    appendU16(body, payload.atmosphericPressureHpa);
    appendU16(body, payload.seaCurrentSpeedRaw);
    appendU16(body, payload.seaCurrentDirectionRaw);
    appendU32(body, seaDepth);
    appendU16(body, payload.seaStateRaw);
    return encodeRaw(frameNumber, InfoId::IntegratedNavigation, body, error);
}

QByteArray PacketCodec::encodeTimeCode(quint8 frameNumber, const TimeCodePayload &payload, QString *error)
{
    quint8 second = 0;
    quint8 minute = 0;
    quint8 hour = 0;
    quint8 day = 0;
    quint8 month = 0;
    quint8 yearLow = 0;
    quint8 yearHigh = 0;
    if (!toBcd(payload.second, 0, 59, QStringLiteral("Second"), &second, error)
        || !toBcd(payload.minute, 0, 59, QStringLiteral("Minute"), &minute, error)
        || !toBcd(payload.hour, 0, 23, QStringLiteral("Hour"), &hour, error)
        || !toBcd(payload.day, 1, 31, QStringLiteral("Day"), &day, error)
        || !toBcd(payload.month, 1, 12, QStringLiteral("Month"), &month, error)
        || !toBcd(payload.year % 100, 0, 99, QStringLiteral("Year low"), &yearLow, error)
        || !toBcd(payload.year / 100, 0, 99, QStringLiteral("Year high"), &yearHigh, error)) {
        return {};
    }
    if (payload.year < 0 || payload.year > 9999) {
        setError(error, QStringLiteral("Year must be between 0 and 9999"));
        return {};
    }
    if (payload.timeZoneOffsetHours > 20 || (!payload.westOfUtcPlus8 && payload.timeZoneOffsetHours > 4)) {
        setError(error, QStringLiteral("Timezone correction is outside the protocol range"));
        return {};
    }

    QByteArray body;
    body.reserve(8);
    appendU8(body, second);
    appendU8(body, minute);
    appendU8(body, hour);
    appendU8(body, static_cast<quint8>((payload.westOfUtcPlus8 ? 0x80 : 0x00)
                                      | payload.timeZoneOffsetHours));
    appendU8(body, day);
    appendU8(body, month);
    appendU8(body, yearLow);
    appendU8(body, yearHigh);
    return encodeRaw(frameNumber, InfoId::TimeCode, body, error);
}

QByteArray PacketCodec::encodeSituationDistribution(quint8 frameNumber,
                                                    const SituationDistributionPayload &payload,
                                                    QString *error)
{
    if (payload.sources.size() > std::numeric_limits<quint16>::max()) {
        setError(error, QStringLiteral("Too many situation sources"));
        return {};
    }
    qint32 longitude = 0;
    qint32 latitude = 0;
    if (!encodeSigned32(payload.longitudeDegrees, kGeoAngleLsb, -180.0,
                        180.0 - kGeoAngleLsb, QStringLiteral("Target longitude"), &longitude, error)
        || !encodeSigned32(payload.latitudeDegrees, kGeoAngleLsb, -90.0, 90.0,
                           QStringLiteral("Target latitude"), &latitude, error)) {
        return {};
    }

    QByteArray body;
    body.reserve(74 + payload.sources.size() * 4);
    appendU32(body, payload.timestampMilliseconds);
    appendU16(body, payload.fusionBatch);
    appendU8(body, payload.targetType);
    appendU8(body, payload.targetCategory);
    appendU8(body, payload.militaryCivilAttribute);
    appendU8(body, payload.friendFoeAttribute);
    appendU16(body, payload.countryRegion);
    appendU32(body, payload.targetRangeRaw);
    appendU32(body, payload.targetAzimuthRaw);
    appendI32(body, payload.targetElevationRaw);
    appendI32(body, longitude);
    appendI32(body, latitude);
    appendU32(body, payload.depthHeightRaw);
    appendU32(body, payload.relativeHeadingRaw);
    appendU32(body, payload.absoluteHeadingRaw);
    appendI32(body, payload.relativeSpeedRaw);
    appendI32(body, payload.absoluteSpeedRaw);
    appendI32(body, payload.velocityXRaw);
    appendI32(body, payload.velocityYRaw);
    appendI32(body, payload.velocityZRaw);
    appendU32(body, payload.reserved1);
    appendU32(body, payload.reserved2);
    appendU16(body, static_cast<quint16>(payload.sources.size()));
    for (const SituationSource &source : payload.sources) {
        appendU16(body, source.sourceCode);
        appendU16(body, source.associatedTargetBatch);
    }
    return encodeRaw(frameNumber, InfoId::SituationDistribution, body, error);
}

QByteArray PacketCodec::encodeManualTargetGuidance(quint8 frameNumber,
                                                   const ManualTargetGuidancePayload &payload,
                                                   QString *error)
{
    QByteArray body;
    body.reserve(20);
    appendU32(body, payload.timestampRaw);
    appendU16(body, payload.targetBatch);
    appendU32(body, payload.targetRangeRaw);
    appendU32(body, payload.targetAzimuthRaw);
    appendI32(body, payload.targetElevationRaw);
    appendU16(body, payload.command);
    return encodeRaw(frameNumber, InfoId::ManualTargetGuidance, body, error);
}

QByteArray PacketCodec::encodeElectromagneticStatus(quint8 frameNumber,
                                                    const ElectromagneticStatusPayload &payload,
                                                    QString *error)
{
    QByteArray body;
    body.reserve(10);
    appendU32(body, payload.timestampRaw);
    appendU8(body, payload.switchState);
    appendU16(body, payload.spectrumStartRaw);
    appendU16(body, payload.spectrumEndRaw);
    appendU8(body, payload.reserved);
    return encodeRaw(frameNumber, InfoId::ElectromagneticStatus, body, error);
}

QByteArray PacketCodec::encodeTurretControl(quint8 frameNumber,
                                           const TurretControlPayload &payload,
                                           QString *error)
{
    QByteArray body;
    body.reserve(18);
    appendU32(body, payload.timestampRaw);
    appendU8(body, payload.visibleLightState);
    appendU8(body, payload.midWaveInfraredState);
    appendU8(body, payload.laserNirState);
    appendU16(body, payload.azimuthRaw);
    appendI16(body, payload.elevationHundredths);
    appendU8(body, payload.trackingState);
    appendU8(body, payload.turretState);
    appendU8(body, payload.imagingLensControl);
    appendU32(body, payload.reserved);
    return encodeRaw(frameNumber, InfoId::TurretControl, body, error);
}

QByteArray PacketCodec::encodeLaserNirControl(quint8 frameNumber,
                                             const LaserNirControlPayload &payload,
                                             QString *error)
{
    QByteArray body;
    body.reserve(20);
    appendU32(body, payload.timestampRaw);
    appendU8(body, payload.laserEnabled);
    appendU16(body, payload.laserIntensity);
    appendU8(body, payload.illuminationLensControl);
    appendU32(body, payload.workingDistanceMeters);
    appendU16(body, payload.imagingDepthMeters);
    appendU8(body, payload.imagingGain);
    appendU8(body, payload.imageProcessing);
    appendU32(body, payload.reserved);
    return encodeRaw(frameNumber, InfoId::LaserNirControl, body, error);
}

QByteArray PacketCodec::encodeDeviceStatus(quint8 frameNumber,
                                          const DeviceStatusPayload &payload,
                                          QString *error)
{
    QByteArray body;
    body.reserve(14);
    appendU32(body, payload.timestampRaw);
    appendU8(body, payload.visibleLightState);
    appendU8(body, payload.midWaveInfraredState);
    appendU8(body, payload.laserNirState);
    appendU16(body, payload.azimuthHundredths);
    appendI16(body, payload.elevationHundredths);
    appendU8(body, payload.trackingState);
    appendU16(body, payload.turretState);
    return encodeRaw(frameNumber, InfoId::DeviceStatus, body, error);
}

QByteArray PacketCodec::encodeElectroOpticalTarget(quint8 frameNumber,
                                                  const ElectroOpticalTargetPayload &payload,
                                                  QString *error)
{
    if (payload.reserved.size() != 8) {
        setError(error, QStringLiteral("Electro-optical target reserved field must contain exactly 8 bytes"));
        return {};
    }
    QByteArray body;
    body.reserve(29);
    appendU32(body, payload.timestampTenthsOfMillisecond);
    appendU8(body, payload.reportType);
    appendU16(body, payload.targetBatch);
    appendU8(body, payload.targetType);
    appendU8(body, payload.targetCategory);
    appendU32(body, payload.targetRangeMeters);
    appendU32(body, payload.targetAzimuthHundredths);
    appendI32(body, payload.targetElevationHundredths);
    body.append(payload.reserved);
    return encodeRaw(frameNumber, InfoId::ElectroOpticalTarget, body, error);
}

bool PacketCodec::decode(const QByteArray &datagram, DecodedPacket *packet, QString *error)
{
    if (packet == nullptr) {
        setError(error, QStringLiteral("Decoded packet output cannot be null"));
        return false;
    }
    if (datagram.size() < HeaderSize) {
        setError(error, QStringLiteral("Datagram too short: %1 bytes").arg(datagram.size()));
        return false;
    }

    ByteReader header(datagram.left(HeaderSize));
    quint8 frameNumber = 0;
    quint8 rawId = 0;
    quint16 unitLength = 0;
    if (!header.u8(&frameNumber) || !header.u8(&rawId) || !header.u16(&unitLength)) {
        setError(error, QStringLiteral("Could not read common packet header"));
        return false;
    }
    if (datagram.size() != unitLength) {
        setError(error,
                 QStringLiteral("Information unit length mismatch: header=%1 actual=%2")
                     .arg(unitLength)
                     .arg(datagram.size()));
        return false;
    }

    const InfoId infoId = infoIdFromRaw(rawId);
    if (!validateUnitLength(infoId, unitLength, error)) {
        return false;
    }

    packet->frameNumber = frameNumber;
    packet->infoId = infoId;
    packet->rawInfoId = rawId;
    packet->infoName = infoIdLabel(rawId);
    packet->datagramLength = datagram.size();
    packet->unitLength = unitLength;
    packet->payloadLength = datagram.size() - HeaderSize;
    packet->rawPayload = datagram.mid(HeaderSize);
    packet->receivedAt = QDateTime::currentDateTime();
    packet->fields = {};

    switch (infoId) {
    case InfoId::Attitude: {
        AttitudePayload value;
        if (!parseAttitude(*packet, &value, error)) return false;
        packet->fields = attitudeFields(value);
        break;
    }
    case InfoId::IntegratedNavigation: {
        IntegratedNavigationPayload value;
        if (!parseIntegratedNavigation(*packet, &value, error)) return false;
        packet->fields = navigationFields(value);
        break;
    }
    case InfoId::TimeCode: {
        TimeCodePayload value;
        if (!parseTimeCode(*packet, &value, error)) return false;
        packet->fields = timeCodeFields(value);
        break;
    }
    case InfoId::SituationDistribution: {
        SituationDistributionPayload value;
        if (!parseSituationDistribution(*packet, &value, error)) return false;
        packet->fields = situationFields(value);
        break;
    }
    case InfoId::ManualTargetGuidance: {
        ManualTargetGuidancePayload value;
        if (!parseManualTargetGuidance(*packet, &value, error)) return false;
        packet->fields = manualGuidanceFields(value);
        break;
    }
    case InfoId::ElectromagneticStatus: {
        ElectromagneticStatusPayload value;
        if (!parseElectromagneticStatus(*packet, &value, error)) return false;
        packet->fields = electromagneticFields(value);
        break;
    }
    case InfoId::TurretControl: {
        TurretControlPayload value;
        if (!parseTurretControl(*packet, &value, error)) return false;
        packet->fields = turretFields(value);
        break;
    }
    case InfoId::LaserNirControl: {
        LaserNirControlPayload value;
        if (!parseLaserNirControl(*packet, &value, error)) return false;
        packet->fields = laserFields(value);
        break;
    }
    case InfoId::DeviceStatus: {
        DeviceStatusPayload value;
        if (!parseDeviceStatus(*packet, &value, error)) return false;
        packet->fields = deviceFields(value);
        break;
    }
    case InfoId::ElectroOpticalTarget: {
        ElectroOpticalTargetPayload value;
        if (!parseElectroOpticalTarget(*packet, &value, error)) return false;
        packet->fields = targetFields(value);
        break;
    }
    case InfoId::Unknown:
        packet->fields = {
            {QStringLiteral("rawPayloadHex"), QString::fromLatin1(packet->rawPayload.toHex().toUpper())}
        };
        break;
    }
    return true;
}

bool PacketCodec::parseAttitude(const DecodedPacket &packet, AttitudePayload *payload, QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::Attitude, error)) return false;
    ByteReader reader(packet.rawPayload);
    qint32 heading = 0;
    qint32 pitch = 0;
    qint32 roll = 0;
    if (!reader.u16(&payload->validityFlags) || !reader.i32(&heading)
        || !reader.i32(&pitch) || !reader.i32(&roll) || !readerFinished(reader, error)) {
        setError(error, error != nullptr && !error->isEmpty() ? *error : QStringLiteral("Malformed attitude payload"));
        return false;
    }
    payload->headingDegrees = static_cast<double>(heading) * kAttitudeHeadingLsb;
    payload->pitchDegrees = static_cast<double>(pitch) * kAttitudeAngleLsb;
    payload->rollDegrees = static_cast<double>(roll) * kAttitudeAngleLsb;
    return true;
}

bool PacketCodec::parseIntegratedNavigation(const DecodedPacket &packet,
                                            IntegratedNavigationPayload *payload,
                                            QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::IntegratedNavigation, error)) return false;
    ByteReader reader(packet.rawPayload);
    qint16 relativeSpeed = 0;
    qint16 absoluteSpeed = 0;
    quint16 trackAngle = 0;
    qint16 eastSpeed = 0;
    qint16 northSpeed = 0;
    qint32 longitude = 0;
    qint32 latitude = 0;
    qint16 verticalSpeed = 0;
    qint16 trueWindSpeed = 0;
    qint16 trueWindDirection = 0;
    qint16 relativeWindSpeed = 0;
    qint16 relativeWindDirection = 0;
    qint16 temperature = 0;
    quint32 seaDepth = 0;
    if (!reader.u16(&payload->validityFlags)
        || !reader.i16(&relativeSpeed) || !reader.i16(&absoluteSpeed) || !reader.u16(&trackAngle)
        || !reader.i16(&eastSpeed) || !reader.i16(&northSpeed)
        || !reader.i32(&longitude) || !reader.i32(&latitude) || !reader.i16(&verticalSpeed)
        || !reader.byteArray(8, &payload->timeBytes)
        || !reader.i16(&trueWindSpeed) || !reader.i16(&trueWindDirection)
        || !reader.i16(&relativeWindSpeed) || !reader.i16(&relativeWindDirection)
        || !reader.i16(&temperature) || !reader.u16(&payload->relativeHumidityPercent)
        || !reader.u16(&payload->atmosphericPressureHpa) || !reader.u16(&payload->seaCurrentSpeedRaw)
        || !reader.u16(&payload->seaCurrentDirectionRaw) || !reader.u32(&seaDepth)
        || !reader.u16(&payload->seaStateRaw) || !readerFinished(reader, error)) {
        setError(error, error != nullptr && !error->isEmpty() ? *error : QStringLiteral("Malformed navigation payload"));
        return false;
    }
    payload->relativeSpeedKnots = relativeSpeed * 0.01;
    payload->absoluteSpeedKnots = absoluteSpeed * 0.01;
    payload->trackAngleDegrees = trackAngle * kNavigationTrackLsb;
    payload->eastSpeedKnots = eastSpeed * 0.01;
    payload->northSpeedKnots = northSpeed * 0.01;
    payload->longitudeDegrees = longitude * kGeoAngleLsb;
    payload->latitudeDegrees = latitude * kGeoAngleLsb;
    payload->verticalSpeedMetersPerSecond = verticalSpeed * 0.01;
    payload->trueWindSpeedMetersPerSecond = trueWindSpeed * 0.01;
    payload->trueWindDirectionDegrees = trueWindDirection * kNavigationDirectionLsb;
    payload->relativeWindSpeedMetersPerSecond = relativeWindSpeed * 0.01;
    payload->relativeWindDirectionDegrees = relativeWindDirection * kNavigationDirectionLsb;
    payload->temperatureCelsius = temperature * 0.1;
    payload->seaDepthMeters = seaDepth * 0.01;
    return true;
}

bool PacketCodec::parseTimeCode(const DecodedPacket &packet, TimeCodePayload *payload, QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::TimeCode, error)) return false;
    ByteReader reader(packet.rawPayload);
    quint8 second = 0;
    quint8 minute = 0;
    quint8 hour = 0;
    quint8 correction = 0;
    quint8 day = 0;
    quint8 month = 0;
    quint8 yearLow = 0;
    quint8 yearHigh = 0;
    int low = 0;
    int high = 0;
    if (!reader.u8(&second) || !reader.u8(&minute) || !reader.u8(&hour) || !reader.u8(&correction)
        || !reader.u8(&day) || !reader.u8(&month) || !reader.u8(&yearLow) || !reader.u8(&yearHigh)
        || !readerFinished(reader, error)
        || !fromBcd(second, 0, 59, QStringLiteral("second"), &payload->second, error)
        || !fromBcd(minute, 0, 59, QStringLiteral("minute"), &payload->minute, error)
        || !fromBcd(hour, 0, 23, QStringLiteral("hour"), &payload->hour, error)
        || !fromBcd(day, 1, 31, QStringLiteral("day"), &payload->day, error)
        || !fromBcd(month, 1, 12, QStringLiteral("month"), &payload->month, error)
        || !fromBcd(yearLow, 0, 99, QStringLiteral("year low"), &low, error)
        || !fromBcd(yearHigh, 0, 99, QStringLiteral("year high"), &high, error)) {
        if (error != nullptr && error->isEmpty()) *error = QStringLiteral("Malformed time-code payload");
        return false;
    }
    payload->westOfUtcPlus8 = (correction & 0x80) != 0;
    payload->timeZoneOffsetHours = correction & 0x7f;
    if (payload->timeZoneOffsetHours > 20 || (!payload->westOfUtcPlus8 && payload->timeZoneOffsetHours > 4)) {
        setError(error, QStringLiteral("Invalid timezone correction 0x%1")
                            .arg(QString::number(correction, 16).rightJustified(2, QLatin1Char('0')).toUpper()));
        return false;
    }
    payload->year = high * 100 + low;
    return true;
}

bool PacketCodec::parseSituationDistribution(const DecodedPacket &packet,
                                             SituationDistributionPayload *payload,
                                             QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::SituationDistribution, error)) return false;
    ByteReader reader(packet.rawPayload);
    qint32 longitude = 0;
    qint32 latitude = 0;
    quint16 sourceCount = 0;
    if (!reader.u32(&payload->timestampMilliseconds) || !reader.u16(&payload->fusionBatch)
        || !reader.u8(&payload->targetType) || !reader.u8(&payload->targetCategory)
        || !reader.u8(&payload->militaryCivilAttribute) || !reader.u8(&payload->friendFoeAttribute)
        || !reader.u16(&payload->countryRegion) || !reader.u32(&payload->targetRangeRaw)
        || !reader.u32(&payload->targetAzimuthRaw) || !reader.i32(&payload->targetElevationRaw)
        || !reader.i32(&longitude) || !reader.i32(&latitude) || !reader.u32(&payload->depthHeightRaw)
        || !reader.u32(&payload->relativeHeadingRaw) || !reader.u32(&payload->absoluteHeadingRaw)
        || !reader.i32(&payload->relativeSpeedRaw) || !reader.i32(&payload->absoluteSpeedRaw)
        || !reader.i32(&payload->velocityXRaw) || !reader.i32(&payload->velocityYRaw)
        || !reader.i32(&payload->velocityZRaw) || !reader.u32(&payload->reserved1)
        || !reader.u32(&payload->reserved2) || !reader.u16(&sourceCount)) {
        setError(error, QStringLiteral("Malformed situation-distribution fixed payload"));
        return false;
    }
    if (reader.remaining() != static_cast<int>(sourceCount) * 4) {
        setError(error,
                 QStringLiteral("Situation source count mismatch: count=%1 remainingBytes=%2")
                     .arg(sourceCount)
                     .arg(reader.remaining()));
        return false;
    }
    payload->sources.clear();
    payload->sources.reserve(sourceCount);
    for (quint16 i = 0; i < sourceCount; ++i) {
        SituationSource source;
        if (!reader.u16(&source.sourceCode) || !reader.u16(&source.associatedTargetBatch)) {
            setError(error, QStringLiteral("Malformed situation source entry"));
            return false;
        }
        payload->sources.append(source);
    }
    payload->longitudeDegrees = longitude * kGeoAngleLsb;
    payload->latitudeDegrees = latitude * kGeoAngleLsb;
    return readerFinished(reader, error);
}

bool PacketCodec::parseManualTargetGuidance(const DecodedPacket &packet,
                                            ManualTargetGuidancePayload *payload,
                                            QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::ManualTargetGuidance, error)) return false;
    ByteReader reader(packet.rawPayload);
    if (!reader.u32(&payload->timestampRaw) || !reader.u16(&payload->targetBatch)
        || !reader.u32(&payload->targetRangeRaw) || !reader.u32(&payload->targetAzimuthRaw)
        || !reader.i32(&payload->targetElevationRaw) || !reader.u16(&payload->command)) {
        setError(error, QStringLiteral("Malformed manual-target-guidance payload"));
        return false;
    }
    return readerFinished(reader, error);
}

bool PacketCodec::parseElectromagneticStatus(const DecodedPacket &packet,
                                             ElectromagneticStatusPayload *payload,
                                             QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::ElectromagneticStatus, error)) return false;
    ByteReader reader(packet.rawPayload);
    if (!reader.u32(&payload->timestampRaw) || !reader.u8(&payload->switchState)
        || !reader.u16(&payload->spectrumStartRaw) || !reader.u16(&payload->spectrumEndRaw)
        || !reader.u8(&payload->reserved)) {
        setError(error, QStringLiteral("Malformed electromagnetic-status payload"));
        return false;
    }
    return readerFinished(reader, error);
}

bool PacketCodec::parseTurretControl(const DecodedPacket &packet, TurretControlPayload *payload, QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::TurretControl, error)) return false;
    ByteReader reader(packet.rawPayload);
    if (!reader.u32(&payload->timestampRaw) || !reader.u8(&payload->visibleLightState)
        || !reader.u8(&payload->midWaveInfraredState) || !reader.u8(&payload->laserNirState)
        || !reader.u16(&payload->azimuthRaw) || !reader.i16(&payload->elevationHundredths)
        || !reader.u8(&payload->trackingState) || !reader.u8(&payload->turretState)
        || !reader.u8(&payload->imagingLensControl) || !reader.u32(&payload->reserved)) {
        setError(error, QStringLiteral("Malformed turret-control payload"));
        return false;
    }
    return readerFinished(reader, error);
}

bool PacketCodec::parseLaserNirControl(const DecodedPacket &packet,
                                      LaserNirControlPayload *payload,
                                      QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::LaserNirControl, error)) return false;
    ByteReader reader(packet.rawPayload);
    if (!reader.u32(&payload->timestampRaw) || !reader.u8(&payload->laserEnabled)
        || !reader.u16(&payload->laserIntensity) || !reader.u8(&payload->illuminationLensControl)
        || !reader.u32(&payload->workingDistanceMeters) || !reader.u16(&payload->imagingDepthMeters)
        || !reader.u8(&payload->imagingGain) || !reader.u8(&payload->imageProcessing)
        || !reader.u32(&payload->reserved)) {
        setError(error, QStringLiteral("Malformed laser/NIR-control payload"));
        return false;
    }
    return readerFinished(reader, error);
}

bool PacketCodec::parseDeviceStatus(const DecodedPacket &packet, DeviceStatusPayload *payload, QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::DeviceStatus, error)) return false;
    ByteReader reader(packet.rawPayload);
    if (!reader.u32(&payload->timestampRaw) || !reader.u8(&payload->visibleLightState)
        || !reader.u8(&payload->midWaveInfraredState) || !reader.u8(&payload->laserNirState)
        || !reader.u16(&payload->azimuthHundredths) || !reader.i16(&payload->elevationHundredths)
        || !reader.u8(&payload->trackingState) || !reader.u16(&payload->turretState)) {
        setError(error, QStringLiteral("Malformed device-status payload"));
        return false;
    }
    return readerFinished(reader, error);
}

bool PacketCodec::parseElectroOpticalTarget(const DecodedPacket &packet,
                                           ElectroOpticalTargetPayload *payload,
                                           QString *error)
{
    if (payload == nullptr || !checkPacket(packet, InfoId::ElectroOpticalTarget, error)) return false;
    ByteReader reader(packet.rawPayload);
    if (!reader.u32(&payload->timestampTenthsOfMillisecond) || !reader.u8(&payload->reportType)
        || !reader.u16(&payload->targetBatch) || !reader.u8(&payload->targetType)
        || !reader.u8(&payload->targetCategory) || !reader.u32(&payload->targetRangeMeters)
        || !reader.u32(&payload->targetAzimuthHundredths) || !reader.i32(&payload->targetElevationHundredths)
        || !reader.byteArray(8, &payload->reserved)) {
        setError(error, QStringLiteral("Malformed electro-optical-target payload"));
        return false;
    }
    return readerFinished(reader, error);
}

bool PacketCodec::validateUnitLength(InfoId infoId, int unitLength, QString *error)
{
    if (unitLength < HeaderSize) {
        setError(error, QStringLiteral("Information unit length cannot be smaller than %1").arg(HeaderSize));
        return false;
    }
    if (infoId == InfoId::SituationDistribution) {
        if (unitLength < 78 || ((unitLength - 78) % 4) != 0) {
            setError(error,
                     QStringLiteral("Situation distribution length must be 78 + 4*N bytes; got %1")
                         .arg(unitLength));
            return false;
        }
        return true;
    }
    const int expected = expectedUnitLength(infoId);
    if (expected == VariableUnitLength || expected == unitLength) {
        return true;
    }
    setError(error,
             QStringLiteral("%1 unit length mismatch: expected=%2 actual=%3")
                 .arg(infoIdLabel(infoId))
                 .arg(expected)
                 .arg(unitLength));
    return false;
}

QString PacketCodec::infoIdLabel(InfoId infoId)
{
    switch (infoId) {
    case InfoId::Attitude: return QStringLiteral("B0 Attitude (0xB0)");
    case InfoId::IntegratedNavigation: return QStringLiteral("B1 Integrated Navigation (0xB1)");
    case InfoId::TimeCode: return QStringLiteral("B3 Time Code (0xB3)");
    case InfoId::SituationDistribution: return QStringLiteral("G0 Situation Distribution (0x90)");
    case InfoId::ManualTargetGuidance: return QStringLiteral("G1 Manual Target Guidance (0x91)");
    case InfoId::ElectromagneticStatus: return QStringLiteral("G2 Electromagnetic Status (0x92)");
    case InfoId::TurretControl: return QStringLiteral("G3 Turret Control (0x93)");
    case InfoId::LaserNirControl: return QStringLiteral("G4 Laser/NIR Control (0x94)");
    case InfoId::DeviceStatus: return QStringLiteral("G5 Device Status (0x95)");
    case InfoId::ElectroOpticalTarget: return QStringLiteral("G6 Electro-optical Target (0x96)");
    case InfoId::Unknown: return QStringLiteral("Unknown information ID (0xFF)");
    }
    return QStringLiteral("Unknown information ID");
}

QString PacketCodec::infoIdLabel(quint8 rawId)
{
    const InfoId infoId = infoIdFromRaw(rawId);
    if (infoId != InfoId::Unknown || rawId == rawInfoId(InfoId::Unknown)) {
        return infoIdLabel(infoId);
    }
    return QStringLiteral("Unknown information ID (0x%1)")
        .arg(QString::number(rawId, 16).rightJustified(2, QLatin1Char('0')).toUpper());
}

InfoId PacketCodec::infoIdFromRaw(quint8 rawId)
{
    switch (rawId) {
    case 0xb0: return InfoId::Attitude;
    case 0xb1: return InfoId::IntegratedNavigation;
    case 0xb3: return InfoId::TimeCode;
    case 0x90: return InfoId::SituationDistribution;
    case 0x91: return InfoId::ManualTargetGuidance;
    case 0x92: return InfoId::ElectromagneticStatus;
    case 0x93: return InfoId::TurretControl;
    case 0x94: return InfoId::LaserNirControl;
    case 0x95: return InfoId::DeviceStatus;
    case 0x96: return InfoId::ElectroOpticalTarget;
    default: return InfoId::Unknown;
    }
}

quint8 PacketCodec::rawInfoId(InfoId infoId)
{
    return static_cast<quint8>(infoId);
}

int PacketCodec::expectedUnitLength(InfoId infoId)
{
    switch (infoId) {
    case InfoId::Attitude: return 18;
    case InfoId::IntegratedNavigation: return 58;
    case InfoId::TimeCode: return 12;
    case InfoId::SituationDistribution: return VariableUnitLength;
    case InfoId::ManualTargetGuidance: return 24;
    case InfoId::ElectromagneticStatus: return 14;
    case InfoId::TurretControl: return 22;
    case InfoId::LaserNirControl: return 24;
    case InfoId::DeviceStatus: return 18;
    case InfoId::ElectroOpticalTarget: return 33;
    case InfoId::Unknown: return VariableUnitLength;
    }
    return VariableUnitLength;
}

} // namespace Udp709
