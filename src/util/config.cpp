#include "config.h"
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <fstream>

Config::Config(QObject *parent)
    : QObject(parent)
{
    load_defaults();
}

Config::~Config()
{
}

void Config::load_defaults()
{
    m_data = ConfigData();
}

bool Config::load_from_file(const QString &filepath)
{
    std::ifstream file(filepath.toStdString());
    if (!file.is_open()) {
        emit config_error(QString("Cannot open config file: %1").arg(filepath));
        return false;
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::parse_error& e) {
        emit config_error(QString("JSON parse error: %1").arg(e.what()));
        return false;
    }
    file.close();

    if (!root.is_object()) {
        emit config_error("Config file root must be a JSON object");
        return false;
    }
    
    // Load version
    if (root.contains("version") && root["version"].is_string()) {
        m_data.version = QString::fromStdString(root["version"].get<std::string>());
    }
    
    // Load COM settings
    if (root.contains("com_tcu") && root["com_tcu"].is_object()) {
        m_data.com_tcu = com_settings_from_json(root["com_tcu"]);
    }
    if (root.contains("com_lens") && root["com_lens"].is_object()) {
        m_data.com_lens = com_settings_from_json(root["com_lens"]);
    }
    if (root.contains("com_laser") && root["com_laser"].is_object()) {
        m_data.com_laser = com_settings_from_json(root["com_laser"]);
    }
    if (root.contains("com_range") && root["com_range"].is_object()) {
        m_data.com_range = com_settings_from_json(root["com_range"]);
    }
    if (root.contains("com_ptz") && root["com_ptz"].is_object()) {
        m_data.com_ptz = com_settings_from_json(root["com_ptz"]);
    }
    
    // Load network settings
    if (root.contains("network") && root["network"].is_object()) {
        m_data.network = network_settings_from_json(root["network"]);
    }
    
    // Load UI settings
    if (root.contains("ui") && root["ui"].is_object()) {
        m_data.ui = ui_settings_from_json(root["ui"]);
    }
    
    // Load camera settings
    if (root.contains("camera") && root["camera"].is_object()) {
        m_data.camera = camera_settings_from_json(root["camera"]);
    }
    
    // Load TCU settings
    if (root.contains("tcu") && root["tcu"].is_object()) {
        m_data.tcu = tcu_settings_from_json(root["tcu"]);
    }
    
    // Load device settings
    if (root.contains("device") && root["device"].is_object()) {
        m_data.device = device_settings_from_json(root["device"]);
    }

    // Load YOLO settings
    if (root.contains("yolo") && root["yolo"].is_object()) {
        m_data.yolo = yolo_settings_from_json(root["yolo"]);
    }

    emit config_loaded();
    return true;
}

bool Config::save_to_file(const QString &filepath) const
{
    nlohmann::json root;
    
    // Save version
    root["version"] = m_data.version.toStdString();
    
    // Save COM settings
    root["com_tcu"] = com_settings_to_json(m_data.com_tcu);
    root["com_lens"] = com_settings_to_json(m_data.com_lens);
    root["com_laser"] = com_settings_to_json(m_data.com_laser);
    root["com_range"] = com_settings_to_json(m_data.com_range);
    root["com_ptz"] = com_settings_to_json(m_data.com_ptz);
    
    // Save network settings
    root["network"] = network_settings_to_json(m_data.network);
    
    // Save UI settings
    root["ui"] = ui_settings_to_json(m_data.ui);
    
    // Save camera settings
    root["camera"] = camera_settings_to_json(m_data.camera);
    
    // Save TCU settings
    root["tcu"] = tcu_settings_to_json(m_data.tcu);
    
    // Save device settings
    root["device"] = device_settings_to_json(m_data.device);

    // Save YOLO settings
    root["yolo"] = yolo_settings_to_json(m_data.yolo);

    std::ofstream file(filepath.toStdString());
    if (!file.is_open()) {
        return false;
    }
    
    file << root.dump(4);
    file.close();
    
    return true;
}

nlohmann::json Config::com_settings_to_json(const ComSettings &settings) const
{
    nlohmann::json obj;
    obj["port"] = settings.port.toStdString();
    obj["baudrate"] = settings.baudrate;
    return obj;
}

Config::ComSettings Config::com_settings_from_json(const nlohmann::json &obj) const
{
    ComSettings settings;
    if (obj.contains("port") && obj["port"].is_string()) {
        settings.port = QString::fromStdString(obj["port"].get<std::string>());
    }
    if (obj.contains("baudrate") && obj["baudrate"].is_number()) {
        settings.baudrate = obj["baudrate"].get<int>();
    }
    return settings;
}

