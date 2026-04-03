/**
 * @file motion_detector.cpp
 * @brief Motion detection class implementation
 */

#include "motion_detector.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// Virtual table for GStreamer base transform
typedef struct {
    GstBaseTransformClass parent_class;
} MotionDetectorClass;

typedef struct {
    GstBaseTransform parent;
    MotionDetector* detector;
} MotionDetectorElement;

// GObject type definitions
#define MOTION_DETECTOR_TYPE (motion_detector_get_type())
#define MOTION_DETECTOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), MOTION_DETECTOR_TYPE, MotionDetectorElement))

G_DEFINE_TYPE(MotionDetectorElement, motion_detector, GST_TYPE_BASE_TRANSFORM)

// Forward declarations
static GstFlowReturn motion_detector_transform(GstBaseTransform* trans, GstBuffer* inbuf, GstBuffer* outbuf);
static gboolean motion_detector_set_caps(GstBaseTransform* trans, GstCaps* incaps, GstCaps* outcaps);

/**
 * @brief GObject class initialization
 */
static void motion_detector_class_init(MotionDetectorClass* klass) {
    GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    
    base_transform_class->transform = GST_DEBUG_FUNCPTR(motion_detector_transform);
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(motion_detector_set_caps);
    
    // Use in-place transform
    base_transform_class->transform_ip_on_passthrough = FALSE;
}

/**
 * @brief GObject instance initialization
 */
static void motion_detector_init(MotionDetectorElement* element) {
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(element), TRUE);
}

/**
 * @brief Constructor
 */
MotionDetector::MotionDetector() {
    // Reset statistics
    resetStats();

    // Initialize last alert time
    last_alert_time_ = std::chrono::steady_clock::now();
    
#ifdef HAVE_OPENCV
    // Create background subtractor
    if (params_.algorithm == MotionAlgorithm::MOG2) {
        bg_subtractor_ = cv::createBackgroundSubtractorMOG2(
            params_.history_length, 
            16.0, 
            params_.detect_shadows
        );
    } else if (params_.algorithm == MotionAlgorithm::KNN) {
        bg_subtractor_ = cv::createBackgroundSubtractorKNN(
            params_.history_length,
            400.0,
            params_.detect_shadows
        );
    }
    
    std::cout << "[MotionDetector] OpenCV motion detection enabled." << std::endl;
#endif
}

/**
 * @brief Destructor
 */
MotionDetector::~MotionDetector() {
    if (element_) {
        gst_object_unref(element_);
    }
}

/**
 * @brief Creates a GStreamer element
 */
GstElement* MotionDetector::createElement() {
    element_ = g_object_new(MOTION_DETECTOR_TYPE, nullptr);
    MOTION_DETECTOR(element_)->detector = this;
    
    return GST_ELEMENT(element_);
}

/**
 * @brief Sets detection parameters
 */
void MotionDetector::setParameters(const MotionDetectionParams& params) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    params_ = params;
    
#ifdef HAVE_OPENCV
    // Create a new background subtractor if algorithm changed
    if (params_.algorithm == MotionAlgorithm::MOG2) {
        bg_subtractor_ = cv::createBackgroundSubtractorMOG2(
            params_.history_length, 
            16.0, 
            params_.detect_shadows
        );
    } else if (params_.algorithm == MotionAlgorithm::KNN) {
        bg_subtractor_ = cv::createBackgroundSubtractorKNN(
            params_.history_length,
            400.0,
            params_.detect_shadows
        );
    }
#endif
}

/**
 * @brief Returns current parameters
 */
MotionDetectionParams MotionDetector::getParameters() const {
    std::lock_guard<std::mutex> lock(params_mutex_);
    return params_;
}

/**
 * @brief Changes the motion detection algorithm
 */
void MotionDetector::setAlgorithm(MotionAlgorithm algorithm) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    params_.algorithm = algorithm;
    
