#include "inference.h"
#include <regex>

#define benchmark

DCSP_CORE::DCSP_CORE() = default;

DCSP_CORE::~DCSP_CORE() {
    delete session;
    // Clean up allocated node names
    for (auto name : inputNodeNames) {
        delete[] name;
    }
    for (auto name : outputNodeNames) {
        delete[] name;
    }
}

template<typename T>
char* BlobFromImage(cv::Mat& iImg, T& iBlob) {
    int channels = iImg.channels();
    int imgHeight = iImg.rows;
    int imgWidth = iImg.cols;

    for (int c = 0; c < channels; c++) {
        for (int h = 0; h < imgHeight; h++) {
            for (int w = 0; w < imgWidth; w++) {
                iBlob[c * imgWidth * imgHeight + h * imgWidth + w] =
                    typename std::remove_pointer<T>::type((iImg.at<cv::Vec3b>(h, w)[c]) / 255.0f);
            }
        }
    }
    return RET_OK;
}

char* PostProcess(cv::Mat& iImg, std::vector<int> iImgSize, cv::Mat& oImg) {
    cv::resize(iImg, oImg, cv::Size(iImgSize.at(0), iImgSize.at(1)));
    if (oImg.channels() == 1) {
        cv::cvtColor(oImg, oImg, cv::COLOR_GRAY2BGR);
    }
    cv::cvtColor(oImg, oImg, cv::COLOR_BGR2RGB);
    return RET_OK;
}

char* DCSP_CORE::CreateSession(DCSP_INIT_PARAM& iParams) {
    // Check for Chinese characters in path
    std::regex pattern("[\u4e00-\u9fa5]");
    if (std::regex_search(iParams.ModelPath, pattern)) {
        const char* error = "[DCSP_ONNX]: Model path error. Change your model path without Chinese characters.";
        std::cout << error << std::endl;
        return const_cast<char*>(error);
    }

    try {
        rectConfidenceThreshold = iParams.RectConfidenceThreshold;
        iouThreshold = iParams.iouThreshold;
        imgSize = iParams.imgSize;
        modelType = iParams.ModelType;
        cudaEnable = iParams.CudaEnable;

        env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "Yolo");
        Ort::SessionOptions sessionOption;

        if (iParams.CudaEnable) {
            std::cout << "[INFO] Attempting to initialize CUDA provider..." << std::endl;
            try {
                OrtCUDAProviderOptions cudaOption;
                cudaOption.device_id = 0;
                sessionOption.AppendExecutionProvider_CUDA(cudaOption);
                std::cout << "[SUCCESS] CUDA provider initialized successfully!" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[WARNING] Failed to initialize CUDA provider: " << e.what() << std::endl;
                std::cerr << "[INFO] Falling back to CPU execution..." << std::endl;
                cudaEnable = false;
            }
        } else {
            std::cout << "[INFO] Using CPU execution" << std::endl;
        }

        sessionOption.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        sessionOption.SetIntraOpNumThreads(iParams.IntraOpNumThreads);
        sessionOption.SetLogSeverityLevel(iParams.LogSeverityLevel);

#ifdef _WIN32
        int ModelPathSize = MultiByteToWideChar(CP_UTF8, 0, iParams.ModelPath.c_str(),
                                                static_cast<int>(iParams.ModelPath.length()), nullptr, 0);
        wchar_t* wide_cstr = new wchar_t[ModelPathSize + 1];
        MultiByteToWideChar(CP_UTF8, 0, iParams.ModelPath.c_str(),
                           static_cast<int>(iParams.ModelPath.length()), wide_cstr, ModelPathSize);
        wide_cstr[ModelPathSize] = L'\0';

        session = new Ort::Session(env, wide_cstr, sessionOption);
        delete[] wide_cstr;
#else
        session = new Ort::Session(env, iParams.ModelPath.c_str(), sessionOption);
#endif

        Ort::AllocatorWithDefaultOptions allocator;

        // Get input node names
        size_t inputNodesNum = session->GetInputCount();
        for (size_t i = 0; i < inputNodesNum; i++) {
            Ort::AllocatedStringPtr input_node_name = session->GetInputNameAllocated(i, allocator);
            char* temp_buf = new char[50];
            strcpy(temp_buf, input_node_name.get());
            inputNodeNames.push_back(temp_buf);
        }

        // Get output node names
        size_t OutputNodesNum = session->GetOutputCount();
        for (size_t i = 0; i < OutputNodesNum; i++) {
            Ort::AllocatedStringPtr output_node_name = session->GetOutputNameAllocated(i, allocator);
            char* temp_buf = new char[10];
            strcpy(temp_buf, output_node_name.get());
            outputNodeNames.push_back(temp_buf);
        }

        options = Ort::RunOptions{nullptr};
        WarmUpSession();
        return RET_OK;
    }
    catch (const std::exception& e) {
        std::string error = "[DCSP_ONNX]: " + std::string(e.what());
        std::cout << error << std::endl;
        return const_cast<char*>("[DCSP_ONNX]: Create session failed.");
    }
}

