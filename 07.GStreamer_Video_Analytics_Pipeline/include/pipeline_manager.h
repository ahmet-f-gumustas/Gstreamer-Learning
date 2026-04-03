/**
 * @file pipeline_manager.h
 * @brief Main class for GStreamer pipeline management
 *
 * This class is responsible for creating, configuring, and managing
 * the GStreamer pipeline. It manages video input/output sources,
 * processing elements, and stream control.
 */

#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <map>

// Forward declarations
class VideoProcessor;
class MotionDetector;
class RTSPStreamer;

/**
 * @brief Defines video source types
 */
enum class SourceType {
    FILE,        // Video file
    WEBCAM,      // USB/V4L2 camera
    RTSP,        // RTSP stream
    HTTP,        // HTTP stream
    APPSRC       // Application source
};

/**
 * @brief Defines output target types
 */
enum class SinkType {
    DISPLAY,     // Display on screen
    FILE,        // Record to file
    RTSP,        // RTSP server
    APPSINK      // Application sink
};

/**
 * @brief Pipeline configuration parameters
 */
struct PipelineConfig {
    // Input parameters
    SourceType source_type = SourceType::FILE;
    std::string source_location = "";

    // Video parameters
    int width = 1920;
    int height = 1080;
    int framerate = 30;
    std::string video_format = "I420";

    // Output parameters
    SinkType sink_type = SinkType::DISPLAY;
    std::string sink_location = "";

    // Processing options
    bool enable_motion_detection = false;
    bool enable_gpu_acceleration = false;
    bool enable_recording = false;
    std::string record_location = "";

    // Codec settings
    std::string encoder = "x264enc";
    int bitrate = 4000000; // 4 Mbps

    // RTSP server settings
    std::string rtsp_mount_point = "/live";
    int rtsp_port = 8554;
};

/**
 * @brief GStreamer pipeline manager class
 */
class PipelineManager {
public:
    /**
     * @brief Constructor
     * @param config Pipeline configuration parameters
     */
    explicit PipelineManager(const PipelineConfig& config);

    /**
     * @brief Destructor - Cleans up resources
     */
    ~PipelineManager();

    // Disable copy and move operations
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;

    /**
     * @brief Starts the pipeline
     * @return true if successful
     */
    bool start();

    /**
     * @brief Stops the pipeline
     */
    void stop();

    /**
     * @brief Checks if the pipeline is running
     * @return true if running
     */
    bool isRunning() const { return is_running_.load(); }

    /**
     * @brief Returns the current FPS value
     * @return Frames per second
     */
    double getCurrentFPS() const { return current_fps_; }

    /**
     * @brief Returns the video processor object
     * @return VideoProcessor pointer
     */
    VideoProcessor* getVideoProcessor() { return video_processor_.get(); }

    /**
     * @brief Returns the motion detector object
     * @return MotionDetector pointer
     */
    MotionDetector* getMotionDetector() { return motion_detector_.get(); }

    /**
     * @brief Returns the RTSP streamer object
     * @return RTSPStreamer pointer
     */
    RTSPStreamer* getRTSPStreamer() { return rtsp_streamer_.get(); }

    /**
     * @brief Adds a new element to the pipeline
     * @param element GStreamer element to add
     * @param name Element name
     * @return true if successful
     */
    bool addElement(GstElement* element, const std::string& name);

    /**
     * @brief Removes an element from the pipeline
     * @param name Name of element to remove
     * @return true if successful
     */
    bool removeElement(const std::string& name);

    /**
     * @brief Changes the video source at runtime
     * @param new_source New source location
     * @param type New source type
     * @return true if successful
     */
    bool changeSource(const std::string& new_source, SourceType type);

    /**
     * @brief Starts/stops recording
     * @param start true to start, false to stop
     * @param filename Recording file name
     * @return true if successful
     */
    bool toggleRecording(bool start, const std::string& filename = "");

    /**
     * @brief Returns the pipeline state
     * @return GStreamer state enum
     */
    GstState getState() const;

    /**
     * @brief Sets the error callback
     * @param callback Function to call on error
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        error_callback_ = callback;
    }

    /**
     * @brief Sets the EOS (End of Stream) callback
     * @param callback Function to call when stream ends
     */
    void setEOSCallback(std::function<void()> callback) {
        eos_callback_ = callback;
    }

private:
    /**
     * @brief Creates the pipeline
     * @return true if successful
     */
    bool createPipeline();

    /**
     * @brief Creates the video source
     * @return Created source element
     */
    GstElement* createSource();

    /**
     * @brief Creates the video sink
     * @return Created sink element
     */
    GstElement* createSink();

    /**
     * @brief Creates the video encoder
     * @return Created encoder element
     */
    GstElement* createEncoder();

    /**
     * @brief Handles bus messages
     * @param bus GStreamer bus
     * @param msg Message
     * @param user_data User data (this pointer)
     * @return TRUE to continue, FALSE to stop
     */
    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer user_data);

    /**
     * @brief Handles pad-added signal
     * @param src Source element
     * @param new_pad New pad
     * @param data User data
     */
    static void onPadAdded(GstElement* src, GstPad* new_pad, gpointer data);

    /**
     * @brief Main event loop thread function
     */
    void mainLoopThread();

    /**
     * @brief FPS calculation function
     */
    void calculateFPS();

    /**
     * @brief Cleans up the pipeline
     */
    void cleanup();

    // Member variables
    PipelineConfig config_;                                    // Pipeline configuration

    // GStreamer elements
    GstElement* pipeline_ = nullptr;                           // Main pipeline
    GstElement* source_ = nullptr;                             // Video source
    GstElement* sink_ = nullptr;                               // Video sink
    GstElement* tee_ = nullptr;                                // For splitting the stream
    std::map<std::string, GstElement*> elements_;             // All elements

    // Helper classes
    std::unique_ptr<VideoProcessor> video_processor_;          // Video processor
    std::unique_ptr<MotionDetector> motion_detector_;          // Motion detector
    std::unique_ptr<RTSPStreamer> rtsp_streamer_;             // RTSP streamer

    // Thread and synchronization
    std::unique_ptr<std::thread> main_loop_thread_;           // Main loop thread
    GMainLoop* main_loop_ = nullptr;                          // GLib main loop
    std::atomic<bool> is_running_{false};                      // Running state

    // Performance metrics
    double current_fps_ = 0.0;                                 // Current FPS
    guint64 frame_count_ = 0;                                 // Total frame count
    GstClockTime last_fps_time_ = 0;                          // Last FPS calculation time

    // Callback functions
    std::function<void(const std::string&)> error_callback_;   // Error callback
    std::function<void()> eos_callback_;                       // EOS callback

    // Additional elements for recording
    GstElement* recording_queue_ = nullptr;                    // Recording queue
    GstElement* recording_sink_ = nullptr;                     // Recording sink
    bool is_recording_ = false;                                // Recording state
};