#ifdef HAVE_OPENCV
    // Reset background model for new algorithm
    previous_frame_ = cv::Mat();
    background_model_ = cv::Mat();
    
    if (algorithm == MotionAlgorithm::MOG2) {
        bg_subtractor_ = cv::createBackgroundSubtractorMOG2();
    } else if (algorithm == MotionAlgorithm::KNN) {
        bg_subtractor_ = cv::createBackgroundSubtractorKNN();
    }
#endif
}

/**
 * @brief Sets sensitivity level
 */
void MotionDetector::setSensitivity(double sensitivity) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    params_.sensitivity = std::clamp(sensitivity, 0.0, 1.0);
    
    // Adjust threshold values based on sensitivity
    params_.threshold = 25.0 * (1.0 - params_.sensitivity) + 5.0;
    params_.min_area = (int)(1000 * (1.0 - params_.sensitivity) + 100);
}

/**
 * @brief Sets the motion event callback
 */
void MotionDetector::setMotionEventCallback(MotionEventCallback callback) {
    motion_callback_ = callback;
}

/**
 * @brief Returns statistics
 */
MotionStats MotionDetector::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

/**
 * @brief Resets statistics
 */
void MotionDetector::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = MotionStats();
    stats_.last_motion_time = std::chrono::steady_clock::now();
}

/**
 * @brief Returns current motion regions
 */
std::vector<MotionRegion> MotionDetector::getCurrentMotions() const {
    std::lock_guard<std::mutex> lock(motions_mutex_);
    return current_motions_;
}

/**
 * @brief Enables/disables motion detection
 */
void MotionDetector::setEnabled(bool enable) {
    enabled_ = enable;
}

/**
 * @brief Sets the region of interest
 */
void MotionDetector::setROI(int x, int y, int width, int height) {
    roi_ = cv::Rect(x, y, width, height);
    has_roi_ = true;
}

/**
 * @brief Clears the region of interest
 */
void MotionDetector::clearROI() {
    has_roi_ = false;
}

/**
 * @brief Adds an exclusion zone
 */
void MotionDetector::addExclusionZone(int x, int y, int width, int height) {
    exclusion_zones_.push_back(cv::Rect(x, y, width, height));
}

/**
 * @brief Clears all exclusion zones
 */
void MotionDetector::clearExclusionZones() {
    exclusion_zones_.clear();
}

#ifdef HAVE_OPENCV
/**
 * @brief Returns the motion mask
 */
cv::Mat MotionDetector::getMotionMask() const {
    return motion_mask_.clone();
}

/**
 * @brief Returns the background model
 */
cv::Mat MotionDetector::getBackgroundModel() const {
    if (bg_subtractor_) {
        cv::Mat background;
        bg_subtractor_->getBackgroundImage(background);
        return background;
    }
    return background_model_.clone();
}
#endif

/**
 * @brief Processes a video frame
 */