char* DCSP_CORE::RunSession(cv::Mat& iImg, std::vector<DCSP_RESULT>& oResult) {
#ifdef benchmark
    clock_t starttime_1 = clock();
#endif

    cv::Mat processedImg;
    PostProcess(iImg, imgSize, processedImg);

    // All models use the same processing
    float* blob = new float[processedImg.total() * 3];
    BlobFromImage(processedImg, blob);
    std::vector<int64_t> inputNodeDims = {1, 3, imgSize.at(0), imgSize.at(1)};
    TensorProcess(starttime_1, iImg, blob, inputNodeDims, oResult);

    return RET_OK;
}

template<typename N>
char* DCSP_CORE::TensorProcess(clock_t& starttime_1, cv::Mat& iImg, N& blob,
                               std::vector<int64_t>& inputNodeDims,
                               std::vector<DCSP_RESULT>& oResult) {
    Ort::Value inputTensor = Ort::Value::CreateTensor<typename std::remove_pointer<N>::type>(
        Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU), blob,
        3 * imgSize.at(0) * imgSize.at(1), inputNodeDims.data(), inputNodeDims.size());

#ifdef benchmark
    clock_t starttime_2 = clock();
#endif

    auto outputTensor = session->Run(options, inputNodeNames.data(), &inputTensor, 1,
                                    outputNodeNames.data(), outputNodeNames.size());

#ifdef benchmark
    clock_t starttime_3 = clock();
#endif

    Ort::TypeInfo typeInfo = outputTensor.front().GetTypeInfo();
    auto tensor_info = typeInfo.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> outputNodeDims = tensor_info.GetShape();
    auto output = outputTensor.front().GetTensorMutableData<typename std::remove_pointer<N>::type>();
    delete[] blob;

    // Process model output
    if (modelType == YOLO_DETECT_FP32 || modelType == YOLO_DETECT_FP16) {
        processYOLOv8Output(outputNodeDims, output, iImg, oResult, starttime_1, starttime_2, starttime_3);
    } else {
        std::cerr << "[ERROR] Unsupported model type: " << modelType << std::endl;
        return const_cast<char*>("[ERROR] Unsupported model type");
    }

    return RET_OK;
}

