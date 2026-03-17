#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include "util/config.h"

class TestConfig : public QObject
{
    Q_OBJECT

private slots:
    // TC-CF-001: Default values after construction
    void defaultValuesAfterConstruction()
    {
        Config config;
        const Config::ConfigData &d = config.get_data();

        // COM defaults
        QCOMPARE(d.com_tcu.port, QString("COM0"));
        QCOMPARE(d.com_tcu.baudrate, 9600);

        // Network defaults
        QCOMPARE(d.network.tcp_server_address, QString("192.168.1.233"));

        // UI defaults
        QCOMPARE(d.ui.dark_theme, true);
        QCOMPARE(d.ui.english, true);

        // Camera defaults
        QCOMPARE(d.camera.time_exposure, 95000.0f);

        // TCU defaults
        QCOMPARE(d.tcu.type, 0);
        QCOMPARE(d.tcu.ab_lock, true);
        QCOMPARE(d.tcu.max_dist, 15000.0f);

        // Device defaults
        QCOMPARE(d.device.ptz_type, 0);

        // YOLO defaults
        QCOMPARE(d.yolo.config_path, QString("yolo_config.ini"));
    }

    // TC-CF-002: JSON round-trip
    void jsonRoundTrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/config_rt.json";

        // Save modified config
        Config config1;
        config1.get_data().com_tcu.port = "COM3";
        config1.get_data().com_tcu.baudrate = 115200;
        config1.get_data().network.udp_target_port = 40000;
        config1.get_data().ui.dark_theme = false;
        config1.get_data().camera.frequency = 30;
        config1.get_data().tcu.type = 2;
        QVERIFY(config1.save_to_file(tmpFile));

