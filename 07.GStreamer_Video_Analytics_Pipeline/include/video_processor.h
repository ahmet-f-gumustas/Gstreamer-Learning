/**
 * @file video_processor.h
 * @brief Video processing and filter application class
 *
 * This class is used to process video frames, apply filters,
 * and perform image enhancements in the GStreamer pipeline.
 */

#ifndef VIDEO_PROCESSOR_H
#define VIDEO_PROCESSOR_H

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/base/gstbasetransform.h>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

/**
 * @brief Video filter types
 */
enum class FilterType {
    NONE,           // No filter
    GRAYSCALE,      // Grayscale
    BLUR,           // Blur
    SHARPEN,        // Sharpen
    EDGE_DETECT,    // Edge detection
    DENOISE,        // Noise reduction
    BRIGHTNESS,     // Brightness adjustment
    CONTRAST,       // Contrast adjustment
    CUSTOM          // Custom filter
};

/**
 * @brief Video processing parameters
 */
struct ProcessingParams {
    // Filter parameters
    FilterType filter_type = FilterType::NONE;

    // Brightness and contrast settings
    double brightness = 0.0;    // -100 to 100
    double contrast = 1.0;      // 0.5 to 3.0
    double saturation = 1.0;    // 0.0 to 2.0
    double hue = 0.0;          // -180 to 180

    // Blur parameters
    int blur_kernel_size = 5;   // Must be odd

    // Sharpening parameters
    double sharpen_strength = 1.0;

    // Noise reduction parameters
    double denoise_strength = 10.0;

    // Edge detection parameters
    double edge_threshold1 = 100.0;
    double edge_threshold2 = 200.0;

    // Rotation and flip
    int rotation = 0;           // 0, 90, 180, 270 degrees
    bool flip_horizontal = false;
    bool flip_vertical = false;

    // Crop parameters
    int crop_x = 0;
    int crop_y = 0;
    int crop_width = 0;
    int crop_height = 0;

    // Scaling
    bool enable_scaling = false;
    int scale_width = 0;
    int scale_height = 0;
};

/**
 * @brief Video processing statistics
 */
struct ProcessingStats {
    guint64 frames_processed = 0;      // Total frames processed
    double avg_processing_time = 0.0;  // Average processing time (ms)
    double min_processing_time = 999999.0;
    double max_processing_time = 0.0;
    guint64 dropped_frames = 0;        // Dropped frame count
};

/**
 * @brief Video processing callback type
 * @param buffer GStreamer buffer to process
 * @param width Video width
 * @param height Video height
 * @return Processed buffer (nullptr to use original)
 */
using ProcessingCallback = std::function<GstBuffer*(GstBuffer*, int, int)>;

/**
 * @brief Video processing class
 */
class VideoProcessor {
public:
    /**
     * @brief Constructor
     */
    VideoProcessor();

    /**
     * @brief Destructor
     */
    ~VideoProcessor();

    /**
     * @brief Creates the video processing element
     * @return GStreamer element
     */
    GstElement* createElement();

    /**
     * @brief Sets processing parameters
     * @param params New parameters
     */
    void setParameters(const ProcessingParams& params);

    /**
     * @brief Returns current parameters
     * @return Processing parameters
     */
    ProcessingParams getParameters() const;

    /**
     * @brief Changes the filter type
     * @param type New filter type
     */
    void setFilter(FilterType type);

    /**
     * @brief Sets the custom processing callback
     * @param callback Processing function
     */
    void setCustomProcessor(ProcessingCallback callback);

    /**
     * @brief Returns processing statistics
     * @return Statistics structure
     */
    ProcessingStats getStats() const;

    /**
     * @brief Resets statistics
     */
    void resetStats();

    /**
     * @brief Enables/disables GPU acceleration
     * @param enable true to enable
     */
    void setGPUAcceleration(bool enable);

    /**
     * @brief Takes a snapshot
     * @param filename Output file name
     * @return true if successful
     */
    bool takeSnapshot(const std::string& filename);

