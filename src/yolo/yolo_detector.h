#ifndef YOLO_DETECTOR_H
#define YOLO_DETECTOR_H

#include <QString>
#include <QFileInfo>
#include <QDir>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// Forward declaration - actual DCSP_CORE from external yolov8 project
class DCSP_CORE;

struct YoloResult {
    int classId;
    float confidence;
    cv::Rect box;
};

class YoloDetector {
public:
    YoloDetector();
    ~YoloDetector();

    /**
     * @brief Initialize the detector from a config.ini file
     * @param config_ini_path Path to the yolov8 config.ini file
     * @return true if initialization successful
     */
    bool initialize(const QString& config_ini_path);

    /**
     * @brief Check if detector is initialized and ready
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Run detection on an image
     * @param image Input image (BGR format)
     * @param results Output vector of detection results
     * @return true if inference successful
     */
    bool run(const cv::Mat& image, std::vector<YoloResult>& results);

    /**
     * @brief Get the class names loaded from classes.txt
     */
    const std::vector<std::string>& getClassNames() const { return m_class_names; }

private:
    bool loadClassNames(const std::string& filepath);

    DCSP_CORE* m_core;
    std::vector<std::string> m_class_names;
    bool m_initialized;
};

#endif // YOLO_DETECTOR_H