// YOLO detection model output processing (YOLOv8/v9/v10/v11 compatible, format: [1, 4+num_classes, num_boxes])
template<typename T>
void DCSP_CORE::processYOLOv8Output(const std::vector<int64_t>& outputNodeDims, T* output,
                                    cv::Mat& iImg, std::vector<DCSP_RESULT>& oResult,
                                    clock_t starttime_1, clock_t starttime_2, clock_t starttime_3) {
    int strideNum = outputNodeDims[2];
    int signalResultNum = outputNodeDims[1];
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    cv::Mat rawData;
    // Handle FP16 and FP32
    bool isHalfPrecision = (modelType == YOLO_DETECT_FP16);

    if (isHalfPrecision) {
        rawData = cv::Mat(signalResultNum, strideNum, CV_16F, output);
        rawData.convertTo(rawData, CV_32F);
    } else {
        rawData = cv::Mat(signalResultNum, strideNum, CV_32F, output);
    }

    rawData = rawData.t();
    float* data = (float*)rawData.data;

    // Calculate number of classes from model output dimensions
    // YOLO format: [x, y, w, h, class1_score, class2_score, ..., classN_score]
    int numClasses = signalResultNum - 4;  // Subtract the 4 box coordinates

    // Verify the number of classes matches expectations
    if (numClasses != this->classes.size()) {
        std::cout << "[WARNING] Model output has " << numClasses
                  << " classes but classes.txt has " << this->classes.size()
                  << " classes. Using model's class count." << std::endl;
    }

    // Calculate scale factors
    float x_factor = iImg.cols / static_cast<float>(imgSize.at(0));
    float y_factor = iImg.rows / static_cast<float>(imgSize.at(1));

    for (int i = 0; i < strideNum; ++i) {
        float* classesScores = data + 4;
        cv::Mat scores(1, numClasses, CV_32FC1, classesScores);
        cv::Point class_id;
        double maxClassScore;
        cv::minMaxLoc(scores, 0, &maxClassScore, 0, &class_id);

        if (maxClassScore > rectConfidenceThreshold) {
            confidences.push_back(maxClassScore);
            class_ids.push_back(class_id.x);

            float x = data[0];
            float y = data[1];
            float w = data[2];
            float h = data[3];

            int left = std::max(0, int((x - 0.5 * w) * x_factor));
            int top = std::max(0, int((y - 0.5 * h) * y_factor));
            int width = std::min(int(w * x_factor), iImg.cols - left);
            int height = std::min(int(h * y_factor), iImg.rows - top);

            boxes.emplace_back(left, top, width, height);
        }
        data += signalResultNum;
    }

    std::vector<int> nmsResult;
    cv::dnn::NMSBoxes(boxes, confidences, rectConfidenceThreshold, iouThreshold, nmsResult);

    for (size_t i = 0; i < nmsResult.size(); ++i) {
        int idx = nmsResult[i];
        DCSP_RESULT result;
        result.classId = class_ids[idx];
        result.confidence = confidences[idx];
        result.box = boxes[idx];
        oResult.push_back(result);
    }

#ifdef benchmark
    clock_t starttime_4 = clock();
    double pre_process_time = (double)(starttime_2 - starttime_1) / CLOCKS_PER_SEC * 1000;
    double process_time = (double)(starttime_3 - starttime_2) / CLOCKS_PER_SEC * 1000;
    double post_process_time = (double)(starttime_4 - starttime_3) / CLOCKS_PER_SEC * 1000;
    std::cout << "[DCSP_ONNX]: " << pre_process_time << "ms pre-process, "
              << process_time << "ms inference, " << post_process_time
              << "ms post-process." << std::endl;
#endif
}

char* DCSP_CORE::WarmUpSession() {
    clock_t starttime_1 = clock();
    cv::Mat iImg = cv::Mat(cv::Size(imgSize.at(0), imgSize.at(1)), CV_8UC3);
    cv::Mat processedImg;
    PostProcess(iImg, imgSize, processedImg);

    float* blob = new float[iImg.total() * 3];
    BlobFromImage(processedImg, blob);
    std::vector<int64_t> YOLO_input_node_dims = {1, 3, imgSize.at(0), imgSize.at(1)};
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU), blob,
        3 * imgSize.at(0) * imgSize.at(1),
        YOLO_input_node_dims.data(), YOLO_input_node_dims.size());
    auto output_tensors = session->Run(options, inputNodeNames.data(), &input_tensor, 1,
                                      outputNodeNames.data(), outputNodeNames.size());
    delete[] blob;

    clock_t starttime_4 = clock();
    double warm_up_time = (double)(starttime_4 - starttime_1) / CLOCKS_PER_SEC * 1000;
    if (cudaEnable) {
        std::cout << "[DCSP_ONNX(CUDA)]: Warm-up cost " << warm_up_time << " ms." << std::endl;
    }

    return RET_OK;
}