GstBuffer* MotionDetector::processFrame(GstBuffer* buffer, const GstVideoInfo* info) {
    if (!enabled_) {
        return buffer;
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_frames++;
    }

#ifdef HAVE_OPENCV
    // Convert buffer to OpenCV Mat
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READWRITE)) {
        return buffer;
    }
    
    int width = GST_VIDEO_INFO_WIDTH(info);
    int height = GST_VIDEO_INFO_HEIGHT(info);
    
    cv::Mat frame;
    
    // Convert based on video format
    switch (GST_VIDEO_INFO_FORMAT(info)) {
        case GST_VIDEO_FORMAT_I420:
            {
                cv::Mat yuv(height + height/2, width, CV_8UC1, map.data);
                cv::cvtColor(yuv, frame, cv::COLOR_YUV2BGR_I420);
            }
            break;
            
        case GST_VIDEO_FORMAT_RGB:
            frame = cv::Mat(height, width, CV_8UC3, map.data).clone();
            cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
            break;
            
        case GST_VIDEO_FORMAT_BGR:
            frame = cv::Mat(height, width, CV_8UC3, map.data).clone();
            break;
            
        default:
            gst_buffer_unmap(buffer, &map);
            return buffer;
    }
    
    // Detect motion
    std::vector<MotionRegion> regions = detectMotion(frame);
    
    // Store motion regions
    {
        std::lock_guard<std::mutex> lock(motions_mutex_);
        current_motions_ = regions;
    }
    
    // Update motion history
    motion_history_.push_back(regions);
    if (motion_history_.size() > max_history_size_) {
        motion_history_.pop_front();
    }
    
    // Update statistics
    if (!regions.empty()) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.motion_frames++;
        stats_.last_motion_time = std::chrono::steady_clock::now();
        stats_.total_regions += regions.size();
        
        // Calculate motion area
        double total_area = 0;
        for (const auto& region : regions) {
            total_area += region.width * region.height;
        }
        double motion_percentage = (total_area / (width * height)) * 100.0;
        
        stats_.average_motion_area = 
            (stats_.average_motion_area * (stats_.motion_frames - 1) + motion_percentage) 
            / stats_.motion_frames;
        
        stats_.max_motion_area = std::max(stats_.max_motion_area, motion_percentage);
        
        // Trigger motion event
        if (motion_percentage > params_.alert_threshold) {
            triggerMotionEvent(regions, GST_BUFFER_PTS(buffer));
        }
    }
    
    // Visualization
    if (params_.draw_motion_regions || params_.show_debug_view) {
        drawMotionRegions(frame, regions);
        
        // Debug view
        if (params_.show_debug_view && !motion_mask_.empty()) {
            cv::Mat debug_view;
            cv::cvtColor(motion_mask_, debug_view, cv::COLOR_GRAY2BGR);
            
            // Split screen: left side original, right side motion mask
            cv::Mat combined(height, width * 2, CV_8UC3);
            frame.copyTo(combined(cv::Rect(0, 0, width, height)));
            debug_view.copyTo(combined(cv::Rect(width, 0, width, height)));
            frame = combined;
        }
    }
    
    // Write frame back to buffer
    switch (GST_VIDEO_INFO_FORMAT(info)) {
        case GST_VIDEO_FORMAT_I420:
            {
                cv::Mat yuv;
                cv::cvtColor(frame, yuv, cv::COLOR_BGR2YUV_I420);
                memcpy(map.data, yuv.data, map.size);
            }
            break;
            
        case GST_VIDEO_FORMAT_RGB:
            {
                cv::Mat rgb;
                cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
                memcpy(map.data, rgb.data, map.size);
            }
            break;
            
        case GST_VIDEO_FORMAT_BGR:
            memcpy(map.data, frame.data, map.size);
            break;
            
        default:
            break;
    }
    
    gst_buffer_unmap(buffer, &map);
#endif // HAVE_OPENCV
    
    return buffer;
}

#ifdef HAVE_OPENCV
/**
 * @brief Detect motion with OpenCV
 */
std::vector<MotionRegion> MotionDetector::detectMotion(const cv::Mat& frame) {
    cv::Mat mask;
    
    // Algorithm selection
    switch (params_.algorithm) {
        case MotionAlgorithm::FRAME_DIFF:
            mask = frameDifference(frame);
            break;
            
        case MotionAlgorithm::BACKGROUND_SUB:
        case MotionAlgorithm::MOG2:
        case MotionAlgorithm::KNN:
            mask = backgroundSubtraction(frame);
            break;
            
        case MotionAlgorithm::OPTICAL_FLOW:
            // TODO: Optical flow implementation
            mask = cv::Mat::zeros(frame.size(), CV_8UC1);
            break;
            
        default:
            mask = cv::Mat::zeros(frame.size(), CV_8UC1);
            break;
    }
    
    // Apply ROI if set
    if (has_roi_) {
        cv::Mat roi_mask = cv::Mat::zeros(mask.size(), CV_8UC1);
        roi_mask(roi_) = 255;
        cv::bitwise_and(mask, roi_mask, mask);
    }
    
    // Apply exclusion zones
    for (const auto& zone : exclusion_zones_) {
        mask(zone) = 0;
    }
    
    // Morphological operations
    applyMorphology(mask);
    
    // Store motion mask
    motion_mask_ = mask.clone();
    
    // Find motion regions
    std::vector<MotionRegion> regions = findMotionRegions(mask);
    
    // Filter regions
    return filterRegions(regions);
}

