#include "config.h"
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <string>

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
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit config_error(QString("Cannot open config file: %1").arg(filepath));
        return false;
    }
    const QByteArray bytes = file.readAll();
    file.close();

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(bytes.constBegin(), bytes.constEnd());
    } catch (const nlohmann::json::parse_error& e) {
        emit config_error(QString("JSON parse error: %1").arg(e.what()));
        return false;
    }

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
    if (root.contains("udp709") && root["udp709"].is_object()) {
        m_data.udp709 = udp709_settings_from_json(root["udp709"]);
    }
    
    // Load UI settings
    if (root.contains("ui") && root["ui"].is_object()) {
        m_data.ui = ui_settings_from_json(root["ui"]);
    }
    
    // Load camera settings
    if (root.contains("camera") && root["camera"].is_object()) {
        m_data.camera = camera_settings_from_json(root["camera"]);
    }

    // Load video URL presets
    if (root.contains("video") && root["video"].is_object()) {
        m_data.video = video_settings_from_json(root["video"]);
    }

    // Load startup automation settings. Runtime config reloads store this
    // value for the next launch but do not execute automatic actions.
    if (root.contains("automatic") && root["automatic"].is_object()) {
        m_data.automatic = automatic_settings_from_json(root["automatic"]);
    }

    // Load startup debug settings.
    if (root.contains("debug") && root["debug"].is_object()) {
        m_data.debug = debug_settings_from_json(root["debug"]);
    }
    
    // Load TCU settings
    if (root.contains("tcu") && root["tcu"].is_object()) {
        m_data.tcu = tcu_settings_from_json(root["tcu"]);
    }
    
    // Load device settings
    if (root.contains("device") && root["device"].is_object()) {
        m_data.device = device_settings_from_json(root["device"]);
    }

    // Load save settings
    if (root.contains("save") && root["save"].is_object()) {
        m_data.save = save_settings_from_json(root["save"]);
    }

    // Load panorama optics calibration
    if (root.contains("panorama_optics") && root["panorama_optics"].is_object()) {
        m_data.panorama_optics = panorama_optics_settings_from_json(root["panorama_optics"]);
    }

    // Load image processing settings
    if (root.contains("image_proc") && root["image_proc"].is_object()) {
        m_data.image_proc = image_proc_settings_from_json(root["image_proc"]);
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
    root["udp709"] = udp709_settings_to_json(m_data.udp709);
    
    // Save UI settings
    root["ui"] = ui_settings_to_json(m_data.ui);
    
    // Save camera settings
    root["camera"] = camera_settings_to_json(m_data.camera);

    // Save video URL presets and startup automation
    root["video"] = video_settings_to_json(m_data.video);
    root["automatic"] = automatic_settings_to_json(m_data.automatic);
    root["debug"] = debug_settings_to_json(m_data.debug);
    
    // Save TCU settings
    root["tcu"] = tcu_settings_to_json(m_data.tcu);
    
    // Save device settings
    root["device"] = device_settings_to_json(m_data.device);

    // Save save settings
    root["save"] = save_settings_to_json(m_data.save);

    // Save panorama optics calibration
    root["panorama_optics"] = panorama_optics_settings_to_json(m_data.panorama_optics);

    // Save image processing settings
    root["image_proc"] = image_proc_settings_to_json(m_data.image_proc);

    // Save YOLO settings
    root["yolo"] = yolo_settings_to_json(m_data.yolo);

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const std::string dumped = root.dump(4);
    if (file.write(dumped.data(), static_cast<qint64>(dumped.size())) != static_cast<qint64>(dumped.size())) {
        file.close();
        return false;
    }
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
    obj["udpptz_protocol"] = settings.udpptz_protocol;
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
    if (obj.contains("udpptz_protocol") && obj["udpptz_protocol"].is_number()) {
        int v = obj["udpptz_protocol"].get<int>();
        // Legacy migration: old default.json used 0/1; new encoding is the
        // raw model number (500/530). Map old values forward on load.
        if (v == 0) v = 500;
        else if (v == 1) v = 530;
        settings.udpptz_protocol = v;
    }
    return settings;
}

nlohmann::json Config::udp709_settings_to_json(const Udp709Settings &settings) const
{
    nlohmann::json obj;
    obj["local_ip"] = settings.local_ip.toStdString();
    obj["local_port"] = settings.local_port;
    obj["peer_ip"] = settings.peer_ip.toStdString();
    obj["peer_port"] = settings.peer_port;
    return obj;
}

