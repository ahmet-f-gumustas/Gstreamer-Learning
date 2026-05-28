#include "edge_pipeline.h"
#include "camera/camera_factory.h"

#include <gst/video/video.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include <iostream>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace edge {

static cv::VideoWriter g_writer;
static cv::Mat         g_last_display;

EdgePipeline::EdgePipeline() = default;
EdgePipeline::~EdgePipeline() { stop(); }

// ─────────────────────────────────────────────────────────────────────────────
bool EdgePipeline::initialize(const PipelineConfig& cfg) {
    cfg_ = cfg;

    // 1) Camera
    camera_ = CameraFactory::create(cfg_.camera_type, cfg_.camera_node, cfg_.caps);
    if (!camera_) {
        std::cerr << "[pipeline] Failed to construct camera (" << cfg_.camera_type << ")\n";
        return false;
    }
    auto info = camera_->getInfo();
    std::cout << "[pipeline] Camera: " << CameraFactory::typeName(info.type)
              << " @ " << info.node << " (" << info.model << ")\n";

    // 2) TensorRT engine — try to load, build if missing
    engine_ = std::make_unique<TensorRTEngine>();
    EngineConfig ec;
    ec.engine_path      = cfg_.engine_path;
    ec.onnx_path        = cfg_.onnx_path;
    ec.calib_images_dir = cfg_.calib_dir;
    ec.calib_cache_path = cfg_.calib_cache;
    ec.precision        = cfg_.precision;
    ec.input_width      = 640;
    ec.input_height     = 640;
    if (fs::exists(cfg_.engine_path)) {
        if (!engine_->load(cfg_.engine_path)) return false;
        // load() does not populate cfg_, so set width/height from EngineConfig
        ec.input_width  = engine_->inputWidth();
        ec.input_height = engine_->inputHeight();
    } else if (fs::exists(cfg_.onnx_path)) {
        std::cout << "[pipeline] No engine at " << cfg_.engine_path
                  << " — building from ONNX (this can take a few minutes)…\n";
        if (!engine_->build(ec)) return false;
    } else {
        std::cerr << "[pipeline] Neither engine nor ONNX present — "
                     "run scripts/build_int8_engine.py first\n";
        return false;
    }

    // 3) Tracker
    tracker_ = std::make_unique<ByteTracker>(cfg_.tracker);

    // 4) Monitoring
    tegra_ = std::make_unique<TegrastatsParser>();
    tegra_->start(1000);    // 1 Hz; on dev PC this fails harmlessly

    orin_sim_ = std::make_unique<OrinSimulator>();
    orin_sim_->setPowerMode(cfg_.orin_mode);
    std::cout << "[pipeline] Host GPU: " << orin_sim_->hostDevice().name
              << "  (" << orin_sim_->hostDevice().int8_tops << " INT8 TOPS)\n";

    perf_ = std::make_unique<PerfLogger>();
    perf_->open(cfg_.perf_csv);

    // 5) GStreamer
    return buildGstPipeline();
}