nlohmann::json Config::network_settings_to_json(const NetworkSettings &settings) const
{
    nlohmann::json obj;
    obj["tcp_server_address"] = settings.tcp_server_address.toStdString();
    obj["udp_target_ip"] = settings.udp_target_ip.toStdString();
    obj["udp_target_port"] = settings.udp_target_port;
    obj["udp_listen_ip"] = settings.udp_listen_ip.toStdString();
    obj["udp_listen_port"] = settings.udp_listen_port;
    return obj;
}

Config::NetworkSettings Config::network_settings_from_json(const nlohmann::json &obj) const
{
    NetworkSettings settings;
    if (obj.contains("tcp_server_address") && obj["tcp_server_address"].is_string()) {
        settings.tcp_server_address = QString::fromStdString(obj["tcp_server_address"].get<std::string>());
    }
    if (obj.contains("udp_target_ip") && obj["udp_target_ip"].is_string()) {
        settings.udp_target_ip = QString::fromStdString(obj["udp_target_ip"].get<std::string>());
    }
    if (obj.contains("udp_target_port") && obj["udp_target_port"].is_number()) {
        settings.udp_target_port = obj["udp_target_port"].get<int>();
    }
    if (obj.contains("udp_listen_ip") && obj["udp_listen_ip"].is_string()) {
        settings.udp_listen_ip = QString::fromStdString(obj["udp_listen_ip"].get<std::string>());
    }
    if (obj.contains("udp_listen_port") && obj["udp_listen_port"].is_number()) {
        settings.udp_listen_port = obj["udp_listen_port"].get<int>();
    }
    return settings;
}

nlohmann::json Config::ui_settings_to_json(const UISettings &settings) const
{
    nlohmann::json obj;
    obj["simplified"] = settings.simplified;
    obj["dark_theme"] = settings.dark_theme;
    obj["english"] = settings.english;
    return obj;
}

Config::UISettings Config::ui_settings_from_json(const nlohmann::json &obj) const
{
    UISettings settings;
    if (obj.contains("simplified") && obj["simplified"].is_boolean()) {
        settings.simplified = obj["simplified"].get<bool>();
    }
    if (obj.contains("dark_theme") && obj["dark_theme"].is_boolean()) {
        settings.dark_theme = obj["dark_theme"].get<bool>();
    }
    if (obj.contains("english") && obj["english"].is_boolean()) {
        settings.english = obj["english"].get<bool>();
    }
    return settings;
}

nlohmann::json Config::camera_settings_to_json(const CameraSettings &settings) const
{
    nlohmann::json obj;
    obj["continuous_mode"] = settings.continuous_mode;
    obj["frequency"] = settings.frequency;
    obj["time_exposure"] = static_cast<double>(settings.time_exposure);
    obj["gain"] = static_cast<double>(settings.gain);
    return obj;
}

Config::CameraSettings Config::camera_settings_from_json(const nlohmann::json &obj) const
{
    CameraSettings settings;
    if (obj.contains("continuous_mode") && obj["continuous_mode"].is_boolean()) {
        settings.continuous_mode = obj["continuous_mode"].get<bool>();
    }
    if (obj.contains("frequency") && obj["frequency"].is_number()) {
        settings.frequency = obj["frequency"].get<int>();
    }
    if (obj.contains("time_exposure") && obj["time_exposure"].is_number()) {
        settings.time_exposure = static_cast<float>(obj["time_exposure"].get<double>());
    }
    if (obj.contains("gain") && obj["gain"].is_number()) {
        settings.gain = static_cast<float>(obj["gain"].get<double>());
    }
    return settings;
}

nlohmann::json Config::tcu_settings_to_json(const TCUSettings &settings) const
{
    nlohmann::json obj;
    obj["type"] = settings.type;
    obj["auto_rep_freq"] = settings.auto_rep_freq;
    obj["auto_mcp"] = settings.auto_mcp;
    obj["ab_lock"] = settings.ab_lock;
    obj["hz_unit"] = settings.hz_unit;
    obj["base_unit"] = settings.base_unit;
    obj["max_dist"] = static_cast<double>(settings.max_dist);
    obj["delay_offset"] = static_cast<double>(settings.delay_offset);
    obj["max_dov"] = static_cast<double>(settings.max_dov);
    obj["gate_width_offset"] = static_cast<double>(settings.gate_width_offset);
    obj["max_laser_width"] = static_cast<double>(settings.max_laser_width);
    obj["laser_width_offset"] = static_cast<double>(settings.laser_width_offset);
    
    nlohmann::json psStepArray = nlohmann::json::array();
    for (int i = 0; i < 4; ++i) {
        psStepArray.push_back(static_cast<int>(settings.ps_step[i]));
    }
    obj["ps_step"] = psStepArray;
    
    obj["laser_on"] = settings.laser_on;
    return obj;
}

