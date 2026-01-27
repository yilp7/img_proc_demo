#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QString>
#include "util/version.h"
#include "nlohmann/json.hpp"

class Config : public QObject
{
    Q_OBJECT

public:
    struct ComSettings {
        QString port = "COM0";
        int baudrate = 9600;
    };

    struct NetworkSettings {
        QString tcp_server_address = "192.168.1.233";
        QString udp_target_ip = "192.168.1.10";
        int udp_target_port = 30000;
        QString udp_listen_ip = "192.168.1.100";
        int udp_listen_port = 30001;
    };

    struct UISettings {
        bool simplified = false;
        bool dark_theme = true;
        bool english = true;
    };

    struct CameraSettings {
        bool continuous_mode = true;
        int frequency = 10;
        float time_exposure = 95000.0f;
        float gain = 0.0f;
    };

    struct TCUSettings {
        int type = 0;
        bool auto_rep_freq = false;
        bool auto_mcp = false;
        bool ab_lock = true;
        int hz_unit = 0;
        int base_unit = 0;
        float max_dist = 15000.0f;
        float delay_offset = 0.0f;
        float max_dov = 15000.0f;
        float gate_width_offset = 0.0f;
        float max_laser_width = 25000.0f;
        float laser_width_offset = 0.0f;
        uint ps_step[4] = {40, 40, 40, 40};
        int laser_on = 0;
    };

    struct DeviceSettings {
        bool flip = false;
        bool underwater = false;
        bool ebus = false;
        bool share_tcu_port = false;
        int ptz_type = 0;
        int cam_to_gate[4] = {1, 2, 3, 4};  // Camera to TCU gate mapping (0=none, 1=A, 2=B, 3=C, 4=D)
    };

    struct ConfigData {
        QString version;
        ComSettings com_tcu;
        ComSettings com_lens;
        ComSettings com_laser;
        ComSettings com_range;
        ComSettings com_ptz;
        NetworkSettings network;
        UISettings ui;
        CameraSettings camera;
        TCUSettings tcu;
        DeviceSettings device;

        ConfigData() {
            version = QString("%1.%2.%3.%4")
                      .arg(VER_MAJOR)
                      .arg(VER_MINOR)
                      .arg(VER_PATCH)
                      .arg(VER_TWEAK);
        }
    };

public:
    explicit Config(QObject *parent = nullptr);
    ~Config();

    bool load_from_file(const QString &filepath);
    bool save_to_file(const QString &filepath) const;

    void load_defaults();

    void auto_save();

    const ConfigData& get_data() const { return m_data; }
    ConfigData& get_data() { return m_data; }

    QString get_version() const { return m_data.version; }
    void set_version(const QString &version) { m_data.version = version; }

private:
    ConfigData m_data;

    nlohmann::json com_settings_to_json(const ComSettings &settings) const;
    ComSettings com_settings_from_json(const nlohmann::json &obj) const;

    nlohmann::json network_settings_to_json(const NetworkSettings &settings) const;
    NetworkSettings network_settings_from_json(const nlohmann::json &obj) const;

    nlohmann::json ui_settings_to_json(const UISettings &settings) const;
    UISettings ui_settings_from_json(const nlohmann::json &obj) const;

    nlohmann::json camera_settings_to_json(const CameraSettings &settings) const;
    CameraSettings camera_settings_from_json(const nlohmann::json &obj) const;

    nlohmann::json tcu_settings_to_json(const TCUSettings &settings) const;
    TCUSettings tcu_settings_from_json(const nlohmann::json &obj) const;

    nlohmann::json device_settings_to_json(const DeviceSettings &settings) const;
    DeviceSettings device_settings_from_json(const nlohmann::json &obj) const;

signals:
    void config_loaded();
    void config_saved();
    void config_error(const QString &error);
};

#endif // CONFIG_H
