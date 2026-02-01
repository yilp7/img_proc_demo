#ifndef AUTOSCAN_H
#define AUTOSCAN_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QStringList>
#include <QCoreApplication>
#include "scanpreset.h"

// Forward declarations
class UserPanel;
class Config;

class AutoScan : public QObject
{
    Q_OBJECT

public:
    enum AutoScanState {
        IDLE,
        INITIALIZING_CAMERA,
        LOADING_PRESET,
        CONFIGURING_DEVICES,
        STARTING_SCAN,
        SCANNING,
        COMPLETED,
        SCAN_ERROR
    };

    struct AutoScanConfig {
        bool enabled = false;
        int startup_delay = 5000;  // ms
        bool verify_devices = true;
        bool create_timestamped_folders = true;
        QString output_directory;
    };

public:
    explicit AutoScan(QObject *parent = nullptr);
    ~AutoScan();

    // Main control functions
    bool is_auto_scan_enabled(const QStringList& args) const;
    QString get_preset_path(const QStringList& args) const;
    bool start_auto_scan_sequence(const QString& preset_path);
    void stop_auto_scan();

    // State management
    AutoScanState get_state() const { return m_state; }
    QString get_state_string() const;
    QString get_last_error() const { return m_last_error; }

    // Configuration
    void set_config(const AutoScanConfig& config) { m_config = config; }
    const AutoScanConfig& get_config() const { return m_config; }

    // Integration points
    void set_config_manager(Config* config) { m_config_manager = config; }
    void set_command_line_args(const QStringList& args) { m_command_line_args = args; }

public slots:
    void on_initialization_complete();
    void on_scan_preset_applied_to_devices();

private slots:
    void on_startup_delay_timeout();
    void on_preset_loaded(const QString& name);
    void on_preset_validation_failed(const QString& error);

private:
    // Auto-scan sequence steps
    bool auto_initialize_camera();
    bool auto_load_scan_preset(const QString& preset_path);
    bool auto_configure_devices();
    bool auto_start_scan();

    // Helper functions
    void set_state(AutoScanState state);
    void set_error(const QString& error);
    void clear_error();
    bool verify_prerequisites() const;
    QString create_output_directory() const;

private:
    AutoScanState m_state;
    AutoScanConfig m_config;
    QString m_last_error;
    QString m_current_preset_path;
    QStringList m_command_line_args;

    // Components
    ScanPreset* m_scan_preset;
    QTimer* m_startup_timer;
    
    // Integration points
    Config* m_config_manager;

signals:
    void auto_scan_started(const QString& preset_name);
    void auto_scan_completed();
    void auto_scan_failed(const QString& error);
    void state_changed(AutoScanState new_state);
    
    // Integration signals for UserPanel
    void trigger_camera_initialization();
    void trigger_scan_start();
    void apply_scan_preset_to_devices(const ScanPreset::ScanPresetData& data);
};

#endif // AUTOSCAN_H