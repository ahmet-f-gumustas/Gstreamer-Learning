#include "stereo_pipeline.h"
#include "depth_estimator.h"
#include "obstacle_detector.h"

#include <gst/gst.h>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <csignal>
#include <atomic>
#include <memory>
#include <string>
#include <chrono>

// ─── Global stop flag ────────────────────────────────────────────────────────
static std::atomic<bool> g_stop{false};

static void signalHandler(int) { g_stop = true; }

// ─── Help message ────────────────────────────────────────────────────────────
static void printUsage(const char* name)
{
    std::cout
        << "\nUsage: " << name << " [OPTIONS]\n\n"
        << "Source modes:\n"
        << "  --sim                Simulation (default, no camera needed)\n"
        << "  --webcam             Dual USB camera\n"
        << "  --file   <path>      Video file\n\n"
        << "Camera options:\n"
        << "  --left   <device>    Left camera (default: /dev/video0)\n"
        << "  --right  <device>    Right camera (default: /dev/video2)\n"
        << "  --width  <px>        Width (default: 640)\n"
        << "  --height <px>        Height (default: 480)\n\n"
        << "Depth options:\n"
        << "  --sgbm               Use StereoSGBM (more accurate, slower)\n"
        << "  --focal  <px>        Focal length in pixels (default: 554)\n"
        << "  --base   <m>         Baseline in meters (default: 0.06)\n"
        << "  --calib  <yaml>      Calibration file\n\n"
        << "Obstacle options:\n"
        << "  --danger  <m>        Danger threshold (default: 1.0 m)\n"
        << "  --caution <m>        Caution threshold (default: 3.0 m)\n\n"
        << "Other:\n"
        << "  --save   <path>      Save output as video\n"
        << "  -h, --help           Show this message\n\n"
        << "Examples:\n"
        << "  " << name << " --sim\n"
        << "  " << name << " --webcam --left /dev/video0 --right /dev/video2\n"
        << "  " << name << " --file video.mp4 --sgbm\n";
}

// ─── Window layout ───────────────────────────────────────────────────────────
static cv::Mat buildDisplay(const cv::Mat& left,
                            const cv::Mat& right,
                            const cv::Mat& colorDepth,
                            const cv::Mat& overlay)
{
    // Resize every image to the same size
    cv::Size sz = left.size();
    cv::Mat rR, rCD, rOV;
    cv::resize(right,      rR,  sz);
    cv::resize(colorDepth, rCD, sz);
    cv::resize(overlay,    rOV, sz);

    // Add labels
    auto label = [](cv::Mat& img, const std::string& txt) {
        cv::rectangle(img, cv::Rect(0, 0, img.cols, 22), cv::Scalar(20,20,20), cv::FILLED);
        cv::putText(img, txt, cv::Point(4, 16),
                    cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(220,220,220), 1);
    };

    cv::Mat L = left.clone(), R = rR.clone(), D = rCD.clone(), O = rOV.clone();
    label(L, "Left Camera");
    label(R, "Right Camera");
    label(D, "Depth Map  (red=near, blue=far)");
    label(O, "Obstacle Analysis");

    cv::Mat top, bot, full;
    cv::hconcat(L, R,   top);
    cv::hconcat(D, O,   bot);
    cv::vconcat(top, bot, full);
    return full;
}

// ─── FPS counter ─────────────────────────────────────────────────────────────
class FpsCounter {
    std::chrono::steady_clock::time_point t0_ = std::chrono::steady_clock::now();
    int frames_ = 0;
    float fps_  = 0.f;
public:
    float tick() {
        ++frames_;
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - t0_).count();
        if (dt >= 1.0f) {
            fps_    = frames_ / dt;
            frames_ = 0;
            t0_     = now;
        }
        return fps_;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    signal(SIGINT,  signalHandler);
    signal(SIGTERM, signalHandler);

    gst_init(&argc, &argv);

