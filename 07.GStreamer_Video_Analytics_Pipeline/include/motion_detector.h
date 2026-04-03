/**
 * @file motion_detector.h
 * @brief Motion detection class for video streams
 *
 * This class provides functionality for detecting motion by analyzing
 * video frames, marking motion regions, and reporting motion events.
 */

#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <chrono>
#include <deque>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>
#endif

/**
 * @brief Motion detection algorithms
 */
enum class MotionAlgorithm {
    FRAME_DIFF,         // Simple frame difference
    BACKGROUND_SUB,     // Background subtraction
    OPTICAL_FLOW,       // Optical flow
    MOG2,              // Mixture of Gaussians v2
    KNN                // K-Nearest Neighbors
};

/**
 * @brief Motion region information
 */
struct MotionRegion {
    int x;              // Top-left corner X coordinate
    int y;              // Top-left corner Y coordinate
    int width;          // Width
    int height;         // Height
    double intensity;   // Motion intensity (0.0 - 1.0)
    guint64 timestamp;  // Detection time
};

/**
 * @brief Motion detection parameters
 */
struct MotionDetectionParams {
    // Algorithm selection
    MotionAlgorithm algorithm = MotionAlgorithm::MOG2;

    // Sensitivity settings
    double sensitivity = 0.5;           // 0.0 (low) - 1.0 (high)
    double threshold = 25.0;            // Pixel change threshold

    // Background subtraction parameters
    int history_length = 500;           // Background history length
    double learning_rate = 0.005;       // Learning rate
    bool detect_shadows = true;         // Shadow detection

    // Morphological operations
    int erosion_size = 2;              // Erosion kernel size
    int dilation_size = 4;             // Dilation kernel size

    // Region filtering
    int min_area = 500;                // Minimum motion area (pixels squared)
    int max_area = 100000;             // Maximum motion area
    double min_aspect_ratio = 0.3;     // Minimum aspect ratio
    double max_aspect_ratio = 3.0;     // Maximum aspect ratio

    // Motion tracking
    bool enable_tracking = true;        // Object tracking
    int max_tracked_objects = 10;       // Maximum tracked objects

    // Visualization
    bool draw_motion_regions = true;    // Draw motion regions
    bool draw_motion_vectors = false;   // Draw motion vectors
    bool show_debug_view = false;       // Debug view

    // Alert settings
    bool enable_alerts = true;          // Motion alerts
    double alert_threshold = 0.1;       // Alert threshold (total area percentage)
    int alert_cooldown_ms = 5000;      // Alert cooldown time
};

/**
 * @brief Motion statistics
 */
struct MotionStats {
    guint64 total_frames = 0;          // Total frames processed
    guint64 motion_frames = 0;         // Frames with motion detected
    double average_motion_area = 0.0;   // Average motion area
    double max_motion_area = 0.0;       // Maximum motion area
    guint64 total_regions = 0;         // Total motion regions
    std::chrono::time_point<std::chrono::steady_clock> last_motion_time;
};

/**
 * @brief Motion event callback type
 * @param regions Detected motion regions
 * @param timestamp Event time
 * @param motion_percentage Motion percentage of total area
 */
using MotionEventCallback = std::function<void(
    const std::vector<MotionRegion>&, guint64, double)>;

/**
 * @brief Motion detection class
 */
class MotionDetector {
public:
    /**
     * @brief Constructor
     */
    MotionDetector();

    /**
     * @brief Destructor
     */
    ~MotionDetector();

    /**
     * @brief Creates a GStreamer element
     * @return Motion detection element
     */
    GstElement* createElement();

    /**
     * @brief Sets detection parameters
     * @param params New parameters
     */
    void setParameters(const MotionDetectionParams& params);

    /**
     * @brief Returns current parameters
     * @return Detection parameters
     */
    MotionDetectionParams getParameters() const;

    /**
     * @brief Changes the motion detection algorithm
     * @param algorithm New algorithm
     */
    void setAlgorithm(MotionAlgorithm algorithm);

    /**
     * @brief Sets sensitivity level
     * @param sensitivity 0.0 (low) - 1.0 (high)
     */
    void setSensitivity(double sensitivity);

    /**
     * @brief Sets the motion event callback
     * @param callback Event function
     */
    void setMotionEventCallback(MotionEventCallback callback);

    /**
     * @brief Returns statistics
     * @return Motion statistics
     */
    MotionStats getStats() const;

    /**
     * @brief Resets statistics
     */
    void resetStats();

    /**
     * @brief Returns current motion regions
     * @return List of motion regions
     */
    std::vector<MotionRegion> getCurrentMotions() const;

    /**
     * @brief Starts/stops motion detection
     * @param enable true to start
     */
    void setEnabled(bool enable);

