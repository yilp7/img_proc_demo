#pragma once

#define RET_OK nullptr

#ifdef _WIN32
#include <Windows.h>
#endif

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "onnxruntime_cxx_api.h"

enum MODEL_TYPE {
    YOLO_DETECT_FP32 = 0,      // YOLO Detection (FP32)
    YOLO_DETECT_FP16 = 1,      // YOLO Detection (FP16)
    YOLO_POSE = 2,             // YOLO Pose (reserved)
    YOLO_CLASSIFY = 3          // YOLO Classify (reserved)
};

typedef struct _DCSP_INIT_PARAM {
    std::string ModelPath;
    MODEL_TYPE ModelType = YOLO_DETECT_FP32;
    std::vector<int> imgSize = {640, 640};
    float RectConfidenceThreshold = 0.4;
    float iouThreshold = 0.5;
    bool CudaEnable = false;
    int LogSeverityLevel = 3;
    int IntraOpNumThreads = 1;
} DCSP_INIT_PARAM;

typedef struct _DCSP_RESULT {
    int classId;
    float confidence;
    cv::Rect box;
} DCSP_RESULT;

class DCSP_CORE {
public:
    DCSP_CORE();
    ~DCSP_CORE();

    char* CreateSession(DCSP_INIT_PARAM& iParams);
    char* RunSession(cv::Mat& iImg, std::vector<DCSP_RESULT>& oResult);

    std::vector<std::string> classes{};

private:
    char* WarmUpSession();

    template<typename N>
    char* TensorProcess(clock_t& starttime_1, cv::Mat& iImg, N& blob,
                       std::vector<int64_t>& inputNodeDims,
                       std::vector<DCSP_RESULT>& oResult);

    // YOLOv8/v9/v10/v11 output format: [1, 4+num_classes, num_boxes]
    template<typename T>
    void processYOLOv8Output(const std::vector<int64_t>& outputNodeDims, T* output,
                            cv::Mat& iImg, std::vector<DCSP_RESULT>& oResult,
                            clock_t starttime_1, clock_t starttime_2, clock_t starttime_3);

    Ort::Env env;
    Ort::Session* session = nullptr;
    bool cudaEnable = false;
    Ort::RunOptions options;
    std::vector<const char*> inputNodeNames;
    std::vector<const char*> outputNodeNames;

    MODEL_TYPE modelType;
    std::vector<int> imgSize;
    float rectConfidenceThreshold;
    float iouThreshold;
};
