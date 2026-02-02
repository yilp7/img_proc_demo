#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <algorithm>
#include "inference.h"

/**
 * @brief Application configuration class for YOLO
 * Used to load and manage configuration parameters
 */
class AppConfig {
public:
    // Model configuration
    std::string modelPath;
    std::string classesFile;
    MODEL_TYPE modelType;

    // Path configuration
    std::string imagesPath;
    std::string outputPath;
    std::string croppedPath;

    // Detection parameters
    float confidenceThreshold;
    float iouThreshold;
    int inputWidth;
    int inputHeight;

    // Runtime settings
    bool useCuda;
    bool displayResults;
    bool saveCroppedImages;

    // Other settings
    int maxDetectionsWarning;
    bool verbose;

    // Class filter
    bool enableClassFilter;
    std::vector<int> allowedClasses;

    AppConfig() {
        // Default values
        modelPath = "./models/ship_detect_vis.onnx";
        classesFile = "./models/classes.txt";
        modelType = YOLO_DETECT_FP32;
        imagesPath = "./images";
        outputPath = "./out_image";
        croppedPath = "./crop_image";

        confidenceThreshold = 0.6f;
        iouThreshold = 0.5f;
        inputWidth = 640;
        inputHeight = 640;

        useCuda = true;
        displayResults = true;
        saveCroppedImages = true;

        maxDetectionsWarning = 100;
        verbose = true;

        // Class filter disabled by default
        enableClassFilter = false;
        allowedClasses = {};
    }

    /**
     * @brief Load configuration from INI file
     * @param filename Configuration file path
     * @return Whether load was successful
     */
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open config file: " << filename << std::endl;
            std::cerr << "Using default configuration." << std::endl;
            return false;
        }

        std::string line;
        std::string currentSection;

        while (std::getline(file, line)) {
            // Remove whitespace
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }

            // Parse section header
            if (line[0] == '[' && line[line.length() - 1] == ']') {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }

            // Parse key-value pair
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = trim(line.substr(0, pos));
                std::string value = trim(line.substr(pos + 1));

                // Remove quotes from value
                if (value.length() >= 2 && value[0] == '"' && value[value.length() - 1] == '"') {
                    value = value.substr(1, value.length() - 2);
                }

                parseValue(key, value);
            }
        }

        file.close();
        return true;
    }

    /**
     * @brief Print current configuration
     */
    void print() const {
        std::cout << "\n========================================" << std::endl;
        std::cout << "CONFIGURATION" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "[Model]" << std::endl;
        std::cout << "  Model Path:       " << modelPath << std::endl;
        std::cout << "  Classes File:     " << classesFile << std::endl;
        std::cout << "  Model Type:       " << getModelTypeName() << std::endl;
        std::cout << "\n[Paths]" << std::endl;
        std::cout << "  Images Path:      " << imagesPath << std::endl;
        std::cout << "  Output Path:      " << outputPath << std::endl;
        std::cout << "  Cropped Path:     " << croppedPath << std::endl;
        std::cout << "\n[Detection]" << std::endl;
        std::cout << "  Confidence:       " << confidenceThreshold << std::endl;
        std::cout << "  IoU Threshold:    " << iouThreshold << std::endl;
        std::cout << "  Input Size:       " << inputWidth << "x" << inputHeight << std::endl;
        std::cout << "\n[Filter]" << std::endl;
        std::cout << "  Enable Filter:    " << (enableClassFilter ? "Yes" : "No") << std::endl;
        if (enableClassFilter && !allowedClasses.empty()) {
            std::cout << "  Allowed Classes:  ";
            for (size_t i = 0; i < allowedClasses.size(); ++i) {
                std::cout << allowedClasses[i];
                if (i < allowedClasses.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        std::cout << "\n[Runtime]" << std::endl;
        std::cout << "  Use CUDA:         " << (useCuda ? "Yes" : "No") << std::endl;
        std::cout << "  Display Results:  " << (displayResults ? "Yes" : "No") << std::endl;
        std::cout << "  Save Crops:       " << (saveCroppedImages ? "Yes" : "No") << std::endl;
        std::cout << "  Verbose:          " << (verbose ? "Yes" : "No") << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

private:
    /**
     * @brief Remove leading and trailing whitespace
     */
    std::string trim(const std::string& str) const {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    /**
     * @brief Get model type name
     */
    std::string getModelTypeName() const {
        switch (modelType) {
            case YOLO_DETECT_FP32: return "YOLO Detection (FP32)";
            case YOLO_DETECT_FP16: return "YOLO Detection (FP16)";
            case YOLO_POSE: return "YOLO Pose (Reserved)";
            case YOLO_CLASSIFY: return "YOLO Classify (Reserved)";
            default: return "Unknown";
        }
    }

    /**
     * @brief Parse model type from string
     */
    MODEL_TYPE parseModelType(const std::string& value) const {
        std::string lower_value = value;
        std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);

        // FP16 model
        if (lower_value == "fp16" || lower_value == "half" || lower_value == "f16") {
            return YOLO_DETECT_FP16;
        }

        // FP32 model (default)
        if (lower_value == "fp32" || lower_value == "float" || lower_value == "f32" ||
            lower_value == "detect" || lower_value == "detection") {
            return YOLO_DETECT_FP32;
        }

        // For compatibility, handle old version names
        if (lower_value.find("half") != std::string::npos ||
            lower_value.find("fp16") != std::string::npos ||
            lower_value.find("f16") != std::string::npos) {
            return YOLO_DETECT_FP16;
        }

        // Default to FP32
        std::cerr << "Warning: Unknown model type '" << value << "', using default FP32" << std::endl;
        return YOLO_DETECT_FP32;
    }

    /**
     * @brief Parse configuration value
     */
    void parseValue(const std::string& key, const std::string& value) {
        if (key == "model_path") modelPath = value;
        else if (key == "classes_file") classesFile = value;
        else if (key == "model_type") modelType = parseModelType(value);
        else if (key == "images_path") imagesPath = value;
        else if (key == "output_path") outputPath = value;
        else if (key == "cropped_path") croppedPath = value;
        else if (key == "confidence_threshold") confidenceThreshold = std::stof(value);
        else if (key == "iou_threshold") iouThreshold = std::stof(value);
        else if (key == "input_width") inputWidth = std::stoi(value);
        else if (key == "input_height") inputHeight = std::stoi(value);
        else if (key == "use_cuda") useCuda = (value == "true" || value == "1" || value == "yes");
        else if (key == "display_results") displayResults = (value == "true" || value == "1" || value == "yes");
        else if (key == "save_cropped_images") saveCroppedImages = (value == "true" || value == "1" || value == "yes");
        else if (key == "max_detections_warning") maxDetectionsWarning = std::stoi(value);
        else if (key == "verbose") verbose = (value == "true" || value == "1" || value == "yes");
        else if (key == "enable_class_filter") enableClassFilter = (value == "true" || value == "1" || value == "yes");
        else if (key == "allowed_classes") parseAllowedClasses(value);
    }

    /**
     * @brief Parse allowed classes list
     * @param value Comma-separated class ID list, e.g., "0,2,5" or "1, 3, 6"
     */
    void parseAllowedClasses(const std::string& value) {
        allowedClasses.clear();
        std::stringstream ss(value);
        std::string item;

        while (std::getline(ss, item, ',')) {
            // Remove whitespace
            item = trim(item);
            if (!item.empty()) {
                try {
                    int classId = std::stoi(item);
                    allowedClasses.push_back(classId);
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Invalid class ID '" << item << "'" << std::endl;
                }
            }
        }
    }
};