/**
 * @brief Frame difference algorithm
 */
cv::Mat MotionDetector::frameDifference(const cv::Mat& frame) {
    cv::Mat gray, diff, mask;
    
    // Convert to grayscale
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    
    // Store if first frame
    if (previous_frame_.empty()) {
        previous_frame_ = gray.clone();
        return cv::Mat::zeros(frame.size(), CV_8UC1);
    }
    
    // Calculate frame difference
    cv::absdiff(previous_frame_, gray, diff);
    
    // Thresholding
    cv::threshold(diff, mask, params_.threshold, 255, cv::THRESH_BINARY);
    
    // Update previous frame
    previous_frame_ = gray.clone();
    
    return mask;
}

/**
 * @brief Background subtraction algorithm
 */
cv::Mat MotionDetector::backgroundSubtraction(const cv::Mat& frame) {
    cv::Mat mask;
    
    if (bg_subtractor_) {
        // Use OpenCV background subtractor
        bg_subtractor_->apply(frame, mask, params_.learning_rate);
        
        // Remove shadows (if enabled)
        if (params_.detect_shadows) {
            cv::threshold(mask, mask, 128, 255, cv::THRESH_BINARY);
        }
    } else {
        // Simple background model
        if (background_model_.empty()) {
            background_model_ = frame.clone();
            return cv::Mat::zeros(frame.size(), CV_8UC1);
        }
        
        cv::Mat diff;
        cv::absdiff(frame, background_model_, diff);
        cv::cvtColor(diff, diff, cv::COLOR_BGR2GRAY);
        cv::threshold(diff, mask, params_.threshold, 255, cv::THRESH_BINARY);
        
        // Update background model
        cv::addWeighted(frame, params_.learning_rate, background_model_, 
                       1.0 - params_.learning_rate, 0, background_model_);
    }
    
    return mask;
}

/**
 * @brief Apply morphological operations
 */
void MotionDetector::applyMorphology(cv::Mat& mask) {
    // Erosion (remove small noise)
    if (params_.erosion_size > 0) {
        cv::Mat erosion_kernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE,
            cv::Size(params_.erosion_size * 2 + 1, params_.erosion_size * 2 + 1)
        );
        cv::erode(mask, mask, erosion_kernel);
    }
    
    // Dilation (fill gaps)
    if (params_.dilation_size > 0) {
        cv::Mat dilation_kernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE,
            cv::Size(params_.dilation_size * 2 + 1, params_.dilation_size * 2 + 1)
        );
        cv::dilate(mask, mask, dilation_kernel);
    }
    
    // Opening operation - remove small objects
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    
    // Closing operation - fill small holes
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
}

/**
 * @brief Find motion regions
 */
std::vector<MotionRegion> MotionDetector::findMotionRegions(const cv::Mat& mask) {
    std::vector<MotionRegion> regions;
    std::vector<std::vector<cv::Point>> contours;
    
    // Find contours
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Create a region for each contour
    for (const auto& contour : contours) {
        cv::Rect bbox = cv::boundingRect(contour);
        double area = cv::contourArea(contour);
        
        MotionRegion region;
        region.x = bbox.x;
        region.y = bbox.y;
        region.width = bbox.width;
        region.height = bbox.height;
        region.intensity = std::min(1.0, area / (params_.max_area / 2.0));
        region.timestamp = 0; // Frame timestamp will be added later
        
        regions.push_back(region);
    }
    
    return regions;
}

/**
 * @brief Filter motion regions
 */
std::vector<MotionRegion> MotionDetector::filterRegions(const std::vector<MotionRegion>& regions) {
    std::vector<MotionRegion> filtered;
    
    for (const auto& region : regions) {
        // Area filter
        double area = region.width * region.height;
        if (area < params_.min_area || area > params_.max_area) {
            continue;
        }
        
        // Aspect ratio filter
        double aspect_ratio = (double)region.width / region.height;
        if (aspect_ratio < params_.min_aspect_ratio || 
            aspect_ratio > params_.max_aspect_ratio) {
            continue;
        }
        
        filtered.push_back(region);
    }
    
    // Tracking limit
    if (params_.enable_tracking && filtered.size() > params_.max_tracked_objects) {
        // Keep the largest regions
        std::sort(filtered.begin(), filtered.end(), 
                  [](const MotionRegion& a, const MotionRegion& b) {
                      return (a.width * a.height) > (b.width * b.height);
                  });
        filtered.resize(params_.max_tracked_objects);
    }
    
    return filtered;
}

