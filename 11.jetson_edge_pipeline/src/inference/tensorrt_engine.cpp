#include "inference/tensorrt_engine.h"
#include "inference/int8_calibrator.h"

#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime_api.h>

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

#include <fstream>
#include <iostream>
#include <chrono>
#include <cstring>
#include <algorithm>

namespace edge {

// ─────────────────────────────────────────────────────────────────────────────
// Logger — silences everything below WARNING by default
class TrtLogger : public nvinfer1::ILogger {
public:
    void log(Severity sev, const char* msg) noexcept override {
        if (sev <= Severity::kWARNING)
            std::cerr << "[TRT] " << msg << "\n";
    }
};
static TrtLogger g_logger;

// ─────────────────────────────────────────────────────────────────────────────
struct TensorRTEngine::Impl {
    nvinfer1::IRuntime*          runtime  = nullptr;
    nvinfer1::ICudaEngine*       engine   = nullptr;
    nvinfer1::IExecutionContext* context  = nullptr;
    cudaStream_t                 stream   = nullptr;

    void*  d_input  = nullptr;
    void*  d_output = nullptr;
    size_t input_size_bytes  = 0;
    size_t output_size_bytes = 0;
    int    num_outputs = 0;
    int    rows = 0, cols = 0;   // YOLOv8: rows = 8400, cols = 84 (4 + num_classes)
    std::vector<float> h_output;

    ~Impl() {
        if (stream)   cudaStreamDestroy(stream);
        if (d_input)  cudaFree(d_input);
        if (d_output) cudaFree(d_output);
        if (context)  delete context;
        if (engine)   delete engine;
        if (runtime)  delete runtime;
    }
};

TensorRTEngine::TensorRTEngine() : impl_(std::make_unique<Impl>()) {}
TensorRTEngine::~TensorRTEngine() = default;

// ─────────────────────────────────────────────────────────────────────────────
bool TensorRTEngine::build(const EngineConfig& cfg) {
    cfg_ = cfg;

    auto* builder = nvinfer1::createInferBuilder(g_logger);
    if (!builder) return false;

    const auto explicit_batch =
        1U << static_cast<uint32_t>(
            nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    auto* network = builder->createNetworkV2(explicit_batch);
    auto* parser  = nvonnxparser::createParser(*network, g_logger);

    if (!parser->parseFromFile(cfg.onnx_path.c_str(),
                               static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
        std::cerr << "[TRT] Failed to parse ONNX: " << cfg.onnx_path << "\n";
        delete parser; delete network; delete builder;
        return false;
    }

    auto* config = builder->createBuilderConfig();
    config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE,
                               1ULL << 30);   // 1 GiB

    std::unique_ptr<Int8EntropyCalibrator2> calib;
    if (cfg.precision == Precision::FP16) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
    } else if (cfg.precision == Precision::INT8) {
        config->setFlag(nvinfer1::BuilderFlag::kINT8);
        calib = std::make_unique<Int8EntropyCalibrator2>(
            1, cfg.input_width, cfg.input_height,
            cfg.calib_images_dir, cfg.calib_cache_path);
        config->setInt8Calibrator(calib.get());
    }

    if (cfg.use_dla) {
        config->setDefaultDeviceType(nvinfer1::DeviceType::kDLA);
        config->setDLACore(cfg.dla_core);
        config->setFlag(nvinfer1::BuilderFlag::kGPU_FALLBACK);
    }

    auto* plan = builder->buildSerializedNetwork(*network, *config);
    if (!plan) {
        std::cerr << "[TRT] Engine build failed\n";
        delete config; delete parser; delete network; delete builder;
        return false;
    }

    std::ofstream f(cfg.engine_path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(plan->data()), plan->size());
    f.close();
    std::cout << "[TRT] Serialized engine -> " << cfg.engine_path
              << "  (" << plan->size() / (1024*1024) << " MiB)\n";

    delete plan; delete config; delete parser; delete network; delete builder;
    return load(cfg.engine_path);
}

// ─────────────────────────────────────────────────────────────────────────────
bool TensorRTEngine::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        std::cerr << "[TRT] Cannot open engine: " << path << "\n";
        return false;
    }
    size_t sz = f.tellg();
    f.seekg(0);
    std::vector<char> blob(sz);
    f.read(blob.data(), sz);

    impl_->runtime = nvinfer1::createInferRuntime(g_logger);
    impl_->engine  = impl_->runtime->deserializeCudaEngine(blob.data(), sz);
    if (!impl_->engine) {
        std::cerr << "[TRT] deserializeCudaEngine failed\n";
        return false;
    }
    impl_->context = impl_->engine->createExecutionContext();
    cudaStreamCreate(&impl_->stream);

    // Discover input/output sizes for binding[0] and binding[1] (YOLOv8 has
    // one input "images" 1x3xHxW, one output "output0" 1x84x8400 or 1x8400x84).
    impl_->input_size_bytes = 3 * cfg_.input_width * cfg_.input_height * sizeof(float);
    cudaMalloc(&impl_->d_input, impl_->input_size_bytes);

    // Output: query from engine
    auto dims = impl_->engine->getTensorShape(impl_->engine->getIOTensorName(1));
    size_t out_count = 1;
    for (int i = 0; i < dims.nbDims; ++i)
        out_count *= std::max<int64_t>(1, dims.d[i]);

    // YOLOv8 output ordering: (1, 84, 8400) — channels first.
    if (dims.nbDims == 3) {
        impl_->cols = dims.d[1];           // 84
        impl_->rows = dims.d[2];           // 8400
    }
    impl_->output_size_bytes = out_count * sizeof(float);
    cudaMalloc(&impl_->d_output, impl_->output_size_bytes);
    impl_->h_output.resize(out_count);

    impl_->context->setTensorAddress(impl_->engine->getIOTensorName(0), impl_->d_input);
    impl_->context->setTensorAddress(impl_->engine->getIOTensorName(1), impl_->d_output);

    std::cout << "[TRT] Engine loaded. Input=" << cfg_.input_width << "x"
              << cfg_.input_height << "  Output rows=" << impl_->rows
              << " cols=" << impl_->cols << "\n";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// YOLOv8 NMS on CPU (simple — for GPU NMS see DeepStream or efficientNMSPlugin)
