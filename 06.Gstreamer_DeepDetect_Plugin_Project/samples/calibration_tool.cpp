/**
 * @brief INT8 Calibration tool for YOLOv8 models
 */
#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <memory>
#include <filesystem>

class Logger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char* msg) noexcept override {
        if (severity <= Severity::kWARNING) {
            std::cout << msg << std::endl;
        }
    }
} gLogger;

class Int8Calibrator : public nvinfer1::IInt8Calibrator {
public:
    Int8Calibrator(const std::vector<std::string> &image_paths, 
                   const std::string &cache_file,
                   int batch_size = 1) 
        : image_paths_(image_paths), cache_file_(cache_file), 
          batch_size_(batch_size), current_index_(0) {
        
        input_size_ = 3 * 640 * 640 * sizeof(float); // YOLOv8 input size
        cudaMalloc(&device_input_, input_size_);
    }
    
    ~Int8Calibrator() {
        if (device_input_) {
            cudaFree(device_input_);
        }
    }
    
    int getBatchSize() const noexcept override {
        return batch_size_;
    }
    
    bool getBatch(void* bindings[], const char* names[], int nbBindings) noexcept override {
        if (current_index_ >= image_paths_.size()) {
            return false;
        }
        
        // Load and preprocess image
        cv::Mat image = cv::imread(image_paths_[current_index_]);
        if (image.empty()) {
            std::cerr << "Failed to load image: " << image_paths_[current_index_] << std::endl;
            current_index_++;
            return getBatch(bindings, names, nbBindings);
        }
        
        // Resize to model input size
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(640, 640));
        
        // Convert to float and normalize
        cv::Mat float_image;
        resized.convertTo(float_image, CV_32F, 1.0/255.0);
        
        // Convert HWC to CHW format
        std::vector<cv::Mat> channels(3);
        cv::split(float_image, channels);
        
        std::vector<float> input_data;
        for (int c = 0; c < 3; c++) {
            for (int h = 0; h < 640; h++) {
                for (int w = 0; w < 640; w++) {
                    input_data.push_back(channels[c].at<float>(h, w));
                }
            }
        }
        
        // Copy to device
        cudaMemcpy(device_input_, input_data.data(), input_size_, cudaMemcpyHostToDevice);
        bindings[0] = device_input_;
        
        std::cout << "Calibrating with image " << current_index_ + 1 
                  << "/" << image_paths_.size() << ": " 
                  << image_paths_[current_index_] << std::endl;
        
        current_index_++;
        return true;
    }
    
    const void* readCalibrationCache(size_t& length) noexcept override {
        std::ifstream input(cache_file_, std::ios::binary);
        if (!input.good()) {
            return nullptr;
        }
        
        input.seekg(0, std::ifstream::end);
        length = input.tellg();
        input.seekg(0, std::ifstream::beg);
        
        calibration_cache_.resize(length);
        input.read(calibration_cache_.data(), length);
        return calibration_cache_.data();
    }
    
    void writeCalibrationCache(const void* cache, size_t length) noexcept override {
        std::ofstream output(cache_file_, std::ios::binary);
        output.write(reinterpret_cast<const char*>(cache), length);
        std::cout << "Calibration cache written to: " << cache_file_ << std::endl;
    }

private:
    std::vector<std::string> image_paths_;
    std::string cache_file_;
    int batch_size_;
    size_t current_index_;
    size_t input_size_;
    void* device_input_;
    std::vector<char> calibration_cache_;
};

std::vector<std::string> collect_images(const std::string& directory) {
    std::vector<std::string> image_paths;
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
                image_paths.push_back(entry.path().string());
            }
        }
    }
    
    std::sort(image_paths.begin(), image_paths.end());
    return image_paths;
}

