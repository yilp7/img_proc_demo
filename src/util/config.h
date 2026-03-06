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
        int flip = 0; // 0=none, 1=both, 2=X-axis, 3=Y-axis (cv::flip flipCode = flip-2)
        bool underwater = false;
        bool ebus = false;
        bool share_tcu_port = false;
        int ptz_type = 0;
        int rotation = 0;
        int pixel_type = 0;
    };

    struct SaveSettings {
        bool save_info = true;
        bool custom_topleft_info = false;
        bool save_in_grayscale = false;
        bool consecutive_capture = true;
        bool integrate_info = true;
        int img_format = 0;
    };

    struct ImageProcSettings {
        // basic
        float accu_base = 1.0f;
        float gamma = 1.2f;
        float low_in = 0.0f;
        float high_in = 0.05f;
        float low_out = 0.0f;
        float high_out = 1.0f;
        // dehaze
        float dehaze_pct = 0.95f;
        float sky_tolerance = 40.0f;
        int fast_gf = 1;
        // colormap
        int colormap = 2; // cv::COLORMAP_JET
        // 3D gated
        double lower_3d_thresh = 0.0;
        double upper_3d_thresh = 0.981;
        bool truncate_3d = false;
        bool custom_3d_param = false;
        float custom_3d_delay = 0.0f;
        float custom_3d_gate_width = 0.0f;
        // model/fishnet
        int model_idx = 0;
        bool fishnet_recog = false;
        float fishnet_thresh = 0.99f;
        // ECC temporal denoising
        int ecc_window_mode = 0;
        int ecc_warp_mode = 2;
        int ecc_fusion_method = 2;
        int ecc_backward = 20;
        int ecc_forward = 0;
        int ecc_levels = 1;
        int ecc_max_iter = 8;
        double ecc_eps = 0.001;
        bool ecc_half_res_reg = true;
        bool ecc_half_res_fuse = false;
    };

    struct YoloSettings {
        QString config_path = "yolo_config.ini";  // relative to default.json dir

        // Model selection for each display (0=None, 1=Visible Light, 2=Thermal, 3=Gated)
        int main_display_model = 0;
        int alt1_display_model = 0;
        int alt2_display_model = 0;

        // Visible light model
        QString visible_model_path = "models/VL.onnx";
        QString visible_classes_file = "models/VL.txt";

        // Thermal model
        QString thermal_model_path = "models/THERMAL.onnx";
        QString thermal_classes_file = "models/THERMAL.txt";

        // Gated imaging model
        QString gated_model_path = "models/GATED.onnx";
        QString gated_classes_file = "models/GATED.txt";
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
        SaveSettings save;
        ImageProcSettings image_proc;
        YoloSettings yolo;

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

    nlohmann::json save_settings_to_json(const SaveSettings &settings) const;
    SaveSettings save_settings_from_json(const nlohmann::json &obj) const;

    nlohmann::json image_proc_settings_to_json(const ImageProcSettings &settings) const;
    ImageProcSettings image_proc_settings_from_json(const nlohmann::json &obj) const;

    nlohmann::json yolo_settings_to_json(const YoloSettings &settings) const;
    YoloSettings yolo_settings_from_json(const nlohmann::json &obj) const;

signals:
    void config_loaded();
    void config_saved();
    void config_error(const QString &error);
};

#endif // CONFIG_H
