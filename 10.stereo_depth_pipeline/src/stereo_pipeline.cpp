#include "stereo_pipeline.h"
#include <iostream>
#include <chrono>
#include <thread>

// ─────────────────────────────────────────────────────────────────────────────
StereoPipeline::StereoPipeline() = default;

StereoPipeline::~StereoPipeline() {
    stop();
}

// ─── initialize ──────────────────────────────────────────────────────────────
bool StereoPipeline::initialize(SourceMode mode,
                                const std::string& leftSource,
                                const std::string& rightSource,
                                int width, int height)
{
    width_  = width;
    height_ = height;

    bool ok = false;
    switch (mode) {
        case SourceMode::SIMULATION:
            ok = setupSimulation();
            break;
        case SourceMode::DUAL_WEBCAM:
            ok = setupDualWebcam(leftSource, rightSource);
            break;
        case SourceMode::VIDEO_FILE:
            ok = setupVideoFile(leftSource);
            break;
    }

    if (!ok || !pipeline_) return false;

    bindAppsinks();
    bus_ = gst_element_get_bus(pipeline_);
    return true;
}

// ─── Simulation: tek kaynak → tee → iki appsink ──────────────────────────────
bool StereoPipeline::setupSimulation()
{
    // Ball pattern (18) canlı kaynak; tee ile iki kola ayırıyoruz.
    // Sağ kol tamamen aynı görüntüyü alır; "stereo farkı" DepthEstimator
    // tarafından yapay kayma (shift) ile oluşturulur.
    std::string desc =
        "videotestsrc pattern=18 is-live=true "
        "! video/x-raw,width=" + std::to_string(width_) +
        ",height=" + std::to_string(height_) +
        ",framerate=30/1 "
        "! tee name=t "
        "  t. ! queue max-size-buffers=2 leaky=downstream "
        "     ! videoconvert "
        "     ! video/x-raw,format=BGR "
        "     ! appsink name=left_sink  emit-signals=true sync=true  max-buffers=1 drop=true "
        "  t. ! queue max-size-buffers=2 leaky=downstream "
        "     ! videoconvert "
        "     ! video/x-raw,format=BGR "
        "     ! appsink name=right_sink emit-signals=true sync=true  max-buffers=1 drop=true";

    GError* err = nullptr;
    pipeline_ = gst_parse_launch(desc.c_str(), &err);
    if (err) {
        std::cerr << "[Pipeline] Simülasyon hatası: " << err->message << "\n";
        g_error_free(err);
        return false;
    }

    leftAppsink_  = gst_bin_get_by_name(GST_BIN(pipeline_), "left_sink");
    rightAppsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "right_sink");
    std::cout << "[Pipeline] Simülasyon modu hazır (" << width_ << "x" << height_ << ")\n";
    return true;
}

// ─── Dual webcam: iki v4l2src ─────────────────────────────────────────────────
bool StereoPipeline::setupDualWebcam(const std::string& leftDev,
                                     const std::string& rightDev)
{
    std::string caps =
        "video/x-raw,format=BGR,width=" + std::to_string(width_) +
        ",height=" + std::to_string(height_);

    std::string desc =
        // Sol kamera
        "v4l2src device=" + leftDev +
        " ! videoconvert ! videoscale ! " + caps +
        " ! appsink name=left_sink  emit-signals=true sync=false max-buffers=1 drop=true "
        // Sağ kamera
        "v4l2src device=" + rightDev +
        " ! videoconvert ! videoscale ! " + caps +
        " ! appsink name=right_sink emit-signals=true sync=false max-buffers=1 drop=true";

    GError* err = nullptr;
    pipeline_ = gst_parse_launch(desc.c_str(), &err);
    if (err) {
        std::cerr << "[Pipeline] Webcam hatası: " << err->message << "\n";
        g_error_free(err);
        return false;
    }

    leftAppsink_  = gst_bin_get_by_name(GST_BIN(pipeline_), "left_sink");
    rightAppsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "right_sink");
    std::cout << "[Pipeline] Çift webcam modu hazır (" << leftDev << " | " << rightDev << ")\n";
    return true;
}

// ─── Video dosyası: filesrc → decodebin → tee → iki appsink ──────────────────
bool StereoPipeline::setupVideoFile(const std::string& filepath)
{
    std::string caps =
        "video/x-raw,format=BGR,width=" + std::to_string(width_) +
        ",height=" + std::to_string(height_);

    // uridecodebin yerine parsebin kullansaydık daha hızlı olurdu;
    // ancak decodebin daha evrenseldir.
    std::string desc =
        "filesrc location=\"" + filepath + "\" "
        "! decodebin "
        "! videoconvert ! videoscale "
        "! " + caps + " "
        "! tee name=t "
        "  t. ! queue max-size-buffers=2 leaky=downstream "
        "     ! appsink name=left_sink  emit-signals=true sync=true  max-buffers=1 drop=true "
        "  t. ! queue max-size-buffers=2 leaky=downstream "
        "     ! appsink name=right_sink emit-signals=true sync=true  max-buffers=1 drop=true";

    GError* err = nullptr;
    pipeline_ = gst_parse_launch(desc.c_str(), &err);
    if (err) {
        std::cerr << "[Pipeline] Dosya hatası: " << err->message << "\n";
        g_error_free(err);
        return false;
    }

    leftAppsink_  = gst_bin_get_by_name(GST_BIN(pipeline_), "left_sink");
    rightAppsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "right_sink");
    std::cout << "[Pipeline] Video dosyası modu hazır: " << filepath << "\n";
    return true;
}