bool build_int8_engine(const std::string& onnx_path, 
                       const std::string& engine_path,
                       const std::vector<std::string>& calibration_images,
                       const std::string& calib_cache) {
    
    auto builder = std::unique_ptr<nvinfer1::IBuilder>(nvinfer1::createInferBuilder(gLogger));
    if (!builder) {
        std::cerr << "Failed to create builder" << std::endl;
        return false;
    }
    
    auto config = std::unique_ptr<nvinfer1::IBuilderConfig>(builder->createBuilderConfig());
    if (!config) {
        std::cerr << "Failed to create builder config" << std::endl;
        return false;
    }
    
    auto network = std::unique_ptr<nvinfer1::INetworkDefinition>(
        builder->createNetworkV2(1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH)));
    if (!network) {
        std::cerr << "Failed to create network" << std::endl;
        return false;
    }
    
    auto parser = std::unique_ptr<nvonnxparser::IParser>(nvonnxparser::createParser(*network, gLogger));
    if (!parser) {
        std::cerr << "Failed to create parser" << std::endl;
        return false;
    }
    
    // Parse ONNX model
    if (!parser->parseFromFile(onnx_path.c_str(), static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
        std::cerr << "Failed to parse ONNX model" << std::endl;
        return false;
    }
    
    // Configure for INT8
    config->setFlag(nvinfer1::BuilderFlag::kINT8);
    config->setMaxWorkspaceSize(1ULL << 30); // 1GB
    
    // Create calibrator
    auto calibrator = std::make_unique<Int8Calibrator>(calibration_images, calib_cache);
    config->setInt8Calibrator(calibrator.get());
    
    // Build engine
    std::cout << "Building INT8 engine. This may take several minutes..." << std::endl;
    auto engine = std::unique_ptr<nvinfer1::ICudaEngine>(builder->buildEngineWithConfig(*network, *config));
    if (!engine) {
        std::cerr << "Failed to build engine" << std::endl;
        return false;
    }
    
    // Serialize engine
    auto serialized = std::unique_ptr<nvinfer1::IHostMemory>(engine->serialize());
    if (!serialized) {
        std::cerr << "Failed to serialize engine" << std::endl;
        return false;
    }
    
    // Save to file
    std::ofstream outfile(engine_path, std::ios::binary);
    if (!outfile) {
        std::cerr << "Failed to open output file: " << engine_path << std::endl;
        return false;
    }
    
    outfile.write(static_cast<const char*>(serialized->data()), serialized->size());
    outfile.close();
    
    std::cout << "INT8 engine saved to: " << engine_path << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <onnx_model> <images_dir> <output_engine> [calib_cache]" << std::endl;
        std::cout << "Example: " << argv[0] << " yolov8n.onnx /path/to/coco/images yolov8n_int8.trt calibration.cache" << std::endl;
        return 1;
    }
    
    std::string onnx_path = argv[1];
    std::string images_dir = argv[2];
    std::string output_engine = argv[3];
    std::string calib_cache = argc > 4 ? argv[4] : "calibration.cache";
    
    // Validate inputs
    if (!std::filesystem::exists(onnx_path)) {
        std::cerr << "ONNX model not found: " << onnx_path << std::endl;
        return 1;
    }
    
    if (!std::filesystem::exists(images_dir)) {
        std::cerr << "Images directory not found: " << images_dir << std::endl;
        return 1;
    }
    
    // Collect calibration images
    std::cout << "Collecting calibration images from: " << images_dir << std::endl;
    std::vector<std::string> image_paths = collect_images(images_dir);
    
    if (image_paths.empty()) {
        std::cerr << "No images found in directory: " << images_dir << std::endl;
        return 1;
    }
    
    // Limit to 500 images for calibration (recommended)
    if (image_paths.size() > 500) {
        image_paths.resize(500);
    }
    
    std::cout << "Found " << image_paths.size() << " calibration images" << std::endl;
    
    // Build INT8 engine
    if (!build_int8_engine(onnx_path, output_engine, image_paths, calib_cache)) {
        std::cerr << "Failed to build INT8 engine" << std::endl;
        return 1;
    }
    
    std::cout << "INT8 calibration completed successfully!" << std::endl;
    std::cout << "Engine saved to: " << output_engine << std::endl;
    std::cout << "Calibration cache saved to: " << calib_cache << std::endl;
    
    return 0;