#ifndef JETSON_EDGE_PIPELINE_H
#define JETSON_EDGE_PIPELINE_H

#include "camera/i_camera.h"
#include "inference/tensorrt_engine.h"
#include "tracking/byte_tracker.h"
#include "monitoring/tegrastats_parser.h"
#include "monitoring/orin_simulator.h"
#include "monitoring/perf_logger.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <atomic>
#include <string>
#include <memory>
#include <functional>

namespace edge {

struct PipelineConfig {
    std::string camera_type      = "auto";   // usb | csi | gmsl | gige | auto
    std::string camera_node      = "";
    CameraCaps  caps;

    std::string engine_path      = "models/yolov8n_int8.engine";
    std::string onnx_path        = "models/yolov8n.onnx";
    std::string calib_dir        = "models/coco_calib_subset";
    std::string calib_cache      = "models/yolov8n_int8.cache";
    Precision   precision        = Precision::FP16;

    ByteTrackConfig tracker;

    bool        enable_display   = true;     // OpenCV window
    std::string output_video     = "";       // empty = no record
    std::string perf_csv         = "perf.csv";
    std::string perf_json        = "perf_summary.json";

    PowerMode   orin_mode        = PowerMode::P_15W;
    bool        simulate_jetson  = true;     // produce synthetic tegra samples
};

struct DetectionCallback {
    // Optional sink for downstream consumers (e.g. MQTT bridge).
    std::function<void(int frame_id, const std::vector<Detection>&)> on_detections;
};

class EdgePipeline {
public:
    EdgePipeline();
    ~EdgePipeline();

    bool initialize(const PipelineConfig& cfg);
    void run();        // blocks until stop() or EOS / Ctrl-C
    void stop();

    void setCallback(const DetectionCallback& cb) { cb_ = cb; }

private:
    bool buildGstPipeline();
    void onFrame(GstSample* sample);

    static GstFlowReturn onNewSample(GstAppSink* sink, gpointer user);

    PipelineConfig cfg_;
    CameraPtr      camera_;
    std::unique_ptr<TensorRTEngine>   engine_;
    std::unique_ptr<ByteTracker>      tracker_;
    std::unique_ptr<TegrastatsParser> tegra_;
    std::unique_ptr<OrinSimulator>    orin_sim_;
    std::unique_ptr<PerfLogger>       perf_;

    GstElement*  gst_pipeline_ = nullptr;
    GstElement*  gst_sink_     = nullptr;

    std::atomic<bool> stop_{false};
    int  frame_id_  = 0;
    int  width_     = 0;
    int  height_    = 0;
    DetectionCallback cb_;

    // For FPS over a sliding window
    std::chrono::steady_clock::time_point last_fps_t_;
    int   fps_window_frames_ = 0;
    float current_fps_       = 0.f;
};

}  // namespace edge

#endif