Config::Udp709Settings Config::udp709_settings_from_json(const nlohmann::json &obj) const
{
    Udp709Settings settings;
    if (obj.contains("local_ip") && obj["local_ip"].is_string()) {
        settings.local_ip = QString::fromStdString(obj["local_ip"].get<std::string>());
    }
    if (obj.contains("local_port") && obj["local_port"].is_number_integer()) {
        settings.local_port = qBound(1, obj["local_port"].get<int>(), 65535);
    }
    if (obj.contains("peer_ip") && obj["peer_ip"].is_string()) {
        settings.peer_ip = QString::fromStdString(obj["peer_ip"].get<std::string>());
    }
    if (obj.contains("peer_port") && obj["peer_port"].is_number_integer()) {
        settings.peer_port = qBound(1, obj["peer_port"].get<int>(), 65535);
    }
    return settings;
}

nlohmann::json Config::ui_settings_to_json(const UISettings &settings) const
{
    nlohmann::json obj;
    QString color_mode = settings.color_mode.trimmed().toLower();
    if (color_mode != "light") color_mode = "dark";
    const QByteArray custom_title_utf8 = settings.custom_title.toUtf8();
    obj["simplified"] = settings.simplified;
    obj["theme"] = settings.theme.toStdString();
    obj["color_mode"] = color_mode.toStdString();
    obj["custom title"] = std::string(custom_title_utf8.constData(), custom_title_utf8.size());
    obj["english"] = settings.english;
    return obj;
}

