#include "image/nafnetenhancer.h"

#include <QCoreApplication>
#include <QFileInfo>

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <array>
#include <vector>

namespace {

// The graph downsamples three times, so both dimensions must be multiples of 8.
constexpr int DownsampleFactor = 8;

const char *InputNames[] = {"image"};
const char *OutputNames[] = {"enhanced"};

int round_up(int size)
{
    return ((size + DownsampleFactor - 1) / DownsampleFactor) * DownsampleFactor;
}

} // namespace

NafNetEnhancer::NafNetEnhancer() = default;
NafNetEnhancer::~NafNetEnhancer() = default;

QString NafNetEnhancer::default_model_path()
{
    return QCoreApplication::applicationDirPath() + "/models/lightnafnet_gray_best.onnx";
}

bool NafNetEnhancer::supported(const QString &model_path)
{
    if (!QFileInfo::exists(model_path)) return false;
    try {
        const std::vector<std::string> providers = Ort::GetAvailableProviders();
        return std::find(providers.begin(), providers.end(), "CUDAExecutionProvider")
                != providers.end();
    } catch (const std::exception &) {
        return false;
    }
}

bool NafNetEnhancer::ensure_loaded(const QString &model_path)
{
    if (m_session) return true;
    if (m_load_failed) return false;
    m_load_failed = true;

    if (!QFileInfo::exists(model_path)) {
        m_error = QStringLiteral("model not found: %1").arg(model_path);
        return false;
    }

    try {
        // The delivery refuses to run this model on CPU, and that is the right
        // call on a per-frame display path: a CPU session would cost far more
        // than the frame interval and stall the pipeline rather than enhance it.
        const std::vector<std::string> providers = Ort::GetAvailableProviders();
        if (std::find(providers.begin(), providers.end(), "CUDAExecutionProvider")
                == providers.end()) {
            m_error = QStringLiteral("CUDAExecutionProvider unavailable; refusing CPU fallback");
            return false;
        }

        m_env.reset(new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "lightnafnet"));
        m_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        OrtCUDAProviderOptions cuda;
        cuda.device_id = 0;
        cuda.do_copy_in_default_stream = 1;
        m_options.AppendExecutionProvider_CUDA(cuda);

#ifdef _WIN32
        const std::wstring path = model_path.toStdWString();
        m_session.reset(new Ort::Session(*m_env, path.c_str(), m_options));
#else
        const QByteArray path = model_path.toUtf8();
        m_session.reset(new Ort::Session(*m_env, path.constData(), m_options));
#endif
    } catch (const std::exception &e) {
        m_error = QString::fromLocal8Bit(e.what());
        m_session.reset();
        m_env.reset();
        return false;
    }

    m_load_failed = false;
    return true;
}

bool NafNetEnhancer::run_plane(const cv::Mat &plane, cv::Mat &out)
{
    cv::Mat normalized;
    plane.convertTo(normalized, CV_32F, 1.0 / 255.0);

    // Padding is right and bottom only, replicating the edge, so the surviving
    // top-left region maps one-to-one back onto the source pixels.
    const int padded_w = round_up(plane.cols);
    const int padded_h = round_up(plane.rows);
    cv::Mat padded = normalized;
    if (padded_w != plane.cols || padded_h != plane.rows) {
        cv::copyMakeBorder(normalized, padded, 0, padded_h - plane.rows,
                           0, padded_w - plane.cols, cv::BORDER_REPLICATE);
    }
    if (!padded.isContinuous()) padded = padded.clone();

    const std::array<int64_t, 4> shape{1, 1, padded_h, padded_w};
    const Ort::MemoryInfo memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input = Ort::Value::CreateTensor<float>(
                memory, reinterpret_cast<float*>(padded.data),
                static_cast<size_t>(padded_w) * static_cast<size_t>(padded_h),
                shape.data(), shape.size());

    auto outputs = m_session->Run(Ort::RunOptions{nullptr}, InputNames, &input, 1, OutputNames, 1);

    // The result is wrapped without a copy, so a graph that did not return the
    // padded input size would be read out of bounds.
    const std::vector<int64_t> out_shape =
            outputs.front().GetTensorTypeAndShapeInfo().GetShape();
    if (out_shape.size() != 4 || out_shape[2] != padded_h || out_shape[3] != padded_w) {
        m_error = QStringLiteral("unexpected output shape from the model");
        return false;
    }

    const float *data = outputs.front().GetTensorData<float>();

    // saturate_cast inside convertTo rounds and clips, which is the contract's
    // clamp-to-[0,1] followed by the scale to 8 bits.
    const cv::Mat result(padded_h, padded_w, CV_32F, const_cast<float*>(data));
    result(cv::Rect(0, 0, plane.cols, plane.rows)).convertTo(out, CV_8U, 255.0);
    return true;
}

bool NafNetEnhancer::enhance(cv::Mat &image)
{
    if (!m_session || image.empty() || image.depth() != CV_8U) return false;
    if (image.channels() != 1 && image.channels() != 3) return false;

    try {
        if (image.channels() == 1) {
            cv::Mat enhanced;
            if (!run_plane(image, enhanced)) return false;
            image = enhanced;
            return true;
        }

        // Grayscale-only model, so each colour plane is enhanced on its own and
        // recombined. Channels are treated independently, which means a strong
        // per-channel correction can shift the frame's colour balance.
        std::vector<cv::Mat> planes;
        cv::split(image, planes);
        for (cv::Mat &plane : planes) {
            cv::Mat enhanced;
            if (!run_plane(plane, enhanced)) return false;
            plane = enhanced;
        }
        cv::merge(planes, image);
        return true;
    } catch (const std::exception &e) {
        m_error = QString::fromLocal8Bit(e.what());
        return false;
    }
}
