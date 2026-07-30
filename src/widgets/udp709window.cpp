#include "widgets/udp709window.h"

#include "util/config.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QEvent>
#include <QFrame>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QSplitter>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <limits>

using namespace Udp709;

namespace {

bool readUnsigned(const QMap<QString, QLineEdit*> &fields, const QString &key,
                  quint64 maximum, quint64 *value, QString *error)
{
    bool ok = false;
    const QString text = fields.value(key)->text().trimmed();
    const quint64 parsed = text.toULongLong(&ok, 0);
    if (!ok || parsed > maximum) {
        if (error) *error = QObject::tr("Invalid %1 value: %2").arg(key, text);
        return false;
    }
    *value = parsed;
    return true;
}

bool readSigned(const QMap<QString, QLineEdit*> &fields, const QString &key,
                qint64 minimum, qint64 maximum, qint64 *value, QString *error)
{
    bool ok = false;
    const QString text = fields.value(key)->text().trimmed();
    const qint64 parsed = text.toLongLong(&ok, 0);
    if (!ok || parsed < minimum || parsed > maximum) {
        if (error) *error = QObject::tr("Invalid %1 value: %2").arg(key, text);
        return false;
    }
    *value = parsed;
    return true;
}

void refreshDynamicStyle(QWidget *widget)
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

} // namespace

Udp709Window::Udp709Window(Config *config, QWidget *parent)
    : QDialog(parent),
      m_config(config),
      m_service(new Service(this)),
      m_sendTimer(new QTimer(this))
{
    setObjectName(QStringLiteral("UDP709_WINDOW"));
    setModal(false);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(1100, 720);
    setMinimumSize(760, 500);

    qRegisterMetaType<Udp709::TurretControlPayload>("Udp709::TurretControlPayload");
    qRegisterMetaType<Udp709::LaserNirControlPayload>("Udp709::LaserNirControlPayload");

    setupUi();
    reloadEndpointFields();
    retranslateUi();

    m_sendTimer->setInterval(1000);
    connect(m_sendTimer, &QTimer::timeout, this, &Udp709Window::transmitCycle);
    connect(m_connectionButton, &QPushButton::clicked, this, &Udp709Window::toggleConnection);
    connect(m_service, &Service::packetReceived, this, &Udp709Window::handleReceived);
    connect(m_service, &Service::packetSent, this, &Udp709Window::handleSent);
    connect(m_service, &Service::boundChanged, this, &Udp709Window::setConnectionState);
    connect(m_service, &Service::transportError, this, [this](const QString &message) {
        setStatus(message, true);
    });
    if (m_config) {
        connect(m_config, &Config::config_loaded, this, [this]() {
            if (!m_service->isBound()) reloadEndpointFields();
        });
    }

    setConnectionState(false, tr("Not connected"));
    setStatus(tr("Ready"));
}

