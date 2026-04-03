#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <utility>

// ─── Data structure ──────────────────────────────────────────────────────────
struct StereoFrame {
    cv::Mat        left;
    cv::Mat        right;
    GstClockTime   timestamp = 0;
    bool           valid     = false;
};

// ─── StereoPipeline ───────────────────────────────────────────────────────────
//
// Supports three source modes:
//   SIMULATION  – single videotestsrc, distributed to two appsinks via tee
//   DUAL_WEBCAM – two v4l2src (two USB cameras)
//   VIDEO_FILE  – a video file, distributed to two appsinks via tee
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

    // Returns the most recent stereo pair; returns valid=false if no new
    // frame arrives within timeout_ms.
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

    // Pipeline setup functions
    bool setupSimulation();
    bool setupDualWebcam(const std::string& leftDev, const std::string& rightDev);
    bool setupVideoFile(const std::string& filepath);

    // Common appsink configuration
    void bindAppsinks();

    // GStreamer sample → cv::Mat conversion
    cv::Mat sampleToMat(GstSample* sample);

    // appsink "new-sample" callbacks
    static GstFlowReturn onLeftSample (GstAppSink* sink, gpointer data);
    static GstFlowReturn onRightSample(GstAppSink* sink, gpointer data);
};