        // Load into a fresh config
        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));

        const Config::ConfigData &d = config2.get_data();
        QCOMPARE(d.com_tcu.port, QString("COM3"));
        QCOMPARE(d.com_tcu.baudrate, 115200);
        QCOMPARE(d.network.udp_target_port, 40000);
        QCOMPARE(d.ui.dark_theme, false);
        QCOMPARE(d.camera.frequency, 30);
        QCOMPARE(d.tcu.type, 2);
    }

    // TC-CF-003: Partial JSON - missing fields preserve defaults
    void partialJsonPreservesDefaults()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/partial.json";

        // Write minimal JSON with only version and ui.dark_theme
        QFile file(tmpFile);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "{\"version\":\"1.0.0\",\"ui\":{\"dark_theme\":false}}";
        file.close();

        Config config;
        QVERIFY(config.load_from_file(tmpFile));

        const Config::ConfigData &d = config.get_data();

        // Explicitly set field
        QCOMPARE(d.ui.dark_theme, false);

        // All other fields should remain at defaults
        QCOMPARE(d.com_tcu.port, QString("COM0"));
        QCOMPARE(d.com_tcu.baudrate, 9600);
        QCOMPARE(d.network.tcp_server_address, QString("192.168.1.233"));
        QCOMPARE(d.network.udp_target_port, 30000);
        QCOMPARE(d.ui.english, true);
        QCOMPARE(d.ui.simplified, false);
        QCOMPARE(d.camera.time_exposure, 95000.0f);
        QCOMPARE(d.camera.frequency, 10);
        QCOMPARE(d.tcu.type, 0);
        QCOMPARE(d.tcu.ab_lock, true);
        QCOMPARE(d.tcu.max_dist, 15000.0f);
        QCOMPARE(d.device.ptz_type, 0);
        QCOMPARE(d.yolo.config_path, QString("yolo_config.ini"));
    }

    // TC-CF-004: Invalid JSON
    void invalidJson()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/invalid.json";

        QFile file(tmpFile);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "{invalid json";
        file.close();

        Config config;
        QVERIFY(!config.load_from_file(tmpFile));
    }

    // TC-CF-005: Non-existent file
    void nonExistentFile()
    {
        Config config;
        QVERIFY(!config.load_from_file("/nonexistent/path.json"));
    }

    // TC-CF-006: ps_step array round-trip
    void psStepArrayRoundTrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/ps_step.json";

        Config config1;
        config1.get_data().tcu.ps_step[0] = 10;
        config1.get_data().tcu.ps_step[1] = 20;
        config1.get_data().tcu.ps_step[2] = 30;
        config1.get_data().tcu.ps_step[3] = 40;
        QVERIFY(config1.save_to_file(tmpFile));

        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));

        QCOMPARE(config2.get_data().tcu.ps_step[0], 10u);
        QCOMPARE(config2.get_data().tcu.ps_step[1], 20u);
        QCOMPARE(config2.get_data().tcu.ps_step[2], 30u);
        QCOMPARE(config2.get_data().tcu.ps_step[3], 40u);
    }

    // TC-CF-007: YOLO paths round-trip
    void yoloPathsRoundTrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/yolo.json";

        Config config1;
        config1.get_data().yolo.config_path = "custom/yolo.ini";
        config1.get_data().yolo.visible_model_path = "custom/visible.onnx";
        config1.get_data().yolo.visible_classes_file = "custom/visible.txt";
        config1.get_data().yolo.thermal_model_path = "custom/thermal.onnx";
        config1.get_data().yolo.thermal_classes_file = "custom/thermal.txt";
        config1.get_data().yolo.gated_model_path = "custom/gated.onnx";
        config1.get_data().yolo.gated_classes_file = "custom/gated.txt";
        QVERIFY(config1.save_to_file(tmpFile));

        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));

        const Config::YoloSettings &y = config2.get_data().yolo;
        QCOMPARE(y.config_path, QString("custom/yolo.ini"));
        QCOMPARE(y.visible_model_path, QString("custom/visible.onnx"));
        QCOMPARE(y.visible_classes_file, QString("custom/visible.txt"));
        QCOMPARE(y.thermal_model_path, QString("custom/thermal.onnx"));
        QCOMPARE(y.thermal_classes_file, QString("custom/thermal.txt"));
        QCOMPARE(y.gated_model_path, QString("custom/gated.onnx"));
        QCOMPARE(y.gated_classes_file, QString("custom/gated.txt"));
    }

    // TC-CF-008: load_defaults() reset
    void loadDefaultsReset()
    {
        Config config;

        // Modify several fields away from defaults
        config.get_data().com_tcu.port = "COM5";
        config.get_data().com_tcu.baudrate = 57600;
        config.get_data().network.tcp_server_address = "10.0.0.1";
        config.get_data().ui.dark_theme = false;
        config.get_data().ui.english = false;
        config.get_data().camera.frequency = 60;
        config.get_data().camera.time_exposure = 5000.0f;
        config.get_data().tcu.type = 3;
        config.get_data().tcu.ab_lock = false;
        config.get_data().tcu.max_dist = 9999.0f;
        config.get_data().device.ptz_type = 2;
        config.get_data().yolo.config_path = "other.ini";

        // Reset to defaults
        config.load_defaults();

        const Config::ConfigData &d = config.get_data();
        QCOMPARE(d.com_tcu.port, QString("COM0"));
        QCOMPARE(d.com_tcu.baudrate, 9600);
        QCOMPARE(d.network.tcp_server_address, QString("192.168.1.233"));
        QCOMPARE(d.ui.dark_theme, true);
        QCOMPARE(d.ui.english, true);
        QCOMPARE(d.camera.frequency, 10);
        QCOMPARE(d.camera.time_exposure, 95000.0f);
        QCOMPARE(d.tcu.type, 0);
        QCOMPARE(d.tcu.ab_lock, true);
        QCOMPARE(d.tcu.max_dist, 15000.0f);
        QCOMPARE(d.device.ptz_type, 0);
        QCOMPARE(d.yolo.config_path, QString("yolo_config.ini"));
    }

    // TC-CF-009: Version field persistence
    void versionFieldPersistence()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/version.json";

        Config config1;
        QString originalVersion = config1.get_version();
        QVERIFY(!originalVersion.isEmpty());
        QVERIFY(config1.save_to_file(tmpFile));

        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));
        QCOMPARE(config2.get_version(), originalVersion);
    }

    // TC-CF-010: ImageProcSettings round-trip
    void imageProcSettingsRoundTrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/imgproc.json";

        Config config1;
        auto &ip = config1.get_data().image_proc;
        ip.accu_base = 2.5f;
        ip.gamma = 0.8f;
        ip.low_in = 0.1f;
        ip.high_in = 0.9f;
        ip.dehaze_pct = 0.75f;
        ip.colormap = 4;
        ip.lower_3d_thresh = 0.05;
        ip.upper_3d_thresh = 0.95;
        ip.truncate_3d = true;
        ip.custom_3d_param = true;
        ip.custom_3d_delay = 100.0f;
        ip.custom_3d_gate_width = 50.0f;
        ip.ecc_backward = 10;
        ip.ecc_forward = 5;
        ip.ecc_levels = 3;
        ip.ecc_max_iter = 16;
        ip.ecc_eps = 0.0001;
        QVERIFY(config1.save_to_file(tmpFile));

        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));
        const auto &ip2 = config2.get_data().image_proc;
        QCOMPARE(ip2.accu_base, 2.5f);
        QCOMPARE(ip2.gamma, 0.8f);
        QCOMPARE(ip2.low_in, 0.1f);
        QCOMPARE(ip2.high_in, 0.9f);
        QCOMPARE(ip2.dehaze_pct, 0.75f);
        QCOMPARE(ip2.colormap, 4);
        QCOMPARE(ip2.lower_3d_thresh, 0.05);
        QCOMPARE(ip2.upper_3d_thresh, 0.95);
        QCOMPARE(ip2.truncate_3d, true);
        QCOMPARE(ip2.custom_3d_param, true);
        QCOMPARE(ip2.custom_3d_delay, 100.0f);
        QCOMPARE(ip2.custom_3d_gate_width, 50.0f);
        QCOMPARE(ip2.ecc_backward, 10);
        QCOMPARE(ip2.ecc_forward, 5);
        QCOMPARE(ip2.ecc_levels, 3);
        QCOMPARE(ip2.ecc_max_iter, 16);
        QVERIFY(qAbs(ip2.ecc_eps - 0.0001) < 1e-6);
    }

    // TC-CF-011: SaveSettings round-trip
    void saveSettingsRoundTrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/save.json";

        Config config1;
        auto &s = config1.get_data().save;
        s.save_info = false;
        s.custom_topleft_info = true;
        s.save_in_grayscale = true;
        s.consecutive_capture = false;
        s.integrate_info = false;
        s.img_format = 2;
        QVERIFY(config1.save_to_file(tmpFile));

        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));
        const auto &s2 = config2.get_data().save;
        QCOMPARE(s2.save_info, false);
        QCOMPARE(s2.custom_topleft_info, true);
        QCOMPARE(s2.save_in_grayscale, true);
        QCOMPARE(s2.consecutive_capture, false);
        QCOMPARE(s2.integrate_info, false);
        QCOMPARE(s2.img_format, 2);
    }

    // TC-CF-012: DeviceSettings flip int round-trip (backward compat)
    void deviceFlipIntRoundTrip()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/device.json";

        Config config1;
        config1.get_data().device.flip = 2;
        config1.get_data().device.rotation = 90;
        config1.get_data().device.pixel_type = 1;
        QVERIFY(config1.save_to_file(tmpFile));

        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));
        QCOMPARE(config2.get_data().device.flip, 2);
        QCOMPARE(config2.get_data().device.rotation, 90);
        QCOMPARE(config2.get_data().device.pixel_type, 1);
    }

    // TC-CF-013: ImageProcSettings defaults preserved on partial JSON
    void imageProcDefaultsOnPartialJson()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/partial_ip.json";

        // Write JSON with only image_proc.colormap set
        QFile file(tmpFile);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "{\"version\":\"1.0.0\",\"image_proc\":{\"colormap\":11}}";
        file.close();

        Config config;
        QVERIFY(config.load_from_file(tmpFile));
        const auto &ip = config.get_data().image_proc;

        QCOMPARE(ip.colormap, 11);
        // All other fields remain at defaults
        QCOMPARE(ip.accu_base, 1.0f);
        QCOMPARE(ip.gamma, 1.2f);
        QCOMPARE(ip.lower_3d_thresh, 0.0);
        QCOMPARE(ip.upper_3d_thresh, 0.981);
        QCOMPARE(ip.ecc_backward, 20);
        QCOMPARE(ip.ecc_max_iter, 8);
    }
};

QTEST_MAIN(TestConfig)
#include "test_config.moc"