void Udp709Window::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    m_endpointGroup = new QGroupBox(this);
    auto *endpointLayout = new QHBoxLayout(m_endpointGroup);
    endpointLayout->setContentsMargins(10, 14, 10, 8);
    endpointLayout->setSpacing(6);

    m_localIpLabel = new QLabel(m_endpointGroup);
    m_localPortLabel = new QLabel(m_endpointGroup);
    m_peerIpLabel = new QLabel(m_endpointGroup);
    m_peerPortLabel = new QLabel(m_endpointGroup);
    m_localIpEdit = new QLineEdit(m_endpointGroup);
    m_localPortEdit = new QLineEdit(m_endpointGroup);
    m_peerIpEdit = new QLineEdit(m_endpointGroup);
    m_peerPortEdit = new QLineEdit(m_endpointGroup);
    m_connectionButton = new QPushButton(m_endpointGroup);
    m_connectionStatus = new QLabel(m_endpointGroup);

    m_localIpEdit->setMinimumWidth(125);
    m_peerIpEdit->setMinimumWidth(125);
    m_localPortEdit->setFixedWidth(72);
    m_peerPortEdit->setFixedWidth(72);
    m_localPortEdit->setValidator(new QIntValidator(1, 65535, m_localPortEdit));
    m_peerPortEdit->setValidator(new QIntValidator(1, 65535, m_peerPortEdit));
    m_connectionButton->setMinimumWidth(92);
    m_connectionStatus->setMinimumWidth(170);

    endpointLayout->addWidget(m_localIpLabel);
    endpointLayout->addWidget(m_localIpEdit);
    endpointLayout->addWidget(m_localPortLabel);
    endpointLayout->addWidget(m_localPortEdit);
    endpointLayout->addSpacing(10);
    endpointLayout->addWidget(m_peerIpLabel);
    endpointLayout->addWidget(m_peerIpEdit);
    endpointLayout->addWidget(m_peerPortLabel);
    endpointLayout->addWidget(m_peerPortEdit);
    endpointLayout->addWidget(m_connectionButton);
    endpointLayout->addWidget(m_connectionStatus, 1);
    root->addWidget(m_endpointGroup);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    m_txTabs = new QTabWidget(splitter);
    m_rxTabs = new QTabWidget(splitter);
    m_txTabs->setObjectName(QStringLiteral("UDP709_TX_TABS"));
    m_rxTabs->setObjectName(QStringLiteral("UDP709_RX_TABS"));
    m_txTabs->tabBar()->setExpanding(true);
    m_txTabs->tabBar()->setUsesScrollButtons(true);
    m_txTabs->tabBar()->setElideMode(Qt::ElideNone);
    m_rxTabs->tabBar()->setExpanding(true);
    m_rxTabs->tabBar()->setUsesScrollButtons(true);
    m_rxTabs->tabBar()->setElideMode(Qt::ElideNone);
    splitter->addWidget(m_txTabs);
    splitter->addWidget(m_rxTabs);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({360, 700});
    root->addWidget(splitter, 1);

    setupTransmitTabs();
    setupReceiveTabs();

    m_activityStatus = new QLabel(this);
    m_activityStatus->setObjectName(QStringLiteral("UDP709_ACTIVITY_STATUS"));
    m_activityStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(m_activityStatus);
}

