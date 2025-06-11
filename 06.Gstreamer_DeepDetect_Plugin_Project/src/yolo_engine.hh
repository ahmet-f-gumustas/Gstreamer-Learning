#ifndef __YOLO_ENGINE_HH__
#define __YOLO_ENGINE_HH__

#include <NvInfer.h>
#include <cuda_runtime.h>
#include <memory>
#include <vector>
#include <string>

struct DetectionResult;

/**
 * @brief Simple TensorRT logger implementation
 */
class Logger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char* msg) noexcept override {
        if (severity <= Severity::kWARNING) {
            std::cout << msg << std::endl;
        }
    }
};

extern Logger gLogger;

/**
 * @brief Thin wrapper around TensorRT engine for YOLOv8 inference
 */
class YoloEngine {
public:
    /**
     * @brief Constructor
     * @param engine_path Path to TensorRT engine file
     * @param calib_cache_path Optional INT8 calibration cache path
     */
    YoloEngine(const std::string &engine_path, 
               const std::string &calib_cache_path = "");
    
    /**
     * @brief Destructor
     */
    ~YoloEngine();
    
    /**
     * @brief Run inference on GPU memory
     * @param gpu_data Pointer to GPU memory containing input image
     * @param width Image width
     * @param height Image height  
     * @param stream CUDA stream for async execution
     * @return Vector of detection results
     */
    std::vector<DetectionResult> infer(void *gpu_data, int width, int height,
                                       cudaStream_t stream);

private:
    std::unique_ptr<nvinfer1::IRuntime> runtime_;
    std::unique_ptr<nvinfer1::ICudaEngine> engine_;
    std::unique_ptr<nvinfer1::IExecutionContext> context_;
    
    void *input_binding_;
    void *output_binding_;
    size_t input_size_;
    size_t output_size_;
    
    int input_width_;
    int input_height_;
    int num_classes_;
    
    void initialize(const std::string &engine_path);
    void preprocess(void *gpu_input, void *gpu_preprocessed, int width, int height,
                    cudaStream_t stream);
    std::vector<DetectionResult> postprocess(void *gpu_output, cudaStream_t stream);
    std::vector<DetectionResult> apply_nms(const std::vector<DetectionResult>& detections, 
                                          float iou_threshold);
    float calculate_iou(const DetectionResult& a, const DetectionResult& b);
};

// CUDA kernel declaration
__global__ void normalize_kernel(float* data, int width, int height);

#endif /* __YOLO_ENGINE_HH__ */