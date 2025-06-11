#include "yolo_engine.hh"
#include "gstdeepdetect.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <nvbuf_utils.h>

// Global logger instance
Logger gLogger;

YoloEngine::YoloEngine(const std::string &engine_path, 
                       const std::string &calib_cache_path) 
    : input_binding_(nullptr), output_binding_(nullptr) {
    initialize(engine_path);
}

YoloEngine::~YoloEngine() {
    if (input_binding_) cudaFree(input_binding_);
    if (output_binding_) cudaFree(output_binding_);
}

void YoloEngine::initialize(const std::string &engine_path) {
    // Load engine from file
    std::ifstream file(engine_path, std::ios::binary);
    if (!file.good()) {
        throw std::runtime_error("Failed to open engine file: " + engine_path);
    }
    
    file.seekg(0, std::ifstream::end);
    size_t size = file.tellg();
    file.seekg(0, std::ifstream::beg);
    
    std::vector<char> engine_data(size);
    file.read(engine_data.data(), size);
    file.close();
    
    // Create TensorRT runtime and engine
    runtime_.reset(nvinfer1::createInferRuntime(gLogger));
    if (!runtime_) {
        throw std::runtime_error("Failed to create TensorRT runtime");
    }
    
    engine_.reset(runtime_->deserializeCudaEngine(engine_data.data(), size));
    if (!engine_) {
        throw std::runtime_error("Failed to deserialize CUDA engine");
    }
    
    context_.reset(engine_->createExecutionContext());
    if (!context_) {
        throw std::runtime_error("Failed to create execution context");
    }
    
    // Get input/output dimensions
    auto input_dims = engine_->getBindingDimensions(0);
    auto output_dims = engine_->getBindingDimensions(1);
    
    input_width_ = input_dims.d[3];
    input_height_ = input_dims.d[2];
    num_classes_ = 80; // COCO classes
    
    input_size_ = input_dims.d[1] * input_dims.d[2] * input_dims.d[3] * sizeof(float);
    output_size_ = output_dims.d[1] * output_dims.d[2] * sizeof(float);
    
    // Allocate GPU memory for bindings
    cudaMalloc(&input_binding_, input_size_);
    cudaMalloc(&output_binding_, output_size_);
}

std::vector<DetectionResult> YoloEngine::infer(void *gpu_data, int width, int height,
                                               cudaStream_t stream) {
    // Preprocess: resize and normalize input
    preprocess(gpu_data, input_binding_, width, height, stream);
    
    // Run inference
    void *bindings[] = {input_binding_, output_binding_};
    if (!context_->enqueueV2(bindings, stream, nullptr)) {
        throw std::runtime_error("TensorRT inference failed");
    }
    
    // Postprocess: extract detections
    return postprocess(output_binding_, stream);
}

void YoloEngine::preprocess(void *gpu_input, void *gpu_preprocessed, 
                           int width, int height, cudaStream_t stream) {
    // Use NVIDIA Video Processing library for efficient resize/color conversion
    NvBufSurfaceParams src_params = {};
    src_params.width = width;
    src_params.height = height;
    src_params.layout = NVBUF_LAYOUT_PITCH;
    src_params.colorFormat = NVBUF_COLOR_FORMAT_NV12; // Assume NV12 input
    src_params.dataPtr = gpu_input;
    
    NvBufSurfaceParams dst_params = {};
    dst_params.width = input_width_;
    dst_params.height = input_height_;
    dst_params.layout = NVBUF_LAYOUT_PITCH;
    dst_params.colorFormat = NVBUF_COLOR_FORMAT_RGB;
    dst_params.dataPtr = gpu_preprocessed;
    
    // Perform resize and color space conversion
    NvBufSurfTransformParams transform_params = {};
    transform_params.transform_flag = NVBUFSURF_TRANSFORM_FILTER;
    transform_params.transform_filter = NvBufSurfTransformInter_Default;
    
    NvBufSurfTransform(&src_params, &dst_params, &transform_params);
    
    // Normalize pixel values to [0, 1] range (YOLOv8 expects normalized input)
    // This would typically be done with a custom CUDA kernel
    normalize_kernel<<<dim3((input_width_ + 31) / 32, (input_height_ + 31) / 32), 
                       dim3(32, 32), 0, stream>>>(
        (float*)gpu_preprocessed, input_width_, input_height_);
}

__global__ void normalize_kernel(float* data, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (x < width && y < height) {
        int idx = y * width + x;
        // Normalize from [0, 255] to [0, 1]
        data[idx] /= 255.0f;
    }
}

std::vector<DetectionResult> YoloEngine::postprocess(void *gpu_output, cudaStream_t stream) {
    std::vector<DetectionResult> detections;
    
    // Copy output to host for post-processing
    // In a production implementation, this could be optimized to stay on GPU
    size_t output_elements = output_size_ / sizeof(float);
    std::vector<float> host_output(output_elements);
    
    cudaMemcpyAsync(host_output.data(), gpu_output, output_size_,
                    cudaMemcpyDeviceToHost, stream);
    cudaStreamSynchronize(stream);
    
    // YOLOv8 output format: [batch_size, 84, 8400]
    // 84 = 4 (bbox) + 80 (classes)
    int num_detections = 8400;
    int num_classes = 80;
    
    for (int i = 0; i < num_detections; i++) {
        float max_conf = 0.0f;
        int max_class = -1;
        
        // Find class with highest confidence
        for (int j = 0; j < num_classes; j++) {
            float conf = host_output[i * 84 + 4 + j];
            if (conf > max_conf) {
                max_conf = conf;
                max_class = j;
            }
        }
        
        if (max_conf > 0.1f) { // Pre-filter low confidence detections
            DetectionResult det;
            det.x = host_output[i * 84 + 0];
            det.y = host_output[i * 84 + 1]; 
            det.w = host_output[i * 84 + 2];
            det.h = host_output[i * 84 + 3];
            det.confidence = max_conf;
            det.class_id = max_class;
            
            detections.push_back(det);
        }
    }
    
    // Apply Non-Maximum Suppression
    return apply_nms(detections, 0.45f); // IoU threshold
}

std::vector<DetectionResult> YoloEngine::apply_nms(
    const std::vector<DetectionResult>& detections, float iou_threshold) {
    
    std::vector<DetectionResult> result;
    std::vector<bool> suppressed(detections.size(), false);
    
    // Sort by confidence
    std::vector<size_t> indices(detections.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), 
              [&](size_t a, size_t b) {
                  return detections[a].confidence > detections[b].confidence;
              });
    
    for (size_t i = 0; i < indices.size(); i++) {
        if (suppressed[indices[i]]) continue;
        
        result.push_back(detections[indices[i]]);
        
        for (size_t j = i + 1; j < indices.size(); j++) {
            if (suppressed[indices[j]]) continue;
            
            float iou = calculate_iou(detections[indices[i]], detections[indices[j]]);
            if (iou > iou_threshold) {
                suppressed[indices[j]] = true;
            }
        }
    }
    
    return result;
}

float YoloEngine::calculate_iou(const DetectionResult& a, const DetectionResult& b) {
    float x1 = std::max(a.x, b.x);
    float y1 = std::max(a.y, b.y);
    float x2 = std::min(a.x + a.w, b.x + b.w);
    float y2 = std::min(a.y + a.h, b.y + b.h);
    
    if (x2 <= x1 || y2 <= y1) return 0.0f;
    
    float intersection = (x2 - x1) * (y2 - y1);
    float union_area = a.w * a.h + b.w * b.h - intersection;
    
}