Config::TCUSettings Config::tcu_settings_from_json(const nlohmann::json &obj) const
{
    TCUSettings settings;
    if (obj.contains("type") && obj["type"].is_number()) {
        settings.type = obj["type"].get<int>();
    }
    if (obj.contains("auto_rep_freq") && obj["auto_rep_freq"].is_boolean()) {
        settings.auto_rep_freq = obj["auto_rep_freq"].get<bool>();
    }
    if (obj.contains("auto_mcp") && obj["auto_mcp"].is_boolean()) {
        settings.auto_mcp = obj["auto_mcp"].get<bool>();
    }
    if (obj.contains("ab_lock") && obj["ab_lock"].is_boolean()) {
        settings.ab_lock = obj["ab_lock"].get<bool>();
    }
    if (obj.contains("hz_unit") && obj["hz_unit"].is_number()) {
        settings.hz_unit = obj["hz_unit"].get<int>();
    }
    if (obj.contains("base_unit") && obj["base_unit"].is_number()) {
        settings.base_unit = obj["base_unit"].get<int>();
    }
    if (obj.contains("max_dist") && obj["max_dist"].is_number()) {
        settings.max_dist = static_cast<float>(obj["max_dist"].get<double>());
    }
    if (obj.contains("delay_offset") && obj["delay_offset"].is_number()) {
        settings.delay_offset = static_cast<float>(obj["delay_offset"].get<double>());
    }
    if (obj.contains("max_dov") && obj["max_dov"].is_number()) {
        settings.max_dov = static_cast<float>(obj["max_dov"].get<double>());
    }
    if (obj.contains("gate_width_offset") && obj["gate_width_offset"].is_number()) {
        settings.gate_width_offset = static_cast<float>(obj["gate_width_offset"].get<double>());
    }
    if (obj.contains("max_laser_width") && obj["max_laser_width"].is_number()) {
        settings.max_laser_width = static_cast<float>(obj["max_laser_width"].get<double>());
    }
    if (obj.contains("laser_width_offset") && obj["laser_width_offset"].is_number()) {
        settings.laser_width_offset = static_cast<float>(obj["laser_width_offset"].get<double>());
    }
    
    if (obj.contains("ps_step") && obj["ps_step"].is_array()) {
        auto psStepArray = obj["ps_step"];
        for (int i = 0; i < 4 && i < psStepArray.size(); ++i) {
            if (psStepArray[i].is_number()) {
                settings.ps_step[i] = static_cast<uint>(psStepArray[i].get<int>());
            }
        }
    }
    
    if (obj.contains("laser_on") && obj["laser_on"].is_number()) {
        settings.laser_on = obj["laser_on"].get<int>();
    }
    return settings;
}

nlohmann::json Config::device_settings_to_json(const DeviceSettings &settings) const
{
    nlohmann::json obj;
    obj["flip"] = settings.flip;
    obj["underwater"] = settings.underwater;
    obj["ebus"] = settings.ebus;
    obj["share_tcu_port"] = settings.share_tcu_port;
    obj["ptz_type"] = settings.ptz_type;
    return obj;
}

Config::DeviceSettings Config::device_settings_from_json(const nlohmann::json &obj) const
{
    DeviceSettings settings;
    if (obj.contains("flip") && obj["flip"].is_boolean()) {
        settings.flip = obj["flip"].get<bool>();
    }
    if (obj.contains("underwater") && obj["underwater"].is_boolean()) {
        settings.underwater = obj["underwater"].get<bool>();
    }
    if (obj.contains("ebus") && obj["ebus"].is_boolean()) {
        settings.ebus = obj["ebus"].get<bool>();
    }
    if (obj.contains("share_tcu_port") && obj["share_tcu_port"].is_boolean()) {
        settings.share_tcu_port = obj["share_tcu_port"].get<bool>();
    }
    if (obj.contains("ptz_type") && obj["ptz_type"].is_number()) {
        settings.ptz_type = obj["ptz_type"].get<int>();
    }
    return settings;
}

nlohmann::json Config::yolo_settings_to_json(const YoloSettings &settings) const
{
    nlohmann::json obj;
    obj["enabled"] = settings.enabled;
    obj["config_path"] = settings.config_path.toStdString();
    return obj;
}

Config::YoloSettings Config::yolo_settings_from_json(const nlohmann::json &obj) const
{
    YoloSettings settings;
    if (obj.contains("enabled") && obj["enabled"].is_boolean()) {
        settings.enabled = obj["enabled"].get<bool>();
    }
    if (obj.contains("config_path") && obj["config_path"].is_string()) {
        settings.config_path = QString::fromStdString(obj["config_path"].get<std::string>());
    }
    return settings;
}

void Config::auto_save()
{
    QString default_config_path = QCoreApplication::applicationDirPath() + "/default.json";
    save_to_file(default_config_path);
}