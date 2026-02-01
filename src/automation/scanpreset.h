#ifndef SCANPRESET_H
#define SCANPRESET_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "nlohmann/json.hpp"

class ScanPreset : public QObject
{
    Q_OBJECT

public:
    struct CameraParams {
        bool continuous_mode = true;
        int frequency = 10;
        float time_exposure = 95000.0f;
        float gain = 0.0f;
    };

    struct TCUParams {
        float PRF = 30.0f;
        float laser_width = 2000.0f;
        float delay_a = 11220.0f;
        float delay_b = 11220.0f;
        float gw_a = 500.0f;
        float gw_b = 500.0f;
        int mcp = 25;
        int type = 0;
    };

    struct PTZStartingPosition {
        float starting_h = 0.0f;
        float starting_v = 0.0f;
        int speed = 63;
    };

    struct TCUScanParams {
        float starting_delay = 8000.0f;
        float ending_delay = 15000.0f;
        float starting_gw = 400.0f;
        float ending_gw = 600.0f;
        float step_size_delay = 100.0f;
        float step_size_gw = 10.0f;
        int frame_count = 5;
        float rep_freq = 25.0f;
    };

    struct PTZScanParams {
        float starting_h = 0.0f;
        float ending_h = 10.0f;
        float starting_v = 0.0f;
        float ending_v = 10.0f;
        float step_h = 1.0f;
        float step_v = 1.0f;
        int count_h = 10;
        int count_v = 10;
        int direction = 0;
        int wait_time = 4;
        int num_single_pos = 8;
    };

    struct CaptureOptions {
        bool capture_scan_ori = true;
        bool capture_scan_res = false;
        bool record_scan_ori = false;
        bool record_scan_res = true;
    };

    struct ScanParameters {
        TCUScanParams tcu_scan;
        PTZScanParams ptz_scan;
        CaptureOptions capture_options;
    };

    struct ScanPresetData {
        QString name;
        CameraParams camera;
        TCUParams tcu;
        PTZStartingPosition ptz_starting_position;
        ScanParameters scan_parameters;
    };

public:
    explicit ScanPreset(QObject *parent = nullptr);
    ~ScanPreset();

    // Core functionality
    bool load_from_file(const QString& filepath);
    bool save_to_file(const QString& filepath) const;
    bool validate_preset();
    bool apply_preset();

    // Getters
    const ScanPresetData& get_data() const { return m_data; }
    bool is_valid() const { return m_is_valid; }
    QString get_last_error() const { return m_last_error; }

    // Setters
    void set_data(const ScanPresetData& data) { m_data = data; m_is_valid = false; }

private:
    // Validation functions
    bool validate_camera_params() const;
    bool validate_tcu_params() const;
    bool validate_ptz_params() const;
    bool validate_scan_parameters() const;
    bool verify_device_connectivity() const;

    // JSON conversion functions
    nlohmann::json camera_params_to_json(const CameraParams& params) const;
    CameraParams camera_params_from_json(const nlohmann::json& obj) const;

    nlohmann::json tcu_params_to_json(const TCUParams& params) const;
    TCUParams tcu_params_from_json(const nlohmann::json& obj) const;

    nlohmann::json ptz_position_to_json(const PTZStartingPosition& pos) const;
    PTZStartingPosition ptz_position_from_json(const nlohmann::json& obj) const;

    nlohmann::json scan_params_to_json(const ScanParameters& params) const;
    ScanParameters scan_params_from_json(const nlohmann::json& obj) const;

    // Helper functions
    void set_error(const QString& error) const;
    void clear_error() const;

private:
    ScanPresetData m_data;
    bool m_is_valid;
    mutable QString m_last_error;

signals:
    void scan_preset_loaded(const QString& name);
    void scan_preset_validation_failed(const QString& error);
};

#endif // SCANPRESET_H