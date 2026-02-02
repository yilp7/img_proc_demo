#include "yolo_detector.h"
#include "inference.h"
#include "yolo_app_config.h"
#include <QDebug>
#include <fstream>

YoloDetector::YoloDetector()
    : m_core(nullptr)
    , m_initialized(false)
{
}

YoloDetector::~YoloDetector()
{
    if (m_core) {
        delete m_core;
        m_core = nullptr;
    }
}

bool YoloDetector::initialize(const QString& config_ini_path)
{
    if (m_initialized) {
        return true;
    }

    // Load AppConfig from INI file
    AppConfig app_config;
    if (!app_config.loadFromFile(config_ini_path.toStdString())) {
        qWarning() << "YoloDetector: Failed to load config from" << config_ini_path;
        return false;
    }

    // Resolve paths relative to config file directory
    QDir config_dir = QFileInfo(config_ini_path).absoluteDir();
    QString model_path = config_dir.filePath(QString::fromStdString(app_config.modelPath));
    QString classes_path = config_dir.filePath(QString::fromStdString(app_config.classesFile));

    // Check if model file exists
    if (!QFileInfo::exists(model_path)) {
        qWarning() << "YoloDetector: Model file not found:" << model_path;
        return false;
    }

    // Load class names
    if (!loadClassNames(classes_path.toStdString())) {
        qWarning() << "YoloDetector: Failed to load class names from" << classes_path;
        // Continue without class names - not fatal
    }

    // Initialize DCSP_CORE
    m_core = new DCSP_CORE();

    DCSP_INIT_PARAM init_param;
    init_param.ModelPath = model_path.toStdString();
    init_param.ModelType = app_config.modelType;
    init_param.imgSize = {app_config.inputWidth, app_config.inputHeight};
    init_param.RectConfidenceThreshold = app_config.confidenceThreshold;
    init_param.iouThreshold = app_config.iouThreshold;
    init_param.CudaEnable = app_config.useCuda;
    init_param.LogSeverityLevel = 3;  // Warning level
    init_param.IntraOpNumThreads = 1;

    char* err = m_core->CreateSession(init_param);
    if (err != RET_OK) {
        qWarning() << "YoloDetector: Failed to create ONNX session:" << err;
        delete m_core;
        m_core = nullptr;
        return false;
    }

    // Set class names in core
    m_core->classes = m_class_names;

    m_initialized = true;
    qDebug() << "YoloDetector: Initialized successfully with model:" << model_path;
    return true;
}

bool YoloDetector::run(const cv::Mat& image, std::vector<YoloResult>& results)
{
    results.clear();

    if (!m_initialized || !m_core) {
        qWarning() << "YoloDetector: Not initialized";
        return false;
    }

    if (image.empty()) {
        qWarning() << "YoloDetector: Input image is empty";
        return false;
    }

    try {
        // Create a copy to avoid modifying original
        cv::Mat image_copy = image.clone();

        // Run inference
        std::vector<DCSP_RESULT> dcsp_results;
        char* err = m_core->RunSession(image_copy, dcsp_results);

        if (err != RET_OK) {
            qWarning() << "YoloDetector: Inference failed:" << err;
            return false;
        }

        // Convert DCSP_RESULT to YoloResult
        results.reserve(dcsp_results.size());
        for (const auto& r : dcsp_results) {
            YoloResult yr;
            yr.classId = r.classId;
            yr.confidence = r.confidence;
            yr.box = r.box;
            results.push_back(yr);
        }

        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "YoloDetector: Exception during inference:" << e.what();
        return false;
    }
}

bool YoloDetector::loadClassNames(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    m_class_names.clear();
    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start != std::string::npos && end != std::string::npos) {
            m_class_names.push_back(line.substr(start, end - start + 1));
        }
    }

    return !m_class_names.empty();
}