Config::UISettings Config::ui_settings_from_json(const nlohmann::json &obj) const
{
    UISettings settings;
    auto apply_color_mode = [&settings](QString mode) {
        mode = mode.trimmed().toLower();
        settings.color_mode = (mode == "light") ? "light" : "dark";
        settings.dark_mode = (settings.color_mode == "dark");
    };

    if (obj.contains("simplified") && obj["simplified"].is_boolean()) {
        settings.simplified = obj["simplified"].get<bool>();
    }
    if (obj.contains("theme") && obj["theme"].is_string()) {
        settings.theme = QString::fromStdString(obj["theme"].get<std::string>()).trimmed();
    } else if (obj.contains("theme_name") && obj["theme_name"].is_string()) {
        settings.theme = QString::fromStdString(obj["theme_name"].get<std::string>()).trimmed();
    }
    if (settings.theme.isEmpty() || settings.theme == "default") {
        settings.theme = "gruvbox";
    }

    if (obj.contains("color_mode") && obj["color_mode"].is_string()) {
        apply_color_mode(QString::fromStdString(obj["color_mode"].get<std::string>()));
    } else if (obj.contains("theme_mode") && obj["theme_mode"].is_string()) {
        apply_color_mode(QString::fromStdString(obj["theme_mode"].get<std::string>()));
    } else if (obj.contains("dark_theme") && obj["dark_theme"].is_boolean()) {
        apply_color_mode(obj["dark_theme"].get<bool>() ? "dark" : "light");
    }
    if (obj.contains("english") && obj["english"].is_boolean()) {
        settings.english = obj["english"].get<bool>();
    }
    if (obj.contains("custom title") && obj["custom title"].is_string()) {
        const std::string text = obj["custom title"].get<std::string>();
        settings.custom_title = QString::fromUtf8(text.data(), static_cast<int>(text.size()));
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

nlohmann::json Config::video_settings_to_json(const VideoSettings &settings) const
{
    nlohmann::json obj;
    obj["default_urls"] = nlohmann::json::array();
    for (const QString &url : settings.default_urls) {
        obj["default_urls"].push_back(url.toStdString());
    }
    return obj;
}

Config::VideoSettings Config::video_settings_from_json(const nlohmann::json &obj) const
{
    VideoSettings settings;
    if (obj.contains("default_urls") && obj["default_urls"].is_array()) {
        settings.default_urls.clear();
        for (const auto &url : obj["default_urls"]) {
            if (url.is_string()) {
                settings.default_urls.append(QString::fromStdString(url.get<std::string>()));
            }
        }
    }
    return settings;
}

nlohmann::json Config::automatic_settings_to_json(const AutomaticSettings &settings) const
{
    return nlohmann::json{{"enabled", settings.enabled}};
}

Config::AutomaticSettings Config::automatic_settings_from_json(const nlohmann::json &obj) const
{
    AutomaticSettings settings;
    if (obj.contains("enabled") && obj["enabled"].is_boolean()) {
        settings.enabled = obj["enabled"].get<bool>();
    }
    return settings;
}

nlohmann::json Config::debug_settings_to_json(const DebugSettings &settings) const
{
    return nlohmann::json{{"generate_dmp", settings.generate_dmp}};
}

Config::DebugSettings Config::debug_settings_from_json(const nlohmann::json &obj) const
{
    DebugSettings settings;
    if (obj.contains("generate_dmp") && obj["generate_dmp"].is_boolean()) {
        settings.generate_dmp = obj["generate_dmp"].get<bool>();
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
    
    // laser_on is intentionally NOT saved — lasers must always be disabled on startup (safety)
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
    
    // laser_on is intentionally NOT loaded — lasers must always be disabled on startup (safety)
    // settings.laser_on remains at default value 0
    return settings;
}

nlohmann::json Config::device_settings_to_json(const DeviceSettings &settings) const
{
    nlohmann::json obj;
    obj["flip"] = settings.flip; // int: 0=none, 1=both, 2=X, 3=Y
    obj["underwater"] = settings.underwater;
    obj["ebus"] = settings.ebus;
    obj["share_tcu_port"] = settings.share_tcu_port;
    obj["ptz_type"] = settings.ptz_type;
    obj["ptz_zero_angle_h"] = static_cast<double>(settings.ptz_zero_angle_h);
    obj["rotation"] = settings.rotation;
    obj["pixel_type"] = settings.pixel_type;
    return obj;
}

Config::DeviceSettings Config::device_settings_from_json(const nlohmann::json &obj) const
{
    DeviceSettings settings;
    if (obj.contains("flip")) {
        if (obj["flip"].is_number()) {
            settings.flip = obj["flip"].get<int>();
        } else if (obj["flip"].is_boolean()) {
            settings.flip = obj["flip"].get<bool>() ? 1 : 0; // backward compat
        }
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
    if (obj.contains("ptz_zero_angle_h") && obj["ptz_zero_angle_h"].is_number()) {
        settings.ptz_zero_angle_h = static_cast<float>(obj["ptz_zero_angle_h"].get<double>());
    }
    if (obj.contains("rotation") && obj["rotation"].is_number()) {
        settings.rotation = obj["rotation"].get<int>();
    }
    if (obj.contains("pixel_type") && obj["pixel_type"].is_number()) {
        settings.pixel_type = obj["pixel_type"].get<int>();
    }
    return settings;
}

nlohmann::json Config::save_settings_to_json(const SaveSettings &settings) const
{
    nlohmann::json obj;
    obj["save_info"] = settings.save_info;
    obj["custom_topleft_info"] = settings.custom_topleft_info;
    obj["save_in_grayscale"] = settings.save_in_grayscale;
    obj["consecutive_capture"] = settings.consecutive_capture;
    obj["integrate_info"] = settings.integrate_info;
    obj["img_format"] = settings.img_format;
    return obj;
}

Config::SaveSettings Config::save_settings_from_json(const nlohmann::json &obj) const
{
    SaveSettings settings;
    if (obj.contains("save_info") && obj["save_info"].is_boolean()) {
        settings.save_info = obj["save_info"].get<bool>();
    }
    if (obj.contains("custom_topleft_info") && obj["custom_topleft_info"].is_boolean()) {
        settings.custom_topleft_info = obj["custom_topleft_info"].get<bool>();
    }
    if (obj.contains("save_in_grayscale") && obj["save_in_grayscale"].is_boolean()) {
        settings.save_in_grayscale = obj["save_in_grayscale"].get<bool>();
    }
    if (obj.contains("consecutive_capture") && obj["consecutive_capture"].is_boolean()) {
        settings.consecutive_capture = obj["consecutive_capture"].get<bool>();
    }
    if (obj.contains("integrate_info") && obj["integrate_info"].is_boolean()) {
        settings.integrate_info = obj["integrate_info"].get<bool>();
    }
    if (obj.contains("img_format") && obj["img_format"].is_number()) {
        settings.img_format = obj["img_format"].get<int>();
    }
    return settings;
}

nlohmann::json Config::panorama_optics_settings_to_json(const PanoramaOpticsSettings &settings) const
{
    nlohmann::json obj;
    nlohmann::json zoomFovArray = nlohmann::json::array();
    for (const ZoomFovPoint &pt : settings.zoom_fov) {
        nlohmann::json item;
        item["zoom"] = static_cast<double>(pt.zoom);
        item["focal_length_mm"] = static_cast<double>(pt.focal_length_mm);
        item["fov_h"] = static_cast<double>(pt.fov_h);
        item["fov_v"] = static_cast<double>(pt.fov_v);
        zoomFovArray.push_back(item);
    }
    obj["zoom_fov"] = zoomFovArray;
    return obj;
}

Config::PanoramaOpticsSettings Config::panorama_optics_settings_from_json(const nlohmann::json &obj) const
{
    PanoramaOpticsSettings settings;
    if (!obj.contains("zoom_fov") || !obj["zoom_fov"].is_array()) return settings;

    std::vector<ZoomFovPoint> points;
    for (const auto &item : obj["zoom_fov"]) {
        if (!item.is_object()) continue;
        if (!item.contains("zoom") || !item.contains("fov_h") || !item.contains("fov_v")) continue;
        if (!item["zoom"].is_number() || !item["fov_h"].is_number() || !item["fov_v"].is_number()) continue;

        const float zoom = static_cast<float>(item["zoom"].get<double>());
        float focal_length_mm = zoom * 10.0f;
        if (item.contains("focal_length_mm") && item["focal_length_mm"].is_number()) {
            focal_length_mm = static_cast<float>(item["focal_length_mm"].get<double>());
        }
        const float fov_h = static_cast<float>(item["fov_h"].get<double>());
        const float fov_v = static_cast<float>(item["fov_v"].get<double>());
        if (zoom <= 0.0f || focal_length_mm <= 0.0f || fov_h <= 0.0f || fov_v <= 0.0f) continue;
        points.push_back(ZoomFovPoint(zoom, focal_length_mm, fov_h, fov_v));
    }

    if (!points.empty()) settings.zoom_fov = points;
    return settings;
}

nlohmann::json Config::image_proc_settings_to_json(const ImageProcSettings &settings) const
{
    nlohmann::json obj;
    // basic
    obj["accu_base"] = static_cast<double>(settings.accu_base);
    obj["gamma"] = static_cast<double>(settings.gamma);
    obj["low_in"] = static_cast<double>(settings.low_in);
    obj["high_in"] = static_cast<double>(settings.high_in);
    obj["low_out"] = static_cast<double>(settings.low_out);
    obj["high_out"] = static_cast<double>(settings.high_out);
    // dehaze
    obj["dehaze_pct"] = static_cast<double>(settings.dehaze_pct);
    obj["sky_tolerance"] = static_cast<double>(settings.sky_tolerance);
    obj["fast_gf"] = settings.fast_gf;
    // colormap
    obj["colormap"] = settings.colormap;
    // 3D gated
    obj["lower_3d_thresh"] = settings.lower_3d_thresh;
    obj["upper_3d_thresh"] = settings.upper_3d_thresh;
    obj["truncate_3d"] = settings.truncate_3d;
    obj["custom_3d_param"] = settings.custom_3d_param;
    obj["custom_3d_delay"] = static_cast<double>(settings.custom_3d_delay);
    obj["custom_3d_gate_width"] = static_cast<double>(settings.custom_3d_gate_width);
    // model/fishnet
    obj["fishnet_model_idx"] = settings.fishnet_model_idx;
    obj["fishnet_recog"] = settings.fishnet_recog;
    obj["fishnet_thresh"] = static_cast<double>(settings.fishnet_thresh);
    // ECC
    obj["ecc_window_mode"] = settings.ecc_window_mode;
    obj["ecc_warp_mode"] = settings.ecc_warp_mode;
    obj["ecc_fusion_method"] = settings.ecc_fusion_method;
    obj["ecc_backward"] = settings.ecc_backward;
    obj["ecc_forward"] = settings.ecc_forward;
    obj["ecc_levels"] = settings.ecc_levels;
    obj["ecc_max_iter"] = settings.ecc_max_iter;
    obj["ecc_eps"] = settings.ecc_eps;
    obj["ecc_half_res_reg"] = settings.ecc_half_res_reg;
    obj["ecc_half_res_fuse"] = settings.ecc_half_res_fuse;
    return obj;
}

Config::ImageProcSettings Config::image_proc_settings_from_json(const nlohmann::json &obj) const
{
    ImageProcSettings settings;
    // basic
    if (obj.contains("accu_base") && obj["accu_base"].is_number()) {
        settings.accu_base = static_cast<float>(obj["accu_base"].get<double>());
    }
    if (obj.contains("gamma") && obj["gamma"].is_number()) {
        settings.gamma = static_cast<float>(obj["gamma"].get<double>());
    }
    if (obj.contains("low_in") && obj["low_in"].is_number()) {
        settings.low_in = static_cast<float>(obj["low_in"].get<double>());
    }
    if (obj.contains("high_in") && obj["high_in"].is_number()) {
        settings.high_in = static_cast<float>(obj["high_in"].get<double>());
    }
    if (obj.contains("low_out") && obj["low_out"].is_number()) {
        settings.low_out = static_cast<float>(obj["low_out"].get<double>());
    }
    if (obj.contains("high_out") && obj["high_out"].is_number()) {
        settings.high_out = static_cast<float>(obj["high_out"].get<double>());
    }
    // dehaze
    if (obj.contains("dehaze_pct") && obj["dehaze_pct"].is_number()) {
        settings.dehaze_pct = static_cast<float>(obj["dehaze_pct"].get<double>());
    }
    if (obj.contains("sky_tolerance") && obj["sky_tolerance"].is_number()) {
        settings.sky_tolerance = static_cast<float>(obj["sky_tolerance"].get<double>());
    }
    if (obj.contains("fast_gf") && obj["fast_gf"].is_number()) {
        settings.fast_gf = obj["fast_gf"].get<int>();
    }
    // colormap
    if (obj.contains("colormap") && obj["colormap"].is_number()) {
        settings.colormap = obj["colormap"].get<int>();
    }
    // 3D gated
    if (obj.contains("lower_3d_thresh") && obj["lower_3d_thresh"].is_number()) {
        settings.lower_3d_thresh = obj["lower_3d_thresh"].get<double>();
    }
    if (obj.contains("upper_3d_thresh") && obj["upper_3d_thresh"].is_number()) {
        settings.upper_3d_thresh = obj["upper_3d_thresh"].get<double>();
    }
    if (obj.contains("truncate_3d") && obj["truncate_3d"].is_boolean()) {
        settings.truncate_3d = obj["truncate_3d"].get<bool>();
    }
    if (obj.contains("custom_3d_param") && obj["custom_3d_param"].is_boolean()) {
        settings.custom_3d_param = obj["custom_3d_param"].get<bool>();
    }
    if (obj.contains("custom_3d_delay") && obj["custom_3d_delay"].is_number()) {
        settings.custom_3d_delay = static_cast<float>(obj["custom_3d_delay"].get<double>());
    }
    if (obj.contains("custom_3d_gate_width") && obj["custom_3d_gate_width"].is_number()) {
        settings.custom_3d_gate_width = static_cast<float>(obj["custom_3d_gate_width"].get<double>());
    }
    // model/fishnet — accept legacy "model_idx" key on load for backward compat
    if (obj.contains("fishnet_model_idx") && obj["fishnet_model_idx"].is_number()) {
        settings.fishnet_model_idx = obj["fishnet_model_idx"].get<int>();
    } else if (obj.contains("model_idx") && obj["model_idx"].is_number()) {
        settings.fishnet_model_idx = obj["model_idx"].get<int>();
    }
    if (obj.contains("fishnet_recog") && obj["fishnet_recog"].is_boolean()) {
        settings.fishnet_recog = obj["fishnet_recog"].get<bool>();
    }
    if (obj.contains("fishnet_thresh") && obj["fishnet_thresh"].is_number()) {
        settings.fishnet_thresh = static_cast<float>(obj["fishnet_thresh"].get<double>());
    }
    // ECC
    if (obj.contains("ecc_window_mode") && obj["ecc_window_mode"].is_number()) {
        settings.ecc_window_mode = obj["ecc_window_mode"].get<int>();
    }
    if (obj.contains("ecc_warp_mode") && obj["ecc_warp_mode"].is_number()) {
        settings.ecc_warp_mode = obj["ecc_warp_mode"].get<int>();
    }
    if (obj.contains("ecc_fusion_method") && obj["ecc_fusion_method"].is_number()) {
        settings.ecc_fusion_method = obj["ecc_fusion_method"].get<int>();
    }
    if (obj.contains("ecc_backward") && obj["ecc_backward"].is_number()) {
        settings.ecc_backward = obj["ecc_backward"].get<int>();
    }
    if (obj.contains("ecc_forward") && obj["ecc_forward"].is_number()) {
        settings.ecc_forward = obj["ecc_forward"].get<int>();
    }
    if (obj.contains("ecc_levels") && obj["ecc_levels"].is_number()) {
        settings.ecc_levels = obj["ecc_levels"].get<int>();
    }
    if (obj.contains("ecc_max_iter") && obj["ecc_max_iter"].is_number()) {
        settings.ecc_max_iter = obj["ecc_max_iter"].get<int>();
    }
    if (obj.contains("ecc_eps") && obj["ecc_eps"].is_number()) {
        settings.ecc_eps = obj["ecc_eps"].get<double>();
    }
    if (obj.contains("ecc_half_res_reg") && obj["ecc_half_res_reg"].is_boolean()) {
        settings.ecc_half_res_reg = obj["ecc_half_res_reg"].get<bool>();
    }
    if (obj.contains("ecc_half_res_fuse") && obj["ecc_half_res_fuse"].is_boolean()) {
        settings.ecc_half_res_fuse = obj["ecc_half_res_fuse"].get<bool>();
    }
    return settings;
}

nlohmann::json Config::yolo_settings_to_json(const YoloSettings &settings) const
{
    nlohmann::json obj;
    obj["config_path"] = settings.config_path.toStdString();

    // Note: Model selection for each display (main_display_model, alt1_display_model, alt2_display_model)
    // are NOT saved to default.json - they are runtime-only settings

    // Visible light model
    obj["visible_model_path"] = settings.visible_model_path.toStdString();
    obj["visible_classes_file"] = settings.visible_classes_file.toStdString();

    // Thermal model
    obj["thermal_model_path"] = settings.thermal_model_path.toStdString();
    obj["thermal_classes_file"] = settings.thermal_classes_file.toStdString();

    // Gated imaging model
    obj["gated_model_path"] = settings.gated_model_path.toStdString();
    obj["gated_classes_file"] = settings.gated_classes_file.toStdString();

    return obj;
}

Config::YoloSettings Config::yolo_settings_from_json(const nlohmann::json &obj) const
{
    YoloSettings settings;
    if (obj.contains("config_path") && obj["config_path"].is_string()) {
        settings.config_path = QString::fromStdString(obj["config_path"].get<std::string>());
    }

    // Note: Model selection fields (main_display_model, alt1_display_model, alt2_display_model)
    // are NOT loaded from JSON - they always default to 0 (None) on startup

    // Visible light model
    if (obj.contains("visible_model_path") && obj["visible_model_path"].is_string()) {
        settings.visible_model_path = QString::fromStdString(obj["visible_model_path"].get<std::string>());
    }
    if (obj.contains("visible_classes_file") && obj["visible_classes_file"].is_string()) {
        settings.visible_classes_file = QString::fromStdString(obj["visible_classes_file"].get<std::string>());
    }

    // Thermal model
    if (obj.contains("thermal_model_path") && obj["thermal_model_path"].is_string()) {
        settings.thermal_model_path = QString::fromStdString(obj["thermal_model_path"].get<std::string>());
    }
    if (obj.contains("thermal_classes_file") && obj["thermal_classes_file"].is_string()) {
        settings.thermal_classes_file = QString::fromStdString(obj["thermal_classes_file"].get<std::string>());
    }

    // Gated imaging model
    if (obj.contains("gated_model_path") && obj["gated_model_path"].is_string()) {
        settings.gated_model_path = QString::fromStdString(obj["gated_model_path"].get<std::string>());
    }
    if (obj.contains("gated_classes_file") && obj["gated_classes_file"].is_string()) {
        settings.gated_classes_file = QString::fromStdString(obj["gated_classes_file"].get<std::string>());
    }

    return settings;
}

void Config::auto_save()
{
    QString default_config_path = QCoreApplication::applicationDirPath() + "/default.json";
    save_to_file(default_config_path);
}