// ─────────────────────────────────────────────────────────────────────────────
bool EdgePipeline::buildGstPipeline() {
    width_  = cfg_.caps.width;
    height_ = cfg_.caps.height;

    std::string src = camera_->buildPipelineString();
    std::string full =
        src + " ! appsink name=sink emit-signals=true "
              "max-buffers=2 drop=true sync=false "
              "caps=video/x-raw,format=BGR";

    std::cout << "[pipeline] GStreamer pipeline:\n  " << full << "\n";

    GError* err = nullptr;
    gst_pipeline_ = gst_parse_launch(full.c_str(), &err);
    if (!gst_pipeline_) {
        std::cerr << "[pipeline] gst_parse_launch failed: "
                  << (err ? err->message : "unknown") << "\n";
        if (err) g_error_free(err);
        return false;
    }

    gst_sink_ = gst_bin_get_by_name(GST_BIN(gst_pipeline_), "sink");
    g_signal_connect(gst_sink_, "new-sample", G_CALLBACK(onNewSample), this);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
GstFlowReturn EdgePipeline::onNewSample(GstAppSink* sink, gpointer user) {
    auto* self = static_cast<EdgePipeline*>(user);
    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample) return GST_FLOW_ERROR;
    self->onFrame(sample);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
void EdgePipeline::onFrame(GstSample* sample) {
    using clk = std::chrono::high_resolution_clock;

    GstBuffer* buf  = gst_sample_get_buffer(sample);
    GstCaps*   caps = gst_sample_get_caps(sample);
    if (!buf || !caps) return;

    GstVideoInfo vinfo;
    gst_video_info_from_caps(&vinfo, caps);
    int w = GST_VIDEO_INFO_WIDTH(&vinfo);
    int h = GST_VIDEO_INFO_HEIGHT(&vinfo);

    GstMapInfo m;
    if (!gst_buffer_map(buf, &m, GST_MAP_READ)) return;

    cv::Mat frame(h, w, CV_8UC3, m.data, GST_VIDEO_INFO_PLANE_STRIDE(&vinfo, 0));
    cv::Mat frame_copy = frame.clone();  // detach from gst memory
    gst_buffer_unmap(buf, &m);

    auto t0 = clk::now();
    auto dets = engine_->infer(frame_copy.data, w, h);
    auto t1 = clk::now();
    auto tracks = tracker_->update(dets);
    auto t2 = clk::now();

    float track_ms = std::chrono::duration<float, std::milli>(t2 - t1).count();
    (void)t0;  // pre/inf/post already inside engine

    // ── FPS over 30-frame sliding window ─────────────────────────────────────
    ++fps_window_frames_;
    if (fps_window_frames_ >= 30) {
        auto now = clk::now();
        float dt = std::chrono::duration<float>(now - last_fps_t_).count();
        current_fps_ = dt > 0 ? fps_window_frames_ / dt : 0;
        last_fps_t_ = now;
        fps_window_frames_ = 0;
    }

    // ── Build perf frame and log ─────────────────────────────────────────────
    PerfFrame pf;
    pf.frame_id      = ++frame_id_;
    pf.fps           = current_fps_;
    pf.inference_ms  = engine_->lastInferenceMs();
    pf.preproc_ms    = engine_->lastPreprocessMs();
    pf.postproc_ms   = engine_->lastPostprocessMs();
    pf.tracking_ms   = track_ms;
    pf.detections    = static_cast<int>(dets.size());
    pf.active_tracks = tracker_->activeTracks();

    auto real = tegra_->lastSample();
    if (real.valid) {
        pf.tegra = real;
    } else if (cfg_.simulate_jetson && orin_sim_) {
        float util = std::clamp(pf.inference_ms / 30.f, 0.f, 1.f);
        orin_sim_->synthesizeSample(current_fps_, util, pf.tegra);
    }
    perf_->log(pf);

    if (cb_.on_detections) cb_.on_detections(pf.frame_id, tracks);

    // ── Visualization ────────────────────────────────────────────────────────
    if (cfg_.enable_display || !cfg_.output_video.empty()) {
        cv::Mat disp = frame_copy.clone();
        for (auto& d : tracks) {
            cv::Scalar color(
                (d.track_id * 67)  % 255,
                (d.track_id * 113) % 255,
                (d.track_id * 199) % 255);
            cv::rectangle(disp, {(int)d.x, (int)d.y},
                          {(int)(d.x + d.w), (int)(d.y + d.h)}, color, 2);
            std::string label = "id=" + std::to_string(d.track_id) +
                                " cls=" + std::to_string(d.class_id);
            cv::putText(disp, label, {(int)d.x, (int)d.y - 6},
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
        std::ostringstream hud;
        hud << "FPS:" << std::fixed << std::setprecision(1) << current_fps_
            << "  inf:" << pf.inference_ms << "ms"
            << "  trk:" << pf.tracking_ms  << "ms"
            << "  det:" << pf.detections
            << "  pw:"  << std::setprecision(1) << pf.tegra.power_total_mw / 1000.f << "W";
        cv::putText(disp, hud.str(), {10, 24}, cv::FONT_HERSHEY_SIMPLEX,
                    0.55, {0, 255, 255}, 1);

        g_last_display = disp;
        if (cfg_.enable_display) {
            cv::imshow("Jetson Edge Pipeline", disp);
            if (cv::waitKey(1) == 'q') stop_ = true;
        }
        if (!cfg_.output_video.empty()) {
            if (!g_writer.isOpened())
                g_writer.open(cfg_.output_video,
                              cv::VideoWriter::fourcc('M','J','P','G'),
                              30, disp.size());
            g_writer.write(disp);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void EdgePipeline::run() {
    last_fps_t_ = std::chrono::steady_clock::now();
    gst_element_set_state(gst_pipeline_, GST_STATE_PLAYING);

    GstBus* bus = gst_element_get_bus(gst_pipeline_);
    while (!stop_) {
        GstMessage* msg = gst_bus_timed_pop_filtered(
            bus, 100 * GST_MSECOND,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (!msg) continue;
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError* err = nullptr; gchar* dbg = nullptr;
            gst_message_parse_error(msg, &err, &dbg);
            std::cerr << "[pipeline] GST error: " << (err ? err->message : "?") << "\n";
            if (dbg) g_free(dbg);
            if (err) g_error_free(err);
            stop_ = true;
        } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            std::cout << "[pipeline] EOS\n";
            stop_ = true;
        }
        gst_message_unref(msg);
    }
    gst_object_unref(bus);
    gst_element_set_state(gst_pipeline_, GST_STATE_NULL);
}

void EdgePipeline::stop() {
    if (stop_) return;
    stop_ = true;
    if (gst_pipeline_) gst_element_set_state(gst_pipeline_, GST_STATE_NULL);
    if (tegra_) tegra_->stop();
    if (perf_)  perf_->writeSummary(cfg_.perf_json);
    if (g_writer.isOpened()) g_writer.release();
    if (cfg_.enable_display) cv::destroyAllWindows();
    if (gst_sink_)     { gst_object_unref(gst_sink_); gst_sink_ = nullptr; }
    if (gst_pipeline_) { gst_object_unref(gst_pipeline_); gst_pipeline_ = nullptr; }
}

}  // namespace edge