    /**
     * @brief Returns detection state
     * @return true if active
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Sets the region of interest (ROI)
     * @param x Top-left X coordinate
     * @param y Top-left Y coordinate
     * @param width Width
     * @param height Height
     */
    void setROI(int x, int y, int width, int height);

    /**
     * @brief Clears the region of interest
     */
    void clearROI();

    /**
     * @brief Adds an exclusion zone
     * @param x Top-left X coordinate
     * @param y Top-left Y coordinate
     * @param width Width
     * @param height Height
     */
    void addExclusionZone(int x, int y, int width, int height);

    /**
     * @brief Clears all exclusion zones
     */
    void clearExclusionZones();

#ifdef HAVE_OPENCV
    /**
     * @brief Returns the OpenCV-based motion mask
     * @return Motion mask (binary image)
     */
    cv::Mat getMotionMask() const;

    /**
     * @brief Returns the background model
     * @return Background image
     */
    cv::Mat getBackgroundModel() const;
#endif

private:
    /**
     * @brief Processes a video frame
     * @param buffer GStreamer buffer
     * @param info Video information
     * @return Processed buffer
     */
    GstBuffer* processFrame(GstBuffer* buffer, const GstVideoInfo* info);

#ifdef HAVE_OPENCV
    /**
     * @brief Detect motion with OpenCV
     * @param frame Current frame
     * @return Detected motion regions
     */
    std::vector<MotionRegion> detectMotion(const cv::Mat& frame);

    /**
     * @brief Frame difference algorithm
     * @param frame Current frame
     * @return Motion mask
     */
    cv::Mat frameDifference(const cv::Mat& frame);

    /**
     * @brief Background subtraction algorithm
     * @param frame Current frame
     * @return Motion mask
     */
    cv::Mat backgroundSubtraction(const cv::Mat& frame);

    /**
     * @brief Optical flow algorithm
     * @param frame Current frame
     * @return Motion vectors
     */
    std::vector<cv::Point2f> opticalFlow(const cv::Mat& frame);

    /**
     * @brief Apply morphological operations
     * @param mask Binary motion mask
     */
    void applyMorphology(cv::Mat& mask);

    /**
     * @brief Find motion regions
     * @param mask Binary motion mask
     * @return Motion regions
     */
    std::vector<MotionRegion> findMotionRegions(const cv::Mat& mask);

    /**
     * @brief Filter motion regions
     * @param regions Raw motion regions
     * @return Filtered regions
     */
    std::vector<MotionRegion> filterRegions(const std::vector<MotionRegion>& regions);

    /**
     * @brief Draw motion regions
     * @param frame Frame to draw on
     * @param regions Motion regions
     */
    void drawMotionRegions(cv::Mat& frame, const std::vector<MotionRegion>& regions);
#endif

    /**
     * @brief Transform callback
     */
    static GstFlowReturn transformCallback(GstBaseTransform* trans,
                                         GstBuffer* inbuf,
                                         GstBuffer* outbuf);

    /**
     * @brief Trigger motion event
     * @param regions Motion regions
     * @param timestamp Event time
     */
    void triggerMotionEvent(const std::vector<MotionRegion>& regions, guint64 timestamp);

    // Member variables
    GstElement* element_ = nullptr;              // GStreamer element
    MotionDetectionParams params_;               // Detection parameters
    MotionStats stats_;                         // Statistics
    mutable std::mutex params_mutex_;           // Parameter mutex
    mutable std::mutex stats_mutex_;            // Statistics mutex

    bool enabled_ = true;                       // Detection state
    MotionEventCallback motion_callback_;       // Motion event callback

#ifdef HAVE_OPENCV
    // OpenCV variables
    cv::Mat previous_frame_;                    // Previous frame
    cv::Mat background_model_;                  // Background model
    cv::Mat motion_mask_;                       // Motion mask

    // Background subtractors
    cv::Ptr<cv::BackgroundSubtractor> bg_subtractor_;

    // For optical flow
    cv::Mat previous_gray_;                     // Previous grayscale frame
    std::vector<cv::Point2f> previous_points_;  // Previous feature points
#endif

    // Motion regions
    std::vector<MotionRegion> current_motions_; // Current motion regions
    mutable std::mutex motions_mutex_;          // Motion regions mutex

    // ROI and exclusion zones
    cv::Rect roi_;                              // Region of interest
    bool has_roi_ = false;                      // ROI defined
    std::vector<cv::Rect> exclusion_zones_;     // Exclusion zones

    // Alert management
    std::chrono::time_point<std::chrono::steady_clock> last_alert_time_;
    bool in_cooldown_ = false;                  // Alert cooldown state

    // Motion history (for tracking)
    std::deque<std::vector<MotionRegion>> motion_history_;
    const size_t max_history_size_ = 30;        // 1 second history (30fps)
};

#endif // MOTION_DETECTOR_H
