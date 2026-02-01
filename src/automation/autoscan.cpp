#include "autoscan.h"
#include "visual/userpanel.h"
#include "util/config.h"
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QCoreApplication>

AutoScan::AutoScan(QObject *parent)
    : QObject(parent)
    , m_state(IDLE)
    , m_scan_preset(nullptr)
    , m_startup_timer(nullptr)
    , m_config_manager(nullptr)
{
    m_scan_preset = new ScanPreset(this);
    m_startup_timer = new QTimer(this);
    m_startup_timer->setSingleShot(true);

    // Connect signals
    connect(m_scan_preset, &ScanPreset::scan_preset_loaded,
            this, &AutoScan::on_preset_loaded);
    connect(m_scan_preset, &ScanPreset::scan_preset_validation_failed,
            this, &AutoScan::on_preset_validation_failed);
    connect(m_startup_timer, &QTimer::timeout,
            this, &AutoScan::on_startup_delay_timeout);
}

AutoScan::~AutoScan()
{
}

bool AutoScan::is_auto_scan_enabled(const QStringList& args) const
{
    return args.contains("--auto-scan");
}

QString AutoScan::get_preset_path(const QStringList& args) const
{
    int presetIndex = args.indexOf("--scan-preset");
    if (presetIndex >= 0 && presetIndex + 1 < args.size()) {
        return args.at(presetIndex + 1);  // Custom path provided
    }
    return QCoreApplication::applicationDirPath() + "/scan_preset.json";  // Default path
}

bool AutoScan::start_auto_scan_sequence(const QString& preset_path)
{
    clear_error();

    if (m_state != IDLE) {
        set_error("Auto-scan already in progress");
        return false;
    }

    m_current_preset_path = preset_path;

    // Check if preset file exists
    if (!QFile::exists(preset_path)) {
        set_error(QString("Preset file not found: %1").arg(preset_path));
        return false;
    }

    // Verify prerequisites
    if (!verify_prerequisites()) {
        return false;
    }

    qDebug() << "Starting auto-scan sequence with preset:" << preset_path;

    // Start with optional delay
    if (m_config.startup_delay > 0) {
        qDebug() << "Applying startup delay:" << m_config.startup_delay << "ms";
        set_state(LOADING_PRESET);
        m_startup_timer->start(m_config.startup_delay);
    } else {
        on_startup_delay_timeout();
    }

    return true;
}

void AutoScan::stop_auto_scan()
{
    if (m_state == IDLE) {
        return;
    }

    qDebug() << "Stopping auto-scan sequence";

    m_startup_timer->stop();
    set_state(IDLE);
    clear_error();
}

QString AutoScan::get_state_string() const
{
    switch (m_state) {
        case IDLE: return "Idle";
        case INITIALIZING_CAMERA: return "Initializing Camera";
        case LOADING_PRESET: return "Loading Preset";
        case CONFIGURING_DEVICES: return "Configuring Devices";
        case STARTING_SCAN: return "Starting Scan";
        case SCANNING: return "Scanning";
        case COMPLETED: return "Completed";
        case SCAN_ERROR: return "Error";
        default: return "Unknown";
    }
}

void AutoScan::on_startup_delay_timeout()
{
    if (m_state != LOADING_PRESET) {
        return;
    }


    if (!auto_load_scan_preset(m_current_preset_path)) {
        set_state(SCAN_ERROR);
        emit auto_scan_failed(m_last_error);
        return;
    }
}

void AutoScan::on_preset_loaded(const QString& name)
{
    qDebug() << "Preset loaded successfully:" << name;
    emit auto_scan_started(name);

    // Move to camera initialization
    set_state(INITIALIZING_CAMERA);

    if (!auto_initialize_camera()) {
        set_state(SCAN_ERROR);
        emit auto_scan_failed(m_last_error);
        return;
    }

    // Move to device configuration
    set_state(CONFIGURING_DEVICES);

    if (!auto_configure_devices()) {
        set_state(SCAN_ERROR);
        emit auto_scan_failed(m_last_error);
        return;
    }
}

void AutoScan::on_preset_validation_failed(const QString& error)
{
    set_error(QString("Preset validation failed: %1").arg(error));
    set_state(SCAN_ERROR);
    emit auto_scan_failed(m_last_error);
}