/**
 * @brief Draw motion regions
 */
void MotionDetector::drawMotionRegions(cv::Mat& frame, const std::vector<MotionRegion>& regions) {
    for (const auto& region : regions) {
        // Determine color based on intensity
        cv::Scalar color;
        if (region.intensity > 0.7) {
            color = cv::Scalar(0, 0, 255); // Red - high motion
        } else if (region.intensity > 0.4) {
            color = cv::Scalar(0, 165, 255); // Orange - medium motion
        } else {
            color = cv::Scalar(0, 255, 0); // Green - low motion
        }
        
        // Draw rectangle
        cv::rectangle(frame, 
                     cv::Rect(region.x, region.y, region.width, region.height),
                     color, 2);
        
        // Info text
        std::string info = "Motion: " + 
                          std::to_string(static_cast<int>(region.intensity * 100)) + "%";
        cv::putText(frame, info, 
                   cv::Point(region.x, region.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
    }
    
    // General information
    if (!regions.empty()) {
        std::string status = "MOTION DETECTED - " + std::to_string(regions.size()) + " regions";
        cv::putText(frame, status,
                   cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
    }
}
#endif // HAVE_OPENCV

/**
 * @brief Transform callback
 */
static GstFlowReturn motion_detector_transform(GstBaseTransform* trans,
                                             GstBuffer* inbuf,
                                             GstBuffer* outbuf) {
    MotionDetectorElement* element = MOTION_DETECTOR(trans);
    MotionDetector* detector = element->detector;
    
    if (!detector) {
        return GST_FLOW_ERROR;
    }
    
    // Get video info
    GstVideoInfo info;
    GstCaps* caps = gst_pad_get_current_caps(GST_BASE_TRANSFORM_SINK_PAD(trans));
    if (!gst_video_info_from_caps(&info, caps)) {
        gst_caps_unref(caps);
        return GST_FLOW_ERROR;
    }
    gst_caps_unref(caps);
    
    // Process frame (in-place)
    detector->processFrame(inbuf, &info);
    
    return GST_FLOW_OK;
}

/**
 * @brief Set caps callback
 */
static gboolean motion_detector_set_caps(GstBaseTransform* trans,
                                       GstCaps* incaps,
                                       GstCaps* outcaps) {
    return TRUE;
}

/**
 * @brief Trigger motion event
 */
void MotionDetector::triggerMotionEvent(const std::vector<MotionRegion>& regions, 
                                       guint64 timestamp) {
    if (!motion_callback_ || !params_.enable_alerts) {
        return;
    }
    
    // Cooldown check
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_alert_time_).count();
    
    if (elapsed < params_.alert_cooldown_ms) {
        return;
    }
    
    // Calculate total motion area
    double total_area = 0;
    for (const auto& region : regions) {
        total_area += region.width * region.height;
    }
    
    // Calculate as percentage (relative to frame size)
    double motion_percentage = 0;
    if (element_) {
        GstCaps* caps = gst_pad_get_current_caps(
            GST_BASE_TRANSFORM_SINK_PAD(GST_BASE_TRANSFORM(element_)));
        if (caps) {
            GstVideoInfo info;
            if (gst_video_info_from_caps(&info, caps)) {
                int frame_area = GST_VIDEO_INFO_WIDTH(&info) * GST_VIDEO_INFO_HEIGHT(&info);
                motion_percentage = (total_area / frame_area) * 100.0;
            }
            gst_caps_unref(caps);
        }
    }
    
    // Call the callback
    motion_callback_(regions, timestamp, motion_percentage);
    
    // Update last alert time
    last_alert_time_ = now;
}