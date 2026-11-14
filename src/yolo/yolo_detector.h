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
    int class_id;
    float confidence;
    cv::Rect box;
};

class YoloDetector
{
public:
    YoloDetector();
    ~YoloDetector();

    // config_ini_path is the yolov8 config.ini with the general parameters
    bool initialize(const QString &config_ini_path, const QString &model_path, const QString &classes_file);

    bool is_initialized() const { return m_initialized; }

    // image is BGR
    bool run(const cv::Mat &image, std::vector<YoloResult> &results);

    const std::vector<std::string> &get_class_names() const { return m_class_names; }

private:
    bool load_class_names(const QString &filepath);

    DCSP_CORE *m_core;
    std::vector<std::string> m_class_names;
    bool m_initialized;
};

#endif // YOLO_DETECTOR_H
