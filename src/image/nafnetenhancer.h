#ifndef NAFNETENHANCER_H
#define NAFNETENHANCER_H

#include <QString>

#include <opencv2/core.hpp>

#include <memory>

#include "onnxruntime_cxx_api.h"

// LightNAFNetGray wrapped as a frame-average / denoise option. The model is
// single-channel, so a colour frame takes one inference per channel. Preprocessing
// follows others/jiaofu/MODEL_CONTRACT.md in full-frame mode, so output size equals input.
class NafNetEnhancer
{
public:
    NafNetEnhancer();
    ~NafNetEnhancer();

    // Deployment location, relative to the executable like the YOLO models.
    static QString default_model_path();
    // Pre-flight for the GUI, which hides the option entirely when it cannot
    // run. Covers only what is knowable without loading the model: a provider
    // that is listed but whose CUDA libraries fail to load still fails later.
    static bool supported(const QString &model_path);

    // Loads on first call. Failure is sticky: this sits on the per-frame path
    // and must not retry a missing model or an absent CUDA stack every frame.
    bool ensure_loaded(const QString &model_path);
    bool is_available() const { return m_session != nullptr; }
    QString get_last_error() const { return m_error; }

    // In place on CV_8UC1 or CV_8UC3. Returns false and leaves the image
    // untouched when the model is unavailable or the format is unsupported.
    bool enhance(cv::Mat &image);

private:
    bool run_plane(const cv::Mat &plane, cv::Mat &out);

    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    Ort::SessionOptions m_options;
    bool m_load_failed = false;
    QString m_error;
};

#endif // NAFNETENHANCER_H