Udp709Window::FormPage Udp709Window::createFormPage(const QVector<FieldSpec> &specs,
                                                     bool manualMode)
{
    FormPage page;
    page.widget = new QWidget(this);
    auto *pageLayout = new QVBoxLayout(page.widget);
    pageLayout->setContentsMargins(8, 8, 8, 8);
    pageLayout->setSpacing(6);

    page.meta = new QLabel(page.widget);
    page.meta->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pageLayout->addWidget(page.meta);

    if (manualMode) {
        page.manual = new QCheckBox(page.widget);
        pageLayout->addWidget(page.manual);
    }

    auto *scroll = new QScrollArea(page.widget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *contents = new QWidget(scroll);
    page.layout = new QFormLayout(contents);
    page.layout->setContentsMargins(2, 4, 8, 4);
    page.layout->setSpacing(6);
    page.layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    for (const FieldSpec &spec : specs) {
        auto *label = new QLabel(spec.label, contents);
        auto *edit = new QLineEdit(contents);
        edit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        edit->setMinimumWidth(120);
        if (manualMode) {
            edit->setReadOnly(true);
        } else {
            edit->setEnabled(false);
        }
        edit->setText(QStringLiteral("0"));
        page.labels.insert(spec.key, label);
        page.fields.insert(spec.key, edit);
        page.layout->addRow(label, edit);
    }
    scroll->setWidget(contents);
    pageLayout->addWidget(scroll, 1);

    if (page.manual) {
        const auto fields = page.fields;
        connect(page.manual, &QCheckBox::toggled, page.widget, [fields](bool manual) {
            for (QLineEdit *edit : fields) edit->setReadOnly(!manual);
        });
    }
    return page;
}

void Udp709Window::setupTransmitTabs()
{
    const auto add = [this](InfoId id, const QVector<QString> &keys) {
        QVector<FieldSpec> specs;
        specs.reserve(keys.size());
        for (const QString &key : keys) specs.push_back({key, fieldLabel(key)});
        FormPage page = createFormPage(specs, true);
        m_txForms.insert(static_cast<int>(id), page);
        m_txTabs->addTab(page.widget, PacketCodec::infoIdLabel(id).section(QLatin1Char(' '), 0, 0));
    };

    add(InfoId::ElectromagneticStatus,
        {"timestampRaw", "switchState", "spectrumStartRaw", "spectrumEndRaw", "reserved"});
    add(InfoId::DeviceStatus,
        {"timestampRaw", "visibleLightState", "midWaveInfraredState", "laserNirState",
         "azimuthHundredths", "elevationHundredths", "trackingState", "turretState"});
    add(InfoId::ElectroOpticalTarget,
        {"timestampTenthsOfMillisecond", "reportType", "targetBatch", "targetType",
         "targetCategory", "targetRangeMeters", "targetAzimuthHundredths",
         "targetElevationHundredths"});

    setFormValues(m_txForms[static_cast<int>(InfoId::ElectromagneticStatus)], fieldsFor(m_autoG2));
    setFormValues(m_txForms[static_cast<int>(InfoId::DeviceStatus)], fieldsFor(m_autoG5));
    setFormValues(m_txForms[static_cast<int>(InfoId::ElectroOpticalTarget)],
                  fieldsFor(ElectroOpticalTargetPayload()));
    m_txTabs->setTabEnabled(0, false);
    m_txTabs->setCurrentIndex(1);
}

void Udp709Window::setupReceiveTabs()
{
    const auto add = [this](InfoId id, const QVector<QString> &keys) {
        QVector<FieldSpec> specs;
        specs.reserve(keys.size());
        for (const QString &key : keys) specs.push_back({key, fieldLabel(key)});
        FormPage page = createFormPage(specs, false);
        m_rxForms.insert(static_cast<int>(id), page);
        m_rxTabs->addTab(page.widget, PacketCodec::infoIdLabel(id).section(QLatin1Char(' '), 0, 0));
    };

    add(InfoId::Attitude,
        {"validityFlags", "headingDegrees", "pitchDegrees", "rollDegrees"});
    add(InfoId::IntegratedNavigation,
        {"validityFlags", "relativeSpeedKnots", "absoluteSpeedKnots", "trackAngleDegrees",
         "eastSpeedKnots", "northSpeedKnots", "longitudeDegrees", "latitudeDegrees",
         "verticalSpeedMetersPerSecond", "timeBytesHex", "trueWindSpeedMetersPerSecond",
         "trueWindDirectionDegrees", "relativeWindSpeedMetersPerSecond",
         "relativeWindDirectionDegrees", "temperatureCelsius", "relativeHumidityPercent",
         "atmosphericPressureHpa", "seaCurrentSpeedRaw", "seaCurrentDirectionRaw",
         "seaDepthMeters", "seaStateRaw"});
    add(InfoId::TimeCode,
        {"second", "minute", "hour", "westOfUtcPlus8", "timeZoneOffsetHours",
         "day", "month", "year"});
    add(InfoId::SituationDistribution,
        {"timestampMilliseconds", "fusionBatch", "targetType", "targetCategory",
         "militaryCivilAttribute", "friendFoeAttribute", "countryRegion", "targetRangeRaw",
         "targetAzimuthRaw", "targetElevationRaw", "longitudeDegrees", "latitudeDegrees",
         "depthHeightRaw", "relativeHeadingRaw", "absoluteHeadingRaw", "relativeSpeedRaw",
         "absoluteSpeedRaw", "velocityXRaw", "velocityYRaw", "velocityZRaw",
         "sourceCount", "sources"});
    add(InfoId::ManualTargetGuidance,
        {"timestampRaw", "targetBatch", "targetRangeRaw", "targetAzimuthRaw",
         "targetElevationRaw", "command"});
    add(InfoId::TurretControl,
        {"timestampRaw", "visibleLightState", "midWaveInfraredState", "laserNirState",
         "azimuthRaw", "elevationHundredths", "trackingState", "turretState",
         "imagingLensControl", "reserved"});
    add(InfoId::LaserNirControl,
        {"timestampRaw", "laserEnabled", "laserIntensity", "illuminationLensControl",
         "workingDistanceMeters", "imagingDepthMeters", "imagingGain",
         "imageProcessing", "reserved"});
}

void Udp709Window::toggleVisible()
{
    if (isVisible()) {
        hide();
        return;
    }
    if (!m_service->isBound()) reloadEndpointFields();
    show();
    raise();
    activateWindow();
}

void Udp709Window::setAutomaticElectromagneticStatus(const ElectromagneticStatusPayload &payload)
{
    m_autoG2 = payload;
    FormPage &form = m_txForms[static_cast<int>(InfoId::ElectromagneticStatus)];
    if (!form.manual->isChecked()) setFormValues(form, fieldsFor(m_autoG2));
}

void Udp709Window::setAutomaticDeviceStatus(const DeviceStatusPayload &payload)
{
    m_autoG5 = payload;
    FormPage &form = m_txForms[static_cast<int>(InfoId::DeviceStatus)];
    if (!form.manual->isChecked()) setFormValues(form, fieldsFor(payload));
}

void Udp709Window::setAutomaticTargets(const QVector<ElectroOpticalTargetPayload> &targets)
{
    m_autoG6 = targets;
    FormPage &form = m_txForms[static_cast<int>(InfoId::ElectroOpticalTarget)];
    if (!form.manual->isChecked()) {
        setFormValues(form, fieldsFor(targets.isEmpty() ? ElectroOpticalTargetPayload()
                                                        : targets.first()));
        form.meta->setText(tr("Automatic source: %1 detected target(s)").arg(targets.size()));
    }
}

void Udp709Window::toggleConnection()
{
    if (m_service->isBound()) {
        m_service->close();
        return;
    }

    bool localOk = false;
    bool peerOk = false;
    const int localPort = m_localPortEdit->text().toInt(&localOk);
    const int peerPort = m_peerPortEdit->text().toInt(&peerOk);
    if (!localOk || !peerOk || localPort < 1 || localPort > 65535
            || peerPort < 1 || peerPort > 65535) {
        setStatus(tr("Invalid UDP port"), true);
        return;
    }

    QString error;
    if (!m_service->bindEndpoint(m_localIpEdit->text(), static_cast<quint16>(localPort),
                                 m_peerIpEdit->text(), static_cast<quint16>(peerPort), &error)) {
        setStatus(error, true);
        return;
    }

    if (m_config) {
        auto &settings = m_config->get_data().udp709;
        settings.local_ip = m_localIpEdit->text().trimmed();
        settings.local_port = localPort;
        settings.peer_ip = m_peerIpEdit->text().trimmed();
        settings.peer_port = peerPort;
        m_config->auto_save();
    }
}

void Udp709Window::transmitCycle()
{
    if (!m_service->isBound()) return;

    QString error;
    FormPage &g5Form = m_txForms[static_cast<int>(InfoId::DeviceStatus)];
    DeviceStatusPayload g5 = m_autoG5;
    if (g5Form.manual->isChecked() && !readManualG5(&g5, &error)) {
        setStatus(error, true);
        return;
    }
    if (!g5Form.manual->isChecked()) {
        g5.timestampRaw = static_cast<quint32>(QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
        setFormValues(g5Form, fieldsFor(g5));
    }
    if (!m_service->sendDeviceStatus(g5, &error)) {
        setStatus(error, true);
        return;
    }

    FormPage &g6Form = m_txForms[static_cast<int>(InfoId::ElectroOpticalTarget)];
    if (g6Form.manual->isChecked()) {
        ElectroOpticalTargetPayload manualTarget;
        if (!readManualG6(&manualTarget, &error)
                || !m_service->sendElectroOpticalTarget(manualTarget, &error)) {
            setStatus(error, true);
        }
        return;
    }

    for (const ElectroOpticalTargetPayload &target : m_autoG6) {
        if (!m_service->sendElectroOpticalTarget(target, &error)) {
            setStatus(error, true);
            return;
        }
    }
}

void Udp709Window::handleReceived(DecodedPacket packet, QString senderIp, quint16 senderPort)
{
    const int key = static_cast<int>(packet.infoId);
    if (!m_rxForms.contains(key)) return;

    FormPage &form = m_rxForms[key];
    setFormValues(form, packet.fields);
    form.meta->setText(tr("RX sequence %1 from %2:%3 at %4")
                       .arg(packet.frameNumber)
                       .arg(senderIp)
                       .arg(senderPort)
                       .arg(packet.receivedAt.toString(QStringLiteral("HH:mm:ss.zzz"))));
    setStatus(tr("Received %1").arg(packet.infoName));

    QString error;
    if (packet.infoId == InfoId::TurretControl) {
        TurretControlPayload payload;
        if (PacketCodec::parseTurretControl(packet, &payload, &error)) emit turretControlReceived(payload);
    } else if (packet.infoId == InfoId::LaserNirControl) {
        LaserNirControlPayload payload;
        if (PacketCodec::parseLaserNirControl(packet, &payload, &error)) emit laserNirControlReceived(payload);
    }
    if (!error.isEmpty()) setStatus(error, true);
}

void Udp709Window::handleSent(DecodedPacket packet)
{
    const int key = static_cast<int>(packet.infoId);
    if (m_txForms.contains(key)) {
        m_txForms[key].meta->setText(tr("TX sequence %1 at %2")
                                     .arg(packet.frameNumber)
                                     .arg(packet.receivedAt.toString(QStringLiteral("HH:mm:ss.zzz"))));
    }
    setStatus(tr("Sent %1").arg(packet.infoName));
}

void Udp709Window::reloadEndpointFields()
{
    const Config::Udp709Settings settings = m_config ? m_config->get_data().udp709
                                                      : Config::Udp709Settings();
    m_localIpEdit->setText(settings.local_ip);
    m_localPortEdit->setText(QString::number(settings.local_port));
    m_peerIpEdit->setText(settings.peer_ip);
    m_peerPortEdit->setText(QString::number(settings.peer_port));
}

void Udp709Window::setConnectionState(bool bound, const QString &description)
{
    m_localIpEdit->setEnabled(!bound);
    m_localPortEdit->setEnabled(!bound);
    m_peerIpEdit->setEnabled(!bound);
    m_peerPortEdit->setEnabled(!bound);
    m_connectionButton->setText(bound ? tr("Disconnect") : tr("Bind"));
    m_connectionStatus->setText(description);
    m_connectionStatus->setProperty("portState", bound ? "connected" : "idle");
    refreshDynamicStyle(m_connectionStatus);
    if (bound) m_sendTimer->start();
    else m_sendTimer->stop();
}

void Udp709Window::setFormValues(FormPage &form, const QJsonObject &values)
{
    for (auto it = form.fields.begin(); it != form.fields.end(); ++it) {
        const auto value = values.constFind(it.key());
        if (value != values.constEnd()) it.value()->setText(jsonValueText(*value));
    }
}

void Udp709Window::setStatus(const QString &message, bool error)
{
    m_activityStatus->setText(message);
    m_activityStatus->setProperty("portState", error ? "error" : "connected");
    refreshDynamicStyle(m_activityStatus);
}

bool Udp709Window::readManualG2(ElectromagneticStatusPayload *payload, QString *error) const
{
    const auto &fields = m_txForms[static_cast<int>(InfoId::ElectromagneticStatus)].fields;
    quint64 timestamp = 0, state = 0, start = 0, end = 0, reserved = 0;
    if (!readUnsigned(fields, "timestampRaw", std::numeric_limits<quint32>::max(), &timestamp, error)
            || !readUnsigned(fields, "switchState", std::numeric_limits<quint8>::max(), &state, error)
            || !readUnsigned(fields, "spectrumStartRaw", std::numeric_limits<quint16>::max(), &start, error)
            || !readUnsigned(fields, "spectrumEndRaw", std::numeric_limits<quint16>::max(), &end, error)
            || !readUnsigned(fields, "reserved", std::numeric_limits<quint8>::max(), &reserved, error)) return false;
    payload->timestampRaw = static_cast<quint32>(timestamp);
    payload->switchState = static_cast<quint8>(state);
    payload->spectrumStartRaw = static_cast<quint16>(start);
    payload->spectrumEndRaw = static_cast<quint16>(end);
    payload->reserved = static_cast<quint8>(reserved);
    return true;
}

bool Udp709Window::readManualG5(DeviceStatusPayload *payload, QString *error) const
{
    const auto &fields = m_txForms[static_cast<int>(InfoId::DeviceStatus)].fields;
    quint64 timestamp = 0, visible = 0, infrared = 0, laser = 0, azimuth = 0;
    quint64 tracking = 0, turret = 0;
    qint64 elevation = 0;
    if (!readUnsigned(fields, "timestampRaw", std::numeric_limits<quint32>::max(), &timestamp, error)
            || !readUnsigned(fields, "visibleLightState", 255, &visible, error)
            || !readUnsigned(fields, "midWaveInfraredState", 255, &infrared, error)
            || !readUnsigned(fields, "laserNirState", 255, &laser, error)
            || !readUnsigned(fields, "azimuthHundredths", 65535, &azimuth, error)
            || !readSigned(fields, "elevationHundredths", -32768, 32767, &elevation, error)
            || !readUnsigned(fields, "trackingState", 255, &tracking, error)
            || !readUnsigned(fields, "turretState", 65535, &turret, error)) return false;
    payload->timestampRaw = static_cast<quint32>(timestamp);
    payload->visibleLightState = static_cast<quint8>(visible);
    payload->midWaveInfraredState = static_cast<quint8>(infrared);
    payload->laserNirState = static_cast<quint8>(laser);
    payload->azimuthHundredths = static_cast<quint16>(azimuth);
    payload->elevationHundredths = static_cast<qint16>(elevation);
    payload->trackingState = static_cast<quint8>(tracking);
    payload->turretState = static_cast<quint16>(turret);
    return true;
}

bool Udp709Window::readManualG6(ElectroOpticalTargetPayload *payload, QString *error) const
{
    const auto &fields = m_txForms[static_cast<int>(InfoId::ElectroOpticalTarget)].fields;
    quint64 timestamp = 0, report = 0, batch = 0, type = 0, category = 0;
    quint64 range = 0, azimuth = 0;
    qint64 elevation = 0;
    if (!readUnsigned(fields, "timestampTenthsOfMillisecond", std::numeric_limits<quint32>::max(), &timestamp, error)
            || !readUnsigned(fields, "reportType", 255, &report, error)
            || !readUnsigned(fields, "targetBatch", 65535, &batch, error)
            || !readUnsigned(fields, "targetType", 255, &type, error)
            || !readUnsigned(fields, "targetCategory", 255, &category, error)
            || !readUnsigned(fields, "targetRangeMeters", std::numeric_limits<quint32>::max(), &range, error)
            || !readUnsigned(fields, "targetAzimuthHundredths", std::numeric_limits<quint32>::max(), &azimuth, error)
            || !readSigned(fields, "targetElevationHundredths", std::numeric_limits<qint32>::min(),
                           std::numeric_limits<qint32>::max(), &elevation, error)) return false;
    payload->timestampTenthsOfMillisecond = static_cast<quint32>(timestamp);
    payload->reportType = static_cast<quint8>(report);
    payload->targetBatch = static_cast<quint16>(batch);
    payload->targetType = static_cast<quint8>(type);
    payload->targetCategory = static_cast<quint8>(category);
    payload->targetRangeMeters = static_cast<quint32>(range);
    payload->targetAzimuthHundredths = static_cast<quint32>(azimuth);
    payload->targetElevationHundredths = static_cast<qint32>(elevation);
    return true;
}

QString Udp709Window::jsonValueText(const QJsonValue &value)
{
    if (value.isString()) return value.toString();
    if (value.isBool()) return value.toBool() ? QStringLiteral("1") : QStringLiteral("0");
    if (value.isDouble()) return QString::number(value.toDouble(), 'g', 12);
    if (value.isArray()) return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    if (value.isObject()) return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    return QString();
}

QJsonObject Udp709Window::fieldsFor(const ElectromagneticStatusPayload &payload)
{
    return {{"timestampRaw", static_cast<double>(payload.timestampRaw)},
            {"switchState", payload.switchState},
            {"spectrumStartRaw", payload.spectrumStartRaw},
            {"spectrumEndRaw", payload.spectrumEndRaw},
            {"reserved", payload.reserved}};
}

QJsonObject Udp709Window::fieldsFor(const DeviceStatusPayload &payload)
{
    return {{"timestampRaw", static_cast<double>(payload.timestampRaw)},
            {"visibleLightState", payload.visibleLightState},
            {"midWaveInfraredState", payload.midWaveInfraredState},
            {"laserNirState", payload.laserNirState},
            {"azimuthHundredths", payload.azimuthHundredths},
            {"elevationHundredths", payload.elevationHundredths},
            {"trackingState", payload.trackingState},
            {"turretState", payload.turretState}};
}

QJsonObject Udp709Window::fieldsFor(const ElectroOpticalTargetPayload &payload)
{
    return {{"timestampTenthsOfMillisecond", static_cast<double>(payload.timestampTenthsOfMillisecond)},
            {"reportType", payload.reportType},
            {"targetBatch", payload.targetBatch},
            {"targetType", payload.targetType},
            {"targetCategory", payload.targetCategory},
            {"targetRangeMeters", static_cast<double>(payload.targetRangeMeters)},
            {"targetAzimuthHundredths", static_cast<double>(payload.targetAzimuthHundredths)},
            {"targetElevationHundredths", payload.targetElevationHundredths}};
}

QString Udp709Window::fieldLabel(const QString &key) const
{
    if (key == "timestampRaw") return tr("Timestamp");
    if (key == "timestampMilliseconds") return tr("Timestamp (ms)");
    if (key == "timestampTenthsOfMillisecond") return tr("Timestamp (0.1 ms)");
    if (key == "validityFlags") return tr("Validity flags");
    if (key == "headingDegrees") return tr("Heading (deg)");
    if (key == "pitchDegrees") return tr("Pitch (deg)");
    if (key == "rollDegrees") return tr("Roll (deg)");
    if (key == "switchState") return tr("Switch state");
    if (key == "spectrumStartRaw") return tr("Spectrum start");
    if (key == "spectrumEndRaw") return tr("Spectrum end");
    if (key == "visibleLightState") return tr("Visible-light state");
    if (key == "midWaveInfraredState") return tr("MWIR state");
    if (key == "laserNirState") return tr("Laser/NIR state");
    if (key == "azimuthRaw") return tr("Azimuth / H velocity raw");
    if (key == "azimuthHundredths") return tr("Azimuth (0.01 deg)");
    if (key == "elevationHundredths") return tr("Elevation / V velocity (0.01)");
    if (key == "trackingState") return tr("Tracking state");
    if (key == "turretState") return tr("Turret state");
    if (key == "imagingLensControl") return tr("Imaging lens control");
    if (key == "laserEnabled") return tr("Laser enabled");
    if (key == "laserIntensity") return tr("Laser intensity");
    if (key == "illuminationLensControl") return tr("Illumination lens control");
    if (key == "workingDistanceMeters") return tr("Working distance (m)");
    if (key == "imagingDepthMeters") return tr("Imaging depth (m)");
    if (key == "imagingGain") return tr("Imaging gain");
    if (key == "imageProcessing") return tr("Image processing bits");
    if (key == "reportType") return tr("Report type");
    if (key == "targetBatch") return tr("Target batch");
    if (key == "targetType") return tr("Target type");
    if (key == "targetCategory") return tr("Target category");
    if (key == "targetRangeMeters") return tr("Target range (m)");
    if (key == "targetAzimuthHundredths") return tr("Target azimuth (0.01 deg)");
    if (key == "targetElevationHundredths") return tr("Target elevation (0.01 deg)");
    if (key == "command") return tr("Command");
    if (key == "reserved") return tr("Reserved");
    if (key == "fusionBatch") return tr("Fusion batch");
    if (key == "militaryCivilAttribute") return tr("Military/civil attribute");
    if (key == "friendFoeAttribute") return tr("Friend/foe attribute");
    if (key == "countryRegion") return tr("Country/region");
    if (key == "targetRangeRaw") return tr("Target range raw");
    if (key == "targetAzimuthRaw") return tr("Target azimuth raw");
    if (key == "targetElevationRaw") return tr("Target elevation raw");
    if (key == "longitudeDegrees") return tr("Longitude (deg)");
    if (key == "latitudeDegrees") return tr("Latitude (deg)");
    if (key == "depthHeightRaw") return tr("Depth/height raw");
    if (key == "relativeHeadingRaw") return tr("Relative heading raw");
    if (key == "absoluteHeadingRaw") return tr("Absolute heading raw");
    if (key == "relativeSpeedRaw") return tr("Relative speed raw");
    if (key == "absoluteSpeedRaw") return tr("Absolute speed raw");
    if (key == "velocityXRaw") return tr("Velocity X raw");
    if (key == "velocityYRaw") return tr("Velocity Y raw");
    if (key == "velocityZRaw") return tr("Velocity Z raw");
    if (key == "sourceCount") return tr("Source count");
    if (key == "sources") return tr("Sources");
    if (key == "relativeSpeedKnots") return tr("Relative speed (kn)");
    if (key == "absoluteSpeedKnots") return tr("Absolute speed (kn)");
    if (key == "trackAngleDegrees") return tr("Track angle (deg)");
    if (key == "eastSpeedKnots") return tr("East speed (kn)");
    if (key == "northSpeedKnots") return tr("North speed (kn)");
    if (key == "verticalSpeedMetersPerSecond") return tr("Vertical speed (m/s)");
    if (key == "timeBytesHex") return tr("Time bytes");
    if (key == "trueWindSpeedMetersPerSecond") return tr("True wind speed (m/s)");
    if (key == "trueWindDirectionDegrees") return tr("True wind direction (deg)");
    if (key == "relativeWindSpeedMetersPerSecond") return tr("Relative wind speed (m/s)");
    if (key == "relativeWindDirectionDegrees") return tr("Relative wind direction (deg)");
    if (key == "temperatureCelsius") return tr("Temperature (C)");
    if (key == "relativeHumidityPercent") return tr("Relative humidity (%)");
    if (key == "atmosphericPressureHpa") return tr("Pressure (hPa)");
    if (key == "seaCurrentSpeedRaw") return tr("Sea-current speed raw");
    if (key == "seaCurrentDirectionRaw") return tr("Sea-current direction raw");
    if (key == "seaDepthMeters") return tr("Sea depth (m)");
    if (key == "seaStateRaw") return tr("Sea state raw");
    if (key == "second") return tr("Second");
    if (key == "minute") return tr("Minute");
    if (key == "hour") return tr("Hour");
    if (key == "westOfUtcPlus8") return tr("West of UTC+8");
    if (key == "timeZoneOffsetHours") return tr("Timezone offset (h)");
    if (key == "day") return tr("Day");
    if (key == "month") return tr("Month");
    if (key == "year") return tr("Year");
    return key;
}

void Udp709Window::retranslateUi()
{
    setWindowTitle(tr("UDP 709 Interface"));
    m_endpointGroup->setTitle(tr("UDP Endpoint"));
    m_localIpLabel->setText(tr("Local IP"));
    m_localPortLabel->setText(tr("Port"));
    m_peerIpLabel->setText(tr("Peer IP"));
    m_peerPortLabel->setText(tr("Port"));
    m_connectionButton->setText(m_service->isBound() ? tr("Disconnect") : tr("Bind"));

    m_txTabs->setTabText(0, tr("G2 Deprecated"));
    m_txTabs->setTabText(1, tr("G5 Device"));
    m_txTabs->setTabText(2, tr("G6 Target"));
    m_rxTabs->setTabText(0, tr("B0 Attitude"));
    m_rxTabs->setTabText(1, tr("B1 Navigation"));
    m_rxTabs->setTabText(2, tr("B3 Time"));
    m_rxTabs->setTabText(3, tr("G0 Situation"));
    m_rxTabs->setTabText(4, tr("G1 Guidance"));
    m_rxTabs->setTabText(5, tr("G3 Turret"));
    m_rxTabs->setTabText(6, tr("G4 Laser/NIR"));
    m_txForms[static_cast<int>(InfoId::ElectromagneticStatus)].meta->setText(
                tr("Deprecated - transmission disabled"));

    for (auto formIt = m_txForms.begin(); formIt != m_txForms.end(); ++formIt) {
        FormPage &form = formIt.value();
        if (form.manual) form.manual->setText(tr("Manual mode"));
        for (auto labelIt = form.labels.begin(); labelIt != form.labels.end(); ++labelIt) {
            labelIt.value()->setText(fieldLabel(labelIt.key()));
        }
    }
    for (auto formIt = m_rxForms.begin(); formIt != m_rxForms.end(); ++formIt) {
        for (auto labelIt = formIt.value().labels.begin(); labelIt != formIt.value().labels.end(); ++labelIt) {
            labelIt.value()->setText(fieldLabel(labelIt.key()));
        }
    }
}

void Udp709Window::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void Udp709Window::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) retranslateUi();
    QDialog::changeEvent(event);
}