    /**
     * @brief Adds video metadata
     * @param key Metadata key
     * @param value Metadata value
     */
    void addMetadata(const std::string& key, const std::string& value);

#ifdef HAVE_OPENCV
    /**
     * @brief Converts to OpenCV Mat object
     * @param buffer GStreamer buffer
     * @param info Video information
     * @return OpenCV Mat object
     */
    cv::Mat bufferToMat(GstBuffer* buffer, const GstVideoInfo* info);

    /**
     * @brief Creates buffer from OpenCV Mat object
     * @param mat OpenCV Mat object
     * @param info Video information
     * @return GStreamer buffer
     */
    GstBuffer* matToBuffer(const cv::Mat& mat, const GstVideoInfo* info);
#endif

private:
    /**
     * @brief Processes a video frame
     * @param buffer Buffer to process
     * @param info Video information
     * @return Processed buffer
     */
    GstBuffer* processFrame(GstBuffer* buffer, const GstVideoInfo* info);

    /**
     * @brief Applies grayscale filter
     * @param data Video data
     * @param info Video information
     */
    void applyGrayscale(guint8* data, const GstVideoInfo* info);

    /**
     * @brief Adjusts brightness/contrast
     * @param data Video data
     * @param info Video information
     */
    void adjustBrightnessContrast(guint8* data, const GstVideoInfo* info);

#ifdef HAVE_OPENCV
    /**
     * @brief Applies OpenCV-based filter
     * @param mat OpenCV Mat object
     * @param type Filter type
     */
    void applyOpenCVFilter(cv::Mat& mat, FilterType type);

    /**
     * @brief Blur filter (OpenCV)
     * @param mat Image matrix
     */
    void applyBlur(cv::Mat& mat);

    /**
     * @brief Sharpening filter (OpenCV)
     * @param mat Image matrix
     */
    void applySharpen(cv::Mat& mat);

    /**
     * @brief Edge detection filter (OpenCV)
     * @param mat Image matrix
     */
    void applyEdgeDetection(cv::Mat& mat);

    /**
     * @brief Noise reduction filter (OpenCV)
     * @param mat Image matrix
     */
    void applyDenoise(cv::Mat& mat);
#endif

    /**
     * @brief Transform callback (GStreamer)
     * @param trans Transform element
     * @param inbuf Input buffer
     * @param outbuf Output buffer
     * @return Flow status
     */
    static GstFlowReturn transformCallback(GstBaseTransform* trans,
                                         GstBuffer* inbuf,
                                         GstBuffer* outbuf);

    /**
     * @brief Caps setting callback
     * @param trans Transform element
     * @param incaps Input caps
     * @param outcaps Output caps
     * @return TRUE if successful
     */
    static gboolean setCapsCallback(GstBaseTransform* trans,
                                  GstCaps* incaps,
                                  GstCaps* outcaps);

    // Member variables
    GstElement* element_ = nullptr;                    // GStreamer element
    ProcessingParams params_;                          // Processing parameters
    ProcessingStats stats_;                            // Statistics
    mutable std::mutex params_mutex_;                  // Parameter access mutex
    mutable std::mutex stats_mutex_;                   // Statistics access mutex

    ProcessingCallback custom_processor_;              // Custom processing callback
    bool gpu_enabled_ = false;                         // GPU acceleration state

    GstVideoInfo video_info_;                          // Video format information
    bool video_info_valid_ = false;                    // Video info valid flag

    // For snapshots
    std::mutex snapshot_mutex_;                        // Snapshot mutex
    bool take_snapshot_ = false;                       // Snapshot flag
    std::string snapshot_filename_;                    // Snapshot file name

    // Metadata storage
    std::map<std::string, std::string> metadata_;      // Metadata
    std::mutex metadata_mutex_;                        // Metadata mutex

    // For performance measurement
    GstClockTime last_timestamp_ = 0;                  // Last timestamp
    guint64 processing_time_sum_ = 0;                  // Total processing time
};

#endif // VIDEO_PROCESSOR_H