void AutoScan::on_initialization_complete()
{
    // Check if auto-scan is enabled via command line
    if (is_auto_scan_enabled(m_command_line_args)) {
        QString presetPath = get_preset_path(m_command_line_args);
        qDebug() << "Auto-scan enabled with preset:" << presetPath;

        // Configure auto-scan
        AutoScanConfig config;
        config.enabled = true;
        config.startup_delay = 2000;  // 2 second delay after UI initialization
        config.verify_devices = true;
        config.create_timestamped_folders = true;

        // Check for custom output directory
        int outputIndex = m_command_line_args.indexOf("--output-dir");
        if (outputIndex >= 0 && outputIndex + 1 < m_command_line_args.size()) {
            config.output_directory = m_command_line_args.at(outputIndex + 1);
        }

        set_config(config);

        // Start auto-scan sequence
        if (!start_auto_scan_sequence(presetPath)) {
            qWarning() << "Failed to start auto-scan:" << get_last_error();
        }
    } else {
        qDebug() << "Auto-scan not enabled - starting normal operation";
    }
}

void AutoScan::on_scan_preset_applied_to_devices()
{
    qDebug() << "Preset applied to devices successfully";

    set_state(STARTING_SCAN);

    if (!auto_start_scan()) {
        set_state(SCAN_ERROR);
        emit auto_scan_failed(m_last_error);
        return;
    }

    set_state(SCANNING);
}

bool AutoScan::auto_initialize_camera()
{
    qDebug() << "Auto-initializing camera...";

    emit trigger_camera_initialization();

    return true;
}

bool AutoScan::auto_load_scan_preset(const QString& preset_path)
{
    qDebug() << "Auto-loading scan preset:" << preset_path;

    // Load the raw data from file
    if (!m_scan_preset->load_from_file(preset_path)) {
        set_error(QString("Failed to load preset: %1").arg(m_scan_preset->get_last_error()));
        return false;
    }

    // Trigger validation - this will emit scan_preset_loaded or scan_preset_validation_failed
    if (!m_scan_preset->validate_preset()) {
        set_error(QString("Preset validation failed: %1").arg(m_scan_preset->get_last_error()));
        return false;
    }

    return true;
}

bool AutoScan::auto_configure_devices()
{
    if (!m_scan_preset->is_valid()) {
        set_error("Invalid preset data");
        return false;
    }

    qDebug() << "Auto-configuring devices...";

    // Verify preset is ready for application
    if (!m_scan_preset->apply_preset()) {
        set_error(QString("Failed to prepare preset: %1").arg(m_scan_preset->get_last_error()));
        return false;
    }

    // Emit signal to UserPanel to actually apply the preset data to devices
    qDebug() << "Emitting signal to apply preset data to devices";
    emit apply_scan_preset_to_devices(m_scan_preset->get_data());

    return true;
}

bool AutoScan::auto_start_scan()
{
    qDebug() << "Auto-starting scan...";

    // Create output directory if needed
    if (m_config.create_timestamped_folders) {
        QString output_dir = create_output_directory();
        if (output_dir.isEmpty()) {
            set_error("Failed to create output directory");
            return false;
        }
        qDebug() << "Created output directory:" << output_dir;
    }

    qDebug() << "Triggering scan start...";

    emit trigger_scan_start();

    qDebug() << "Auto-scan sequence initiated successfully";

    return true;
}

bool AutoScan::verify_prerequisites() const
{
    // TODO: Add prerequisite checks if needed:
    // - Disk space
    // - Permissions
    // Other checks will be handled by the receiving components via signals

    return true;
}

QString AutoScan::create_output_directory() const
{
    QString base_dir = m_config.output_directory;
    if (base_dir.isEmpty()) {
        base_dir = QCoreApplication::applicationDirPath() + "/scan_results";
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString preset_name = m_scan_preset->get_data().name;
    preset_name.replace(" ", "_");  // Replace spaces with underscores

    QString full_dir = QString("%1/%2_%3").arg(base_dir, timestamp, preset_name);

    QDir dir;
    if (dir.mkpath(full_dir)) {
        return full_dir;
    }

    return QString();
}

void AutoScan::set_state(AutoScanState state)
{
    if (m_state != state) {
        AutoScanState old_state = m_state;
        m_state = state;
        qDebug() << "AutoScan state changed:" << get_state_string();
        emit state_changed(state);
    }
}

void AutoScan::set_error(const QString& error)
{
    m_last_error = error;
    qWarning() << "AutoScan error:" << error;
}

void AutoScan::clear_error()
{
    m_last_error.clear();
}
