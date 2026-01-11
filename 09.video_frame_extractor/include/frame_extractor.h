#ifndef FRAME_EXTRACTOR_H
#define FRAME_EXTRACTOR_H

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>

namespace gst_frame_extractor {

// Frame extraction mode
enum class ExtractionMode {
    INTERVAL,       // Extract frames at fixed intervals
    KEYFRAME,       // Extract only keyframes (I-frames)
    ALL_FRAMES,     // Extract all frames
    TIME_BASED      // Extract at specific timestamps
};

// Output format for extracted frames
enum class OutputFormat {
    PNG,
    JPEG,
    BMP
};

// Configuration structure
struct ExtractorConfig {
    std::string input_uri;              // Video file path or URI
    std::string output_dir;             // Output directory for frames
    std::string output_prefix;          // Prefix for output filenames
    ExtractionMode mode;                // Extraction mode
    OutputFormat format;                // Output format
    int interval_frames;                // Frame interval (for INTERVAL mode)
    double interval_seconds;            // Time interval (for TIME_BASED mode)
    std::vector<double> timestamps;     // Specific timestamps (for TIME_BASED mode)
    int jpeg_quality;                   // JPEG quality (1-100)
    int max_frames;                     // Maximum frames to extract (0 = unlimited)
    bool resize_output;                 // Enable resizing
    int output_width;                   // Output width (if resize enabled)
    int output_height;                  // Output height (if resize enabled)
    bool add_timestamp_overlay;         // Add timestamp text overlay
};

// Frame callback function type
using FrameCallback = std::function<void(const cv::Mat&, int64_t pts, int frame_number)>;

class FrameExtractor {
public:
    FrameExtractor();
    ~FrameExtractor();

    // Initialize extractor with configuration
    bool initialize(const ExtractorConfig& config);

    // Start extraction
    bool start();

    // Stop extraction
    void stop();

    // Wait for extraction to complete
    void waitForCompletion();

    // Get extracted frame count
    int getExtractedFrameCount() const;

    // Get total frame count (if known)
    int64_t getTotalFrameCount() const;

    // Get video duration in nanoseconds
    int64_t getDuration() const;

    // Get current position in nanoseconds
    int64_t getCurrentPosition() const;

    // Set frame callback for custom processing
    void setFrameCallback(FrameCallback callback);

    // Check if extraction is running
    bool isRunning() const;

    // Get last error message
    std::string getLastError() const;

    // Load configuration from YAML file
    static ExtractorConfig loadConfig(const std::string& config_path);

private:
    // GStreamer callbacks
    static GstFlowReturn onNewSample(GstAppSink* sink, gpointer user_data);
    static gboolean onBusMessage(GstBus* bus, GstMessage* msg, gpointer user_data);
    static GstPadProbeReturn onPadProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

    // Internal methods
    bool createPipeline();
    bool setupAppsink();
    void processFrame(GstSample* sample);
    bool saveFrame(const cv::Mat& frame, int frame_number, int64_t pts);
    std::string getOutputPath(int frame_number, int64_t pts);
    std::string formatTimestamp(int64_t pts);
    bool shouldExtractFrame(int frame_number, int64_t pts);
    void queryDuration();

    // Member variables
    ExtractorConfig config_;
    GstElement* pipeline_;
    GstElement* appsink_;
    GstBus* bus_;
    GMainLoop* main_loop_;

    std::atomic<bool> running_;
    std::atomic<int> frame_count_;
    std::atomic<int> extracted_count_;
    std::atomic<int64_t> duration_;
    std::atomic<int64_t> current_position_;

    FrameCallback frame_callback_;
    std::string last_error_;

    // Thread for main loop
    std::unique_ptr<std::thread> loop_thread_;
};

} // namespace gst_frame_extractor

#endif // FRAME_EXTRACTOR_H
