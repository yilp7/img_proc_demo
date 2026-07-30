#ifndef UDP709WINDOW_H
#define UDP709WINDOW_H

#include <QDialog>
#include <QMap>
#include <QVector>

#include "protocol709/udp709service.h"

class Config;
class QCheckBox;
class QCloseEvent;
class QFormLayout;
class QGroupBox;
class QJsonObject;
class QJsonValue;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTimer;

class Udp709Window : public QDialog
{
    Q_OBJECT

public:
    explicit Udp709Window(Config *config, QWidget *parent = nullptr);

    void toggleVisible();
    Q_DECL_DEPRECATED_X("UDP709 G2 transmission is deprecated")
    void setAutomaticElectromagneticStatus(const Udp709::ElectromagneticStatusPayload &payload);
    void setAutomaticDeviceStatus(const Udp709::DeviceStatusPayload &payload);
    void setAutomaticTargets(const QVector<Udp709::ElectroOpticalTargetPayload> &targets);

signals:
    void turretControlReceived(Udp709::TurretControlPayload payload);
    void laserNirControlReceived(Udp709::LaserNirControlPayload payload);

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void toggleConnection();
    void transmitCycle();
    void handleReceived(Udp709::DecodedPacket packet, QString senderIp, quint16 senderPort);
    void handleSent(Udp709::DecodedPacket packet);

private:
    struct FieldSpec {
        QString key;
        QString label;
    };

    struct FormPage {
        QWidget *widget = nullptr;
        QLabel *meta = nullptr;
        QCheckBox *manual = nullptr;
        QFormLayout *layout = nullptr;
        QMap<QString, QLabel*> labels;
        QMap<QString, QLineEdit*> fields;
    };

    FormPage createFormPage(const QVector<FieldSpec> &specs, bool manualMode);
    void setupUi();
    void setupTransmitTabs();
    void setupReceiveTabs();
    void reloadEndpointFields();
    void retranslateUi();
    void setConnectionState(bool bound, const QString &description);
    void setFormValues(FormPage &form, const QJsonObject &values);
    void setStatus(const QString &message, bool error = false);
    QString fieldLabel(const QString &key) const;

    bool readManualG2(Udp709::ElectromagneticStatusPayload *payload, QString *error) const;
    bool readManualG5(Udp709::DeviceStatusPayload *payload, QString *error) const;
    bool readManualG6(Udp709::ElectroOpticalTargetPayload *payload, QString *error) const;

    static QString jsonValueText(const QJsonValue &value);
    static QJsonObject fieldsFor(const Udp709::ElectromagneticStatusPayload &payload);
    static QJsonObject fieldsFor(const Udp709::DeviceStatusPayload &payload);
    static QJsonObject fieldsFor(const Udp709::ElectroOpticalTargetPayload &payload);

    Config *m_config;
    Udp709::Service *m_service;
    QTimer *m_sendTimer;

    QGroupBox *m_endpointGroup = nullptr;
    QLabel *m_localIpLabel = nullptr;
    QLabel *m_localPortLabel = nullptr;
    QLabel *m_peerIpLabel = nullptr;
    QLabel *m_peerPortLabel = nullptr;
    QLineEdit *m_localIpEdit = nullptr;
    QLineEdit *m_localPortEdit = nullptr;
    QLineEdit *m_peerIpEdit = nullptr;
    QLineEdit *m_peerPortEdit = nullptr;
    QPushButton *m_connectionButton = nullptr;
    QLabel *m_connectionStatus = nullptr;
    QLabel *m_activityStatus = nullptr;
    QTabWidget *m_txTabs = nullptr;
    QTabWidget *m_rxTabs = nullptr;

    QMap<int, FormPage> m_txForms;
    QMap<int, FormPage> m_rxForms;

    Udp709::ElectromagneticStatusPayload m_autoG2;
    Udp709::DeviceStatusPayload m_autoG5;
    QVector<Udp709::ElectroOpticalTargetPayload> m_autoG6;
};

Q_DECLARE_METATYPE(Udp709::TurretControlPayload)
Q_DECLARE_METATYPE(Udp709::LaserNirControlPayload)

#endif // UDP709WINDOW_H
