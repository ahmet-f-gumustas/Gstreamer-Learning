#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <utility>

// ─── Veri yapısı ─────────────────────────────────────────────────────────────
struct StereoFrame {
    cv::Mat        left;
    cv::Mat        right;
    GstClockTime   timestamp = 0;
    bool           valid     = false;
};

// ─── StereoPipeline ───────────────────────────────────────────────────────────
//
// Üç kaynak modu destekler:
//   SIMULATION  – tek videotestsrc, tee ile iki appsink'e dağıtılır
//   DUAL_WEBCAM – iki v4l2src (iki USB kamera)
//   VIDEO_FILE  – bir video dosyası, tee ile iki appsink'e dağıtılır
//
class StereoPipeline {
public:
    enum class SourceMode {
        SIMULATION,
        DUAL_WEBCAM,
        VIDEO_FILE
    };

    StereoPipeline();
    ~StereoPipeline();

    bool initialize(SourceMode mode,
                    const std::string& leftSource  = "/dev/video0",
                    const std::string& rightSource = "/dev/video2",
                    int width  = 640,
                    int height = 480);

    void start();
    void stop();
    bool isRunning() const { return running_; }

    // En güncel stereo çiftini döner; timeout_ms içinde yeni frame gelmezse
    // valid=false döner.
    bool getFrame(StereoFrame& out, int timeoutMs = 200);

private:
    GstElement*  pipeline_     = nullptr;
    GstElement*  leftAppsink_  = nullptr;
    GstElement*  rightAppsink_ = nullptr;
    GstBus*      bus_          = nullptr;

    std::mutex leftMtx_, rightMtx_;
    std::queue<std::pair<cv::Mat, GstClockTime>> leftQ_, rightQ_;

    std::atomic<bool> running_{false};
    int width_  = 640;
    int height_ = 480;

    // Pipeline kurulum fonksiyonları
    bool setupSimulation();
    bool setupDualWebcam(const std::string& leftDev, const std::string& rightDev);
    bool setupVideoFile(const std::string& filepath);

    // Ortak appsink konfigürasyonu
    void bindAppsinks();

    // GStreamer örnek → cv::Mat dönüşümü
    cv::Mat sampleToMat(GstSample* sample);

    // appsink "new-sample" callback'leri
    static GstFlowReturn onLeftSample (GstAppSink* sink, gpointer data);
    static GstFlowReturn onRightSample(GstAppSink* sink, gpointer data);
};