    // ── Argument parsing ──────────────────────────────────────────────────────
    StereoPipeline::SourceMode mode = StereoPipeline::SourceMode::SIMULATION;
    std::string leftSrc  = "/dev/video0";
    std::string rightSrc = "/dev/video2";
    std::string filePath;
    std::string calibPath;
    std::string savePath;
    int   width   = 640;
    int   height  = 480;
    float focal   = 554.f;
    float base    = 0.06f;
    float danger  = 1.0f;
    float caution = 3.0f;
    bool  useSGBM = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            return (i + 1 < argc) ? argv[++i] : "";
        };

        if      (a == "--sim")                mode     = StereoPipeline::SourceMode::SIMULATION;
        else if (a == "--webcam")             mode     = StereoPipeline::SourceMode::DUAL_WEBCAM;
        else if (a == "--file")             { mode     = StereoPipeline::SourceMode::VIDEO_FILE;
                                              filePath = next(); }
        else if (a == "--left")               leftSrc  = next();
        else if (a == "--right")              rightSrc = next();
        else if (a == "--width")              width    = std::stoi(next());
        else if (a == "--height")             height   = std::stoi(next());
        else if (a == "--focal")              focal    = std::stof(next());
        else if (a == "--base")               base     = std::stof(next());
        else if (a == "--danger")             danger   = std::stof(next());
        else if (a == "--caution")            caution  = std::stof(next());
        else if (a == "--calib")              calibPath = next();
        else if (a == "--save")               savePath  = next();
        else if (a == "--sgbm")               useSGBM  = true;
        else if (a == "-h" || a == "--help") { printUsage(argv[0]); return 0; }
        else std::cerr << "[main] Unknown argument: " << a << "\n";
    }

    // ── Component initialization ───────────────────────────────────────────────
    StereoPipeline pipeline;
    std::string src = (mode == StereoPipeline::SourceMode::VIDEO_FILE) ? filePath : leftSrc;
    if (!pipeline.initialize(mode, src, rightSrc, width, height)) {
        std::cerr << "[main] Pipeline could not be initialized!\n";
        gst_deinit();
        return 1;
    }

    DepthEstimator depth;
    depth.setAlgorithm(useSGBM ? DepthEstimator::Algorithm::SEMI_GLOBAL_BM
                               : DepthEstimator::Algorithm::BLOCK_MATCHING);
    depth.setFocalLength(focal);
    depth.setBaseline(base);

    // In simulation mode, generate artificial right image
    if (mode == StereoPipeline::SourceMode::SIMULATION)
        depth.setSim(true, /*shift_px=*/30);

    if (!calibPath.empty())
        depth.loadCalibration(calibPath);

    ObstacleDetector detector(danger, caution, /*rows=*/3, /*cols=*/4);

    // Video recording
    cv::VideoWriter writer;
    bool doSave = !savePath.empty();

    // ── Start pipeline ────────────────────────────────────────────────────────
    pipeline.start();

    cv::namedWindow("Stereo Depth Pipeline", cv::WINDOW_NORMAL);
    cv::resizeWindow("Stereo Depth Pipeline", width * 2, height * 2);

    FpsCounter fps;
    StereoFrame frame;
    int frameIdx = 0;

    std::cout << "\n[main] Running. Press 'q' or Ctrl+C to exit\n";
    std::cout << "[main] Mode: "
              << (mode == StereoPipeline::SourceMode::SIMULATION ? "Simulation" :
                  mode == StereoPipeline::SourceMode::DUAL_WEBCAM ? "Dual Webcam" : "Video File")
              << "  Algo: " << (useSGBM ? "SGBM" : "BM")
              << "  f=" << focal << "px  B=" << base * 100.f << "cm\n\n";

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (!g_stop && pipeline.isRunning()) {

        if (!pipeline.getFrame(frame)) {
            // Timeout – check bus messages
            continue;
        }

        // Depth computation
        DepthResult depth_r = depth.compute(frame.left, frame.right);
        if (!depth_r.valid) continue;

        // Obstacle analysis
        ObstacleResult obs = detector.analyze(depth_r.depthMap, frame.left);

        // Display image
        cv::Mat display = buildDisplay(frame.left, frame.right,
                                       depth_r.colorDepth, obs.overlay);

        // Show FPS
        float f = fps.tick();
        if (f > 0.f) {
            std::ostringstream fpsStr;
            fpsStr << std::fixed << std::setprecision(1) << f << " fps";
            cv::putText(display, fpsStr.str(),
                        cv::Point(display.cols - 100, display.rows - 8),
                        cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(200,200,200), 1);
        }

        cv::imshow("Stereo Depth Pipeline", display);

        // Video recording
        if (doSave) {
            if (!writer.isOpened())
                writer.open(savePath, cv::VideoWriter::fourcc('M','J','P','G'),
                            30, display.size());
            writer.write(display);
        }

        // Terminal output (every 30 frames)
        if (++frameIdx % 30 == 0) {
            std::cout << "[Frame " << frameIdx << "] "
                      << "Min: " << std::fixed << std::setprecision(2) << depth_r.minDepthM << "m  "
                      << "Max: " << depth_r.maxDepthM << "m  "
                      << "Closest obstacle: " << obs.closestM << "m  "
                      << "Status: ";
            switch (obs.overallDanger) {
                case DangerLevel::DANGER:  std::cout << "DANGER\n"; break;
                case DangerLevel::CAUTION: std::cout << "CAUTION\n";  break;
                default:                   std::cout << "SAFE\n"; break;
            }
        }

        int key = cv::waitKey(1);
        if (key == 'q' || key == 'Q' || key == 27) g_stop = true;
    }

    // ── Cleanup ──────────────────────────────────────────────────────────────
    pipeline.stop();
    if (writer.isOpened()) writer.release();
    cv::destroyAllWindows();
    gst_deinit();

    std::cout << "[main] Program terminated. Total frames: " << frameIdx << "\n";
    return 0;
}