static float iou(const Detection& a, const Detection& b) {
    float x1 = std::max(a.x, b.x);
    float y1 = std::max(a.y, b.y);
    float x2 = std::min(a.x + a.w, b.x + b.w);
    float y2 = std::min(a.y + a.h, b.y + b.h);
    float w = std::max(0.f, x2 - x1);
    float h = std::max(0.f, y2 - y1);
    float inter = w * h;
    float uni   = a.w * a.h + b.w * b.h - inter;
    return uni > 0 ? inter / uni : 0.f;
}

static std::vector<Detection> nms(std::vector<Detection> dets, float iou_thr) {
    std::sort(dets.begin(), dets.end(),
              [](const Detection& a, const Detection& b){ return a.confidence > b.confidence; });
    std::vector<Detection> keep;
    std::vector<bool> dead(dets.size(), false);
    for (size_t i = 0; i < dets.size(); ++i) {
        if (dead[i]) continue;
        keep.push_back(dets[i]);
        for (size_t j = i + 1; j < dets.size(); ++j) {
            if (dead[j]) continue;
            if (dets[i].class_id == dets[j].class_id &&
                iou(dets[i], dets[j]) > iou_thr)
                dead[j] = true;
        }
    }
    return keep;
}

std::vector<Detection> TensorRTEngine::infer(const uint8_t* bgr,
                                             int orig_w, int orig_h,
                                             void* user_stream) {
    using clk = std::chrono::high_resolution_clock;

    cudaStream_t stream = user_stream
        ? static_cast<cudaStream_t>(user_stream) : impl_->stream;

    // ── 1) Preprocess: letterbox -> CHW float32 [0,1] ───────────────────────
    auto t0 = clk::now();
    cv::Mat src(orig_h, orig_w, CV_8UC3, const_cast<uint8_t*>(bgr));

    int Wi = cfg_.input_width, Hi = cfg_.input_height;
    float r = std::min(static_cast<float>(Wi) / orig_w,
                       static_cast<float>(Hi) / orig_h);
    int nw = static_cast<int>(orig_w * r);
    int nh = static_cast<int>(orig_h * r);
    int dx = (Wi - nw) / 2;
    int dy = (Hi - nh) / 2;

    cv::Mat resized;
    cv::resize(src, resized, {nw, nh});
    cv::Mat letter(Hi, Wi, CV_8UC3, cv::Scalar(114, 114, 114));
    resized.copyTo(letter(cv::Rect(dx, dy, nw, nh)));

    cv::Mat rgb;
    cv::cvtColor(letter, rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(rgb, CV_32FC3, 1.0 / 255.0);

    // HWC -> CHW
    std::vector<cv::Mat> chans(3);
    cv::split(rgb, chans);
    std::vector<float> chw(3 * Wi * Hi);
    std::memcpy(chw.data() + 0 * Wi * Hi, chans[0].data, Wi * Hi * sizeof(float));
    std::memcpy(chw.data() + 1 * Wi * Hi, chans[1].data, Wi * Hi * sizeof(float));
    std::memcpy(chw.data() + 2 * Wi * Hi, chans[2].data, Wi * Hi * sizeof(float));

    cudaMemcpyAsync(impl_->d_input, chw.data(), impl_->input_size_bytes,
                    cudaMemcpyHostToDevice, stream);
    auto t1 = clk::now();

    // ── 2) Inference ─────────────────────────────────────────────────────────
    impl_->context->enqueueV3(stream);
    cudaMemcpyAsync(impl_->h_output.data(), impl_->d_output,
                    impl_->output_size_bytes, cudaMemcpyDeviceToHost, stream);
    cudaStreamSynchronize(stream);
    auto t2 = clk::now();

    // ── 3) Postprocess (YOLOv8 head, channel-first) ──────────────────────────
    std::vector<Detection> raw;
    raw.reserve(64);

    int R = impl_->rows;        // 8400
    int C = impl_->cols;        // 4 + num_classes (e.g. 84)
    int num_cls = C - 4;

    auto at = [&](int row, int col) {
        // channel-first: out[col * R + row]
        return impl_->h_output[col * R + row];
    };

    for (int i = 0; i < R; ++i) {
        float best_conf = 0.f;
        int   best_cls  = -1;
        for (int c = 0; c < num_cls; ++c) {
            float s = at(i, 4 + c);
            if (s > best_conf) { best_conf = s; best_cls = c; }
        }
        if (best_conf < cfg_.conf_thresh) continue;

        float cx = at(i, 0);
        float cy = at(i, 1);
        float w  = at(i, 2);
        float h  = at(i, 3);

        // remove letterbox padding, rescale to original frame
        float x = (cx - w / 2.f - dx) / r;
        float y = (cy - h / 2.f - dy) / r;
        float bw = w / r;
        float bh = h / r;

        Detection d;
        d.x = std::max(0.f, x);
        d.y = std::max(0.f, y);
        d.w = bw;
        d.h = bh;
        d.class_id   = best_cls;
        d.confidence = best_conf;
        raw.push_back(d);
    }

    auto dets = nms(std::move(raw), cfg_.nms_iou);
    auto t3 = clk::now();

    pre_ms_  = std::chrono::duration<float, std::milli>(t1 - t0).count();
    inf_ms_  = std::chrono::duration<float, std::milli>(t2 - t1).count();
    post_ms_ = std::chrono::duration<float, std::milli>(t3 - t2).count();
    return dets;
}

}  // namespace edge