// ─── appsink callback bağlama ─────────────────────────────────────────────────
void StereoPipeline::bindAppsinks()
{
    g_signal_connect(leftAppsink_,  "new-sample", G_CALLBACK(onLeftSample),  this);
    g_signal_connect(rightAppsink_, "new-sample", G_CALLBACK(onRightSample), this);
}

// ─── start / stop ─────────────────────────────────────────────────────────────
void StereoPipeline::start()
{
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[Pipeline] PLAYING durumuna geçilemedi!\n";
        return;
    }
    running_ = true;
    std::cout << "[Pipeline] Başlatıldı.\n";
}

void StereoPipeline::stop()
{
    running_ = false;
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    if (bus_) {
        gst_object_unref(bus_);
        bus_ = nullptr;
    }
    // appsink referansları pipeline unref ile zaten serbest kalır
    leftAppsink_  = nullptr;
    rightAppsink_ = nullptr;
}

// ─── getFrame ─────────────────────────────────────────────────────────────────
bool StereoPipeline::getFrame(StereoFrame& out, int timeoutMs)
{
    using namespace std::chrono;
    auto deadline = steady_clock::now() + milliseconds(timeoutMs);

    while (steady_clock::now() < deadline) {
        {
            std::unique_lock<std::mutex> lL(leftMtx_, std::defer_lock);
            std::unique_lock<std::mutex> lR(rightMtx_, std::defer_lock);
            std::lock(lL, lR);

            if (!leftQ_.empty() && !rightQ_.empty()) {
                out.left      = leftQ_.front().first;
                out.timestamp = leftQ_.front().second;
                leftQ_.pop();

                out.right = rightQ_.front().first;
                rightQ_.pop();

                out.valid = true;
                return true;
            }
        }
        std::this_thread::sleep_for(milliseconds(5));
    }

    out.valid = false;
    return false;
}

// ─── GStreamer örnek → cv::Mat ────────────────────────────────────────────────
cv::Mat StereoPipeline::sampleToMat(GstSample* sample)
{
    GstCaps*      caps   = gst_sample_get_caps(sample);
    GstBuffer*    buffer = gst_sample_get_buffer(sample);
    GstStructure* s      = gst_caps_get_structure(caps, 0);

    int w = 0, h = 0;
    gst_structure_get_int(s, "width",  &w);
    gst_structure_get_int(s, "height", &h);

    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    cv::Mat frame(h, w, CV_8UC3, map.data);
    cv::Mat result = frame.clone();   // pipeline buffer'dan bağımsız kopya
    gst_buffer_unmap(buffer, &map);

    return result;
}

// ─── Static callback'ler ──────────────────────────────────────────────────────
GstFlowReturn StereoPipeline::onLeftSample(GstAppSink* sink, gpointer data)
{
    auto* self   = static_cast<StereoPipeline*>(data);
    GstSample* s = gst_app_sink_pull_sample(sink);
    if (!s) return GST_FLOW_ERROR;

    cv::Mat        frame = self->sampleToMat(s);
    GstClockTime   ts    = GST_BUFFER_PTS(gst_sample_get_buffer(s));
    gst_sample_unref(s);

    std::lock_guard<std::mutex> lk(self->leftMtx_);
    // Kuyruk doluysa eskiyi at
    if (self->leftQ_.size() >= 3) self->leftQ_.pop();
    self->leftQ_.push({frame, ts});

    return GST_FLOW_OK;
}

GstFlowReturn StereoPipeline::onRightSample(GstAppSink* sink, gpointer data)
{
    auto* self   = static_cast<StereoPipeline*>(data);
    GstSample* s = gst_app_sink_pull_sample(sink);
    if (!s) return GST_FLOW_ERROR;

    cv::Mat        frame = self->sampleToMat(s);
    GstClockTime   ts    = GST_BUFFER_PTS(gst_sample_get_buffer(s));
    gst_sample_unref(s);

    std::lock_guard<std::mutex> lk(self->rightMtx_);
    if (self->rightQ_.size() >= 3) self->rightQ_.pop();
    self->rightQ_.push({frame, ts});

    return GST_FLOW_OK;
}
