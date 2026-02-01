#include "scanpreset.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <cmath>

ScanPreset::ScanPreset(QObject *parent)
    : QObject(parent)
    , m_is_valid(false)
{
}

ScanPreset::~ScanPreset()
{
}

bool ScanPreset::load_from_file(const QString& filepath)
{
    clear_error();

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        set_error(QString("Cannot open file: %1").arg(filepath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    try {
        nlohmann::json json = nlohmann::json::parse(data.toStdString());

        // Parse name
        if (json.contains("name") && json["name"].is_string()) {
            m_data.name = QString::fromStdString(json["name"]);
        } else {
            set_error("Missing or invalid 'name' field");
            return false;
        }

        // Parse data section
        if (!json.contains("data") || !json["data"].is_object()) {
            set_error("Missing or invalid 'data' section");
            return false;
        }

        nlohmann::json data_obj = json["data"];

        // Parse camera parameters
        if (data_obj.contains("camera")) {
            m_data.camera = camera_params_from_json(data_obj["camera"]);
        }

        // Parse TCU parameters
        if (data_obj.contains("tcu")) {
            m_data.tcu = tcu_params_from_json(data_obj["tcu"]);
        }

        // Parse PTZ starting position
        if (data_obj.contains("ptz_starting_position")) {
            m_data.ptz_starting_position = ptz_position_from_json(data_obj["ptz_starting_position"]);
        }

        // Parse scan parameters
        if (data_obj.contains("scan_parameters")) {
            m_data.scan_parameters = scan_params_from_json(data_obj["scan_parameters"]);
        }

        // Data loaded successfully, set as valid and let caller handle validation
        m_is_valid = false; // Will be set to true after explicit validation
        qDebug() << "Successfully parsed scan preset data:" << m_data.name;
        return true;

    } catch (const nlohmann::json::exception& e) {
        set_error(QString("JSON parsing error: %1").arg(e.what()));
        return false;
    }
}

bool ScanPreset::save_to_file(const QString& filepath) const
{
    try {
        nlohmann::json json;
        json["name"] = m_data.name.toStdString();

        nlohmann::json data_obj;
        data_obj["camera"] = camera_params_to_json(m_data.camera);
        data_obj["tcu"] = tcu_params_to_json(m_data.tcu);
        data_obj["ptz_starting_position"] = ptz_position_to_json(m_data.ptz_starting_position);
        data_obj["scan_parameters"] = scan_params_to_json(m_data.scan_parameters);

        json["data"] = data_obj;

        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }

        QString json_string = QString::fromStdString(json.dump(2));
        file.write(json_string.toUtf8());
        file.close();

        return true;
    } catch (const nlohmann::json::exception&) {
        return false;
    }
}

bool ScanPreset::validate_preset()
{
    clear_error();

    if (!validate_camera_params()) {
        emit scan_preset_validation_failed(m_last_error);
        return false;
    }
    if (!validate_tcu_params()) {
        emit scan_preset_validation_failed(m_last_error);
        return false;
    }
    if (!validate_ptz_params()) {
        emit scan_preset_validation_failed(m_last_error);
        return false;
    }
    if (!validate_scan_parameters()) {
        emit scan_preset_validation_failed(m_last_error);
        return false;
    }

    m_is_valid = true;
    emit scan_preset_loaded(m_data.name);
    return true;
}

bool ScanPreset::validate_camera_params() const
{
    const auto& cam = m_data.camera;

    if (cam.frequency < 0.1 || cam.frequency > 100) {
        set_error("Camera frequency out of range (0.1-100)");
        return false;
    }

    if (cam.time_exposure < 1000.0f || cam.time_exposure > 200000.0f) {
        set_error("Camera exposure time out of range (1000-1000000)");
        return false;
    }

    if (cam.gain < 0.0f || cam.gain > 23.0f) {
        set_error("Camera gain out of range (0-23)");
        return false;
    }

    return true;
}

bool ScanPreset::validate_tcu_params() const
{
    const auto& tcu = m_data.tcu;

    if (tcu.PRF < 0.01f || tcu.PRF > 100.0f) {
        set_error("TCU PRF out of range (0.01-100)");
        return false;
    }

    if (tcu.laser_width < 0.f || tcu.laser_width > 25000.0f) {
        set_error("TCU laser width out of range (0-25000)");
        return false;
    }

    if (tcu.delay_a < 0.0f || tcu.delay_a > 1000000.0f) {
        set_error("TCU delay_a out of range (0-1000000)");
        return false;
    }

    if (tcu.delay_b < 0.0f || tcu.delay_b > 1000000.0f) {
        set_error("TCU delay_b out of range (0-1000000)");
        return false;
    }

    if (tcu.gw_a < 0.0f || tcu.gw_a > 100000.0f) {
        set_error("TCU gw_a out of range (0-100000)");
        return false;
    }

    if (tcu.gw_b < 0.0f || tcu.gw_b > 100000.0f) {
        set_error("TCU gw_b out of range (0-100000)");
        return false;
    }

    if (tcu.mcp < 0 || tcu.mcp > 255) {
        set_error("TCU MCP out of range (0-255)");
        return false;
    }

    return true;
}

bool ScanPreset::validate_ptz_params() const
{
    const auto& ptz = m_data.ptz_starting_position;

//    if (ptz.starting_h < -180.0f || ptz.starting_h > 180.0f) {
//        set_error("PTZ starting_h out of range (-180 to 180)");
//        return false;
//    }

//    if (ptz.starting_v < -90.0f || ptz.starting_v > 90.0f) {
//        set_error("PTZ starting_v out of range (-90 to 90)");
//        return false;
//    }

    if (ptz.speed < 1 || ptz.speed > 64) {
        set_error("PTZ speed out of range (1-64)");
        return false;
    }

    return true;
}

bool ScanPreset::validate_scan_parameters() const
{
    const auto& tcu_scan = m_data.scan_parameters.tcu_scan;
    const auto& ptz_scan = m_data.scan_parameters.ptz_scan;

    // Validate TCU scan parameters
    if (tcu_scan.starting_delay >= tcu_scan.ending_delay) {
        set_error("TCU starting_delay must be less than ending_delay");
        return false;
    }

    if (tcu_scan.starting_gw >= tcu_scan.ending_gw) {
        set_error("TCU starting_gw must be less than ending_gw");
        return false;
    }

    if (tcu_scan.step_size_delay <= 0.0f) {
        set_error("TCU step_size_delay must be positive");
        return false;
    }

    if (tcu_scan.step_size_gw <= 0.0f) {
        set_error("TCU step_size_gw must be positive");
        return false;
    }

    if (tcu_scan.frame_count < 1 || tcu_scan.frame_count > 100) {
        set_error("TCU frame_count out of range (1-100)");
        return false;
    }

    // Validate PTZ scan parameters
    if (ptz_scan.starting_h >= ptz_scan.ending_h) {
        set_error("PTZ starting_h must be less than ending_h");
        return false;
    }

    if (ptz_scan.starting_v >= ptz_scan.ending_v) {
        set_error("PTZ starting_v must be less than ending_v");
        return false;
    }

    if (ptz_scan.step_h <= 0.0f || ptz_scan.step_v <= 0.0f) {
        set_error("PTZ step sizes must be positive");
        return false;
    }

    if (ptz_scan.wait_time < 0 || ptz_scan.wait_time > 30) {
        set_error("PTZ wait_time out of range (0-30)");
        return false;
    }

    return true;
}

bool ScanPreset::verify_device_connectivity() const
{
    // TODO: Implement device connectivity checks
    // This would check if TCU, PTZ, and camera are accessible
    return true;
}

bool ScanPreset::apply_preset()
{
    if (!m_is_valid) {
        set_error("Cannot apply invalid preset");
        return false;
    }

    // Preset data is validated and ready to be applied by the calling code
    // The caller (AutoScan) will access the data via get_data() and emit appropriate signals
    return true;
}

// JSON conversion functions
nlohmann::json ScanPreset::camera_params_to_json(const CameraParams& params) const
{
    nlohmann::json obj;
    obj["continuous_mode"] = params.continuous_mode;
    obj["frequency"] = params.frequency;
    obj["time_exposure"] = params.time_exposure;
    obj["gain"] = params.gain;
    return obj;
}

ScanPreset::CameraParams ScanPreset::camera_params_from_json(const nlohmann::json& obj) const
{
    CameraParams params;
    if (obj.contains("continuous_mode")) params.continuous_mode = obj["continuous_mode"];
    if (obj.contains("frequency")) params.frequency = obj["frequency"];
    if (obj.contains("time_exposure")) params.time_exposure = obj["time_exposure"];
    if (obj.contains("gain")) params.gain = obj["gain"];
    return params;
}

nlohmann::json ScanPreset::tcu_params_to_json(const TCUParams& params) const
{
    nlohmann::json obj;
    obj["PRF"] = params.PRF;
    obj["laser_width"] = params.laser_width;
    obj["delay_a"] = params.delay_a;
    obj["delay_b"] = params.delay_b;
    obj["gw_a"] = params.gw_a;
    obj["gw_b"] = params.gw_b;
    obj["mcp"] = params.mcp;
    obj["type"] = params.type;
    return obj;
}

ScanPreset::TCUParams ScanPreset::tcu_params_from_json(const nlohmann::json& obj) const
{
    TCUParams params;
    if (obj.contains("PRF")) params.PRF = obj["PRF"];
    if (obj.contains("laser_width")) params.laser_width = obj["laser_width"];
    if (obj.contains("delay_a")) params.delay_a = obj["delay_a"];
    if (obj.contains("delay_b")) params.delay_b = obj["delay_b"];
    if (obj.contains("gw_a")) params.gw_a = obj["gw_a"];
    if (obj.contains("gw_b")) params.gw_b = obj["gw_b"];
    if (obj.contains("mcp")) params.mcp = obj["mcp"];
    if (obj.contains("type")) params.type = obj["type"];
    return params;
}

nlohmann::json ScanPreset::ptz_position_to_json(const PTZStartingPosition& pos) const
{
    nlohmann::json obj;
    obj["starting_h"] = pos.starting_h;
    obj["starting_v"] = pos.starting_v;
    obj["speed"] = pos.speed;
    return obj;
}

ScanPreset::PTZStartingPosition ScanPreset::ptz_position_from_json(const nlohmann::json& obj) const
{
    PTZStartingPosition pos;
    if (obj.contains("starting_h")) pos.starting_h = obj["starting_h"];
    if (obj.contains("starting_v")) pos.starting_v = obj["starting_v"];
    if (obj.contains("speed")) pos.speed = obj["speed"];
    return pos;
}

nlohmann::json ScanPreset::scan_params_to_json(const ScanParameters& params) const
{
    nlohmann::json obj;

    // TCU scan parameters
    nlohmann::json tcu_scan;
    tcu_scan["starting_delay"] = params.tcu_scan.starting_delay;
    tcu_scan["ending_delay"] = params.tcu_scan.ending_delay;
    tcu_scan["starting_gw"] = params.tcu_scan.starting_gw;
    tcu_scan["ending_gw"] = params.tcu_scan.ending_gw;
    tcu_scan["step_size_delay"] = params.tcu_scan.step_size_delay;
    tcu_scan["step_size_gw"] = params.tcu_scan.step_size_gw;
    tcu_scan["frame_count"] = params.tcu_scan.frame_count;
    tcu_scan["rep_freq"] = params.tcu_scan.rep_freq;
    obj["tcu_scan"] = tcu_scan;

    // PTZ scan parameters
    nlohmann::json ptz_scan;
    ptz_scan["starting_h"] = params.ptz_scan.starting_h;
    ptz_scan["ending_h"] = params.ptz_scan.ending_h;
    ptz_scan["starting_v"] = params.ptz_scan.starting_v;
    ptz_scan["ending_v"] = params.ptz_scan.ending_v;
    ptz_scan["step_h"] = params.ptz_scan.step_h;
    ptz_scan["step_v"] = params.ptz_scan.step_v;
    ptz_scan["count_h"] = params.ptz_scan.count_h;
    ptz_scan["count_v"] = params.ptz_scan.count_v;
    ptz_scan["direction"] = params.ptz_scan.direction;
    ptz_scan["wait_time"] = params.ptz_scan.wait_time;
    ptz_scan["num_single_pos"] = params.ptz_scan.num_single_pos;
    obj["ptz_scan"] = ptz_scan;

    // Capture options
    nlohmann::json capture_options;
    capture_options["capture_scan_ori"] = params.capture_options.capture_scan_ori;
    capture_options["capture_scan_res"] = params.capture_options.capture_scan_res;
    capture_options["record_scan_ori"] = params.capture_options.record_scan_ori;
    capture_options["record_scan_res"] = params.capture_options.record_scan_res;
    obj["capture_options"] = capture_options;

    return obj;
}

ScanPreset::ScanParameters ScanPreset::scan_params_from_json(const nlohmann::json& obj) const
{
    ScanParameters params;

    // TCU scan parameters
    if (obj.contains("tcu_scan")) {
        const auto& tcu = obj["tcu_scan"];
        if (tcu.contains("starting_delay")) params.tcu_scan.starting_delay = tcu["starting_delay"];
        if (tcu.contains("ending_delay")) params.tcu_scan.ending_delay = tcu["ending_delay"];
        if (tcu.contains("starting_gw")) params.tcu_scan.starting_gw = tcu["starting_gw"];
        if (tcu.contains("ending_gw")) params.tcu_scan.ending_gw = tcu["ending_gw"];
        if (tcu.contains("step_size_delay")) params.tcu_scan.step_size_delay = tcu["step_size_delay"];
        if (tcu.contains("step_size_gw")) params.tcu_scan.step_size_gw = tcu["step_size_gw"];
        if (tcu.contains("frame_count")) params.tcu_scan.frame_count = tcu["frame_count"];
        if (tcu.contains("rep_freq")) params.tcu_scan.rep_freq = tcu["rep_freq"];
    }

    // PTZ scan parameters
    if (obj.contains("ptz_scan")) {
        const auto& ptz = obj["ptz_scan"];
        if (ptz.contains("starting_h")) params.ptz_scan.starting_h = ptz["starting_h"];
        if (ptz.contains("ending_h")) params.ptz_scan.ending_h = ptz["ending_h"];
        if (ptz.contains("starting_v")) params.ptz_scan.starting_v = ptz["starting_v"];
        if (ptz.contains("ending_v")) params.ptz_scan.ending_v = ptz["ending_v"];
        if (ptz.contains("step_h")) params.ptz_scan.step_h = ptz["step_h"];
        if (ptz.contains("step_v")) params.ptz_scan.step_v = ptz["step_v"];
        if (ptz.contains("count_h")) params.ptz_scan.count_h = ptz["count_h"];
        if (ptz.contains("count_v")) params.ptz_scan.count_v = ptz["count_v"];
        if (ptz.contains("direction")) params.ptz_scan.direction = ptz["direction"];
        if (ptz.contains("wait_time")) params.ptz_scan.wait_time = ptz["wait_time"];
        if (ptz.contains("num_single_pos")) params.ptz_scan.num_single_pos = ptz["num_single_pos"];
    }

    // Capture options
    if (obj.contains("capture_options")) {
        const auto& cap = obj["capture_options"];
        if (cap.contains("capture_scan_ori")) params.capture_options.capture_scan_ori = cap["capture_scan_ori"];
        if (cap.contains("capture_scan_res")) params.capture_options.capture_scan_res = cap["capture_scan_res"];
        if (cap.contains("record_scan_ori")) params.capture_options.record_scan_ori = cap["record_scan_ori"];
        if (cap.contains("record_scan_res")) params.capture_options.record_scan_res = cap["record_scan_res"];
    }

    return params;
}

void ScanPreset::set_error(const QString& error) const
{
    m_last_error = error;
    qWarning() << "ScanPreset error:" << error;
}

void ScanPreset::clear_error() const
{
    m_last_error.clear();
}
