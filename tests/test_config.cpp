#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QByteArray>
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
        QCOMPARE(d.ui.theme, QString("gruvbox"));
        QCOMPARE(d.ui.color_mode, QString("dark"));
        QCOMPARE(d.ui.dark_mode, true);
        QCOMPARE(d.ui.english, true);

        // Camera defaults
        QCOMPARE(d.camera.time_exposure, 95000.0f);

        // Video and startup automation defaults
        QCOMPARE(d.video.default_urls.size(), 3);
        QCOMPARE(d.automatic.enabled, false);
        QCOMPARE(d.debug.generate_dmp, true);

        // TCU defaults
        QCOMPARE(d.tcu.type, 0);
        QCOMPARE(d.tcu.ab_lock, true);
        QCOMPARE(d.tcu.max_dist, 15000.0f);

        // Device defaults
        QCOMPARE(d.device.ptz_type, 0);
        QCOMPARE(d.device.ptz_zero_angle_h, 0.0f);

        // Panorama optics defaults
        QCOMPARE(static_cast<int>(d.panorama_optics.zoom_fov.size()), 2);
        QCOMPARE(d.panorama_optics.zoom_fov[0].zoom, 1.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[0].focal_length_mm, 10.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[0].fov_h, 38.4f);
        QCOMPARE(d.panorama_optics.zoom_fov[0].fov_v, 22.5f);
        QCOMPARE(d.panorama_optics.zoom_fov[1].zoom, 86.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[1].focal_length_mm, 860.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[1].fov_h, 0.49f);
        QCOMPARE(d.panorama_optics.zoom_fov[1].fov_v, 0.28f);

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
        config1.get_data().ui.theme = "gruvbox";
        config1.get_data().ui.color_mode = "light";
        config1.get_data().ui.dark_mode = false;
        const QString monitor_title = QString::fromUtf8(QByteArray::fromHex("4d6f6e69746f7220e6a087e9a298"));
        config1.get_data().ui.custom_title = monitor_title;
        config1.get_data().camera.frequency = 30;
        config1.get_data().video.default_urls = QStringList()
                << "rtsp://camera-one/live"
                << "rtsp://camera-two/live";
        config1.get_data().automatic.enabled = true;
        config1.get_data().debug.generate_dmp = false;
        config1.get_data().tcu.type = 2;
        config1.get_data().device.ptz_zero_angle_h = 123.5f;
        config1.get_data().panorama_optics.zoom_fov.clear();
        config1.get_data().panorama_optics.zoom_fov.push_back(Config::ZoomFovPoint(1.0f, 12.0f, 40.0f, 24.0f));
        config1.get_data().panorama_optics.zoom_fov.push_back(Config::ZoomFovPoint(50.0f, 600.0f, 0.8f, 0.45f));
        QVERIFY(config1.save_to_file(tmpFile));

        // Load into a fresh config
        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));

        const Config::ConfigData &d = config2.get_data();
        QCOMPARE(d.com_tcu.port, QString("COM3"));
        QCOMPARE(d.com_tcu.baudrate, 115200);
        QCOMPARE(d.network.udp_target_port, 40000);
        QCOMPARE(d.ui.theme, QString("gruvbox"));
        QCOMPARE(d.ui.color_mode, QString("light"));
        QCOMPARE(d.ui.dark_mode, false);
        QCOMPARE(d.ui.custom_title, monitor_title);
        QCOMPARE(d.camera.frequency, 30);
        QCOMPARE(d.video.default_urls.size(), 2);
        QCOMPARE(d.video.default_urls.at(0), QString("rtsp://camera-one/live"));
        QCOMPARE(d.video.default_urls.at(1), QString("rtsp://camera-two/live"));
        QCOMPARE(d.automatic.enabled, true);
        QCOMPARE(d.debug.generate_dmp, false);
        QCOMPARE(d.tcu.type, 2);
        QCOMPARE(d.device.ptz_zero_angle_h, 123.5f);
        QCOMPARE(static_cast<int>(d.panorama_optics.zoom_fov.size()), 2);
        QCOMPARE(d.panorama_optics.zoom_fov[0].zoom, 1.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[0].focal_length_mm, 12.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[0].fov_h, 40.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[1].zoom, 50.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[1].focal_length_mm, 600.0f);
        QCOMPARE(d.panorama_optics.zoom_fov[1].fov_v, 0.45f);
    }

    // TC-CF-003: Partial JSON - missing fields preserve defaults
    void partialJsonPreservesDefaults()
    {
        QTemporaryDir tmpDir;
        QVERIFY(tmpDir.isValid());
        QString tmpFile = tmpDir.path() + "/partial.json";

        // Write minimal legacy JSON with only version and ui.dark_theme
        QFile file(tmpFile);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "{\"version\":\"1.0.0\",\"ui\":{\"dark_theme\":false}}";
        file.close();

        Config config;
        QVERIFY(config.load_from_file(tmpFile));

        const Config::ConfigData &d = config.get_data();

        // Explicitly set field
        QCOMPARE(d.ui.theme, QString("gruvbox"));
        QCOMPARE(d.ui.color_mode, QString("light"));
        QCOMPARE(d.ui.dark_mode, false);

        // All other fields should remain at defaults
        QCOMPARE(d.com_tcu.port, QString("COM0"));
        QCOMPARE(d.com_tcu.baudrate, 9600);
        QCOMPARE(d.network.tcp_server_address, QString("192.168.1.233"));
        QCOMPARE(d.network.udp_target_port, 30000);
        QCOMPARE(d.ui.english, true);
        QCOMPARE(d.ui.simplified, false);
        QCOMPARE(d.camera.time_exposure, 95000.0f);
        QCOMPARE(d.camera.frequency, 10);
        QCOMPARE(d.video.default_urls.size(), 3);
        QCOMPARE(d.automatic.enabled, false);
        QCOMPARE(d.debug.generate_dmp, true);
        QCOMPARE(d.tcu.type, 0);
        QCOMPARE(d.tcu.ab_lock, true);
        QCOMPARE(d.tcu.max_dist, 15000.0f);
        QCOMPARE(d.device.ptz_type, 0);
        QCOMPARE(d.device.ptz_zero_angle_h, 0.0f);
        QCOMPARE(static_cast<int>(d.panorama_optics.zoom_fov.size()), 2);
        QCOMPARE(d.panorama_optics.zoom_fov[0].fov_h, 38.4f);
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
        config.get_data().ui.theme = "other";
        config.get_data().ui.color_mode = "light";
        config.get_data().ui.dark_mode = false;
        config.get_data().ui.custom_title = QString::fromUtf8(QByteArray::fromHex("e697a7e6a087e9a298"));
        config.get_data().ui.english = false;
        config.get_data().camera.frequency = 60;
        config.get_data().camera.time_exposure = 5000.0f;
        config.get_data().video.default_urls = QStringList() << "rtsp://temporary/live";
        config.get_data().automatic.enabled = true;
        config.get_data().tcu.type = 3;
        config.get_data().tcu.ab_lock = false;
        config.get_data().tcu.max_dist = 9999.0f;
        config.get_data().device.ptz_type = 2;
        config.get_data().device.ptz_zero_angle_h = 210.0f;
        config.get_data().yolo.config_path = "other.ini";

        // Reset to defaults
        config.load_defaults();

        const Config::ConfigData &d = config.get_data();
        QCOMPARE(d.com_tcu.port, QString("COM0"));
        QCOMPARE(d.com_tcu.baudrate, 9600);
        QCOMPARE(d.network.tcp_server_address, QString("192.168.1.233"));
        QCOMPARE(d.ui.theme, QString("gruvbox"));
        QCOMPARE(d.ui.color_mode, QString("dark"));
        QCOMPARE(d.ui.dark_mode, true);
        QCOMPARE(d.ui.custom_title, QString());
        QCOMPARE(d.ui.english, true);
        QCOMPARE(d.camera.frequency, 10);
        QCOMPARE(d.camera.time_exposure, 95000.0f);
        QCOMPARE(d.video.default_urls.size(), 3);
        QCOMPARE(d.automatic.enabled, false);
        QCOMPARE(d.tcu.type, 0);
        QCOMPARE(d.tcu.ab_lock, true);
        QCOMPARE(d.tcu.max_dist, 15000.0f);
        QCOMPARE(d.device.ptz_type, 0);
        QCOMPARE(d.device.ptz_zero_angle_h, 0.0f);
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
        config1.get_data().device.ptz_zero_angle_h = 315.25f;
        QVERIFY(config1.save_to_file(tmpFile));

        Config config2;
        QVERIFY(config2.load_from_file(tmpFile));
        QCOMPARE(config2.get_data().device.flip, 2);
        QCOMPARE(config2.get_data().device.rotation, 90);
        QCOMPARE(config2.get_data().device.pixel_type, 1);
        QCOMPARE(config2.get_data().device.ptz_zero_angle_h, 315.25f);
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
