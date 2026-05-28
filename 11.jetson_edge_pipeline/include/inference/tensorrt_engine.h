#ifndef JETSON_EDGE_TENSORRT_ENGINE_H
#define JETSON_EDGE_TENSORRT_ENGINE_H

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

// Forward declarations to keep TRT/CUDA headers out of public API.
namespace nvinfer1 {
    class IRuntime;
    class ICudaEngine;
    class IExecutionContext;
    class ILogger;
}

namespace edge {

struct Detection {
    float   x, y, w, h;        // pixel coords on input frame
    int     class_id;
    float   confidence;
    int     track_id = -1;     // assigned by tracker stage
};

enum class Precision { FP32, FP16, INT8 };

struct EngineConfig {
    std::string onnx_path;             // for first-time build
    std::string engine_path;           // serialized .engine cache
    std::string calib_cache_path;      // INT8 cache
    std::string calib_images_dir;      // INT8 calibration images
    Precision   precision     = Precision::FP16;
    int         input_width   = 640;
    int         input_height  = 640;
    int         max_batch     = 1;
    float       conf_thresh   = 0.25f;
    float       nms_iou       = 0.45f;
    int         num_classes   = 80;
    bool        use_dla       = false; // Jetson DLA core 0/1
    int         dla_core      = 0;
};

class TensorRTEngine {
public:
    TensorRTEngine();
    ~TensorRTEngine();

    bool build(const EngineConfig& cfg);    // ONNX -> serialized engine
    bool load(const std::string& engine_path);

    // Async inference. `bgr` must point to HWC uint8 bgr image of size
    // input_width * input_height * 3.  Set stream=0 for default.
    std::vector<Detection> infer(const uint8_t* bgr,
                                 int orig_w, int orig_h,
                                 void* cuda_stream = nullptr);

    int  inputWidth()  const { return cfg_.input_width;  }
    int  inputHeight() const { return cfg_.input_height; }
    const EngineConfig& config() const { return cfg_; }

    // Last inference timing (ms), for perf logging.
    float lastPreprocessMs() const { return pre_ms_;  }
    float lastInferenceMs()  const { return inf_ms_;  }
    float lastPostprocessMs()const { return post_ms_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    EngineConfig cfg_;
    float pre_ms_  = 0.f;
    float inf_ms_  = 0.f;
    float post_ms_ = 0.f;
};

}  // namespace edge

#endif
