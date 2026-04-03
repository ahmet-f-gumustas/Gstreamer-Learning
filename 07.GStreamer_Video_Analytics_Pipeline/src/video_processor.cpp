/**
 * @file video_processor.cpp
 * @brief Video processing class implementation
 */

#include "video_processor.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <chrono>

// Virtual table for GStreamer base transform
typedef struct {
    GstBaseTransformClass parent_class;
} VideoProcessorClass;

typedef struct {
    GstBaseTransform parent;
    VideoProcessor* processor;
} VideoProcessorElement;

// GObject type definitions
#define VIDEO_PROCESSOR_TYPE (video_processor_get_type())
#define VIDEO_PROCESSOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), VIDEO_PROCESSOR_TYPE, VideoProcessorElement))

G_DEFINE_TYPE(VideoProcessorElement, video_processor, GST_TYPE_BASE_TRANSFORM)

// Forward declarations
static GstFlowReturn video_processor_transform(GstBaseTransform* trans, GstBuffer* inbuf, GstBuffer* outbuf);
static gboolean video_processor_set_caps(GstBaseTransform* trans, GstCaps* incaps, GstCaps* outcaps);
static GstCaps* video_processor_transform_caps(GstBaseTransform* trans, GstPadDirection direction, GstCaps* caps, GstCaps* filter);

/**
 * @brief GObject class initialization
 */
static void video_processor_class_init(VideoProcessorClass* klass) {
    GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    // Set transform functions
    base_transform_class->transform = GST_DEBUG_FUNCPTR(video_processor_transform);
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(video_processor_set_caps);
    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(video_processor_transform_caps);
    
    // In-place transform disabled (copy required)
    base_transform_class->transform_ip_on_passthrough = FALSE;
}

/**
 * @brief GObject instance initialization
 */
static void video_processor_init(VideoProcessorElement* element) {
    // Passthrough active by default
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(element), TRUE);
}

/**
 * @brief Constructor
 */
VideoProcessor::VideoProcessor() {
    // Reset statistics
    resetStats();

#ifdef HAVE_OPENCV
    std::cout << "[VideoProcessor] OpenCV support enabled." << std::endl;
#endif
}

/**
 * @brief Destructor
 */
VideoProcessor::~VideoProcessor() {
    if (element_) {
        gst_object_unref(element_);
    }
}

/**
 * @brief Creates a GStreamer element
 */
GstElement* VideoProcessor::createElement() {
    // Create custom element
    element_ = g_object_new(VIDEO_PROCESSOR_TYPE, nullptr);
    
    // Store VideoProcessor pointer
    VIDEO_PROCESSOR(element_)->processor = this;
    
    // Set element properties
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(element_), FALSE);
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(element_), 
                                      params_.filter_type == FilterType::NONE);
    
    return GST_ELEMENT(element_);
}

/**
 * @brief Sets processing parameters
 */
void VideoProcessor::setParameters(const ProcessingParams& params) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    params_ = params;
    
    // Update passthrough mode
    if (element_) {
        gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(element_), 
                                          params_.filter_type == FilterType::NONE);
    }
}

/**
 * @brief Returns current parameters
 */
ProcessingParams VideoProcessor::getParameters() const {
    std::lock_guard<std::mutex> lock(params_mutex_);
    return params_;
}

/**
 * @brief Changes the filter type
 */
void VideoProcessor::setFilter(FilterType type) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    params_.filter_type = type;
    
    if (element_) {
        gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(element_), 
                                          type == FilterType::NONE);
    }
}

/**
 * @brief Sets the custom processing callback
 */
void VideoProcessor::setCustomProcessor(ProcessingCallback callback) {
    custom_processor_ = callback;
}

/**
 * @brief Returns statistics
 */
ProcessingStats VideoProcessor::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

/**
 * @brief Resets statistics
 */
void VideoProcessor::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = ProcessingStats();
}

/**
 * @brief Sets GPU acceleration
 */
void VideoProcessor::setGPUAcceleration(bool enable) {
    gpu_enabled_ = enable;
    if (enable) {
        std::cout << "[VideoProcessor] GPU acceleration enabled." << std::endl;
    }
}

/**
 * @brief Takes a snapshot
 */
bool VideoProcessor::takeSnapshot(const std::string& filename) {
    std::lock_guard<std::mutex> lock(snapshot_mutex_);
    take_snapshot_ = true;
    snapshot_filename_ = filename;
    return true;
}

/**
 * @brief Adds metadata
 */
void VideoProcessor::addMetadata(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(metadata_mutex_);
    metadata_[key] = value;
}

/**
 * @brief Processes a video frame
 */
GstBuffer* VideoProcessor::processFrame(GstBuffer* buffer, const GstVideoInfo* info) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.frames_processed++;
    }
    
    // Use custom processor if available
    if (custom_processor_ && params_.filter_type == FilterType::CUSTOM) {
        return custom_processor_(buffer, info->width, info->height);
    }
    
    // Map the buffer
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READWRITE)) {
        return buffer;
    }
    
#ifdef HAVE_OPENCV
    // OpenCV processing
    if (params_.filter_type != FilterType::NONE) {
        cv::Mat mat = bufferToMat(buffer, info);
        
        // Apply filter
        applyOpenCVFilter(mat, params_.filter_type);
        
        // Take snapshot
        if (take_snapshot_) {
            cv::imwrite(snapshot_filename_, mat);
            take_snapshot_ = false;
            std::cout << "[VideoProcessor] Snapshot saved: " 
                      << snapshot_filename_ << std::endl;
        }
        
        // Convert Mat back to buffer
        gst_buffer_unmap(buffer, &map);
        return matToBuffer(mat, info);
    }
#else
    // Simple CPU processing
    switch (params_.filter_type) {
        case FilterType::GRAYSCALE:
            applyGrayscale(map.data, info);
            break;
            
        case FilterType::BRIGHTNESS:
        case FilterType::CONTRAST:
            adjustBrightnessContrast(map.data, info);
            break;
            
        default:
            break;
    }
#endif
    
    gst_buffer_unmap(buffer, &map);
    
    // Calculate processing time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double processing_time = duration.count() / 1000.0; // ms
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        processing_time_sum_ += processing_time;
        stats_.avg_processing_time = processing_time_sum_ / stats_.frames_processed;
        stats_.min_processing_time = std::min(stats_.min_processing_time, processing_time);
        stats_.max_processing_time = std::max(stats_.max_processing_time, processing_time);
    }
    
    return buffer;
}

/**
 * @brief Applies grayscale filter (CPU)
 */
void VideoProcessor::applyGrayscale(guint8* data, const GstVideoInfo* info) {
    int width = GST_VIDEO_INFO_WIDTH(info);
    int height = GST_VIDEO_INFO_HEIGHT(info);
    int stride = GST_VIDEO_INFO_PLANE_STRIDE(info, 0);
    
    // For I420 format, Y plane is already grayscale
    // Set U and V planes to 128 (neutral color)
    if (GST_VIDEO_INFO_FORMAT(info) == GST_VIDEO_FORMAT_I420) {
        int y_size = stride * height;
        int uv_stride = GST_VIDEO_INFO_PLANE_STRIDE(info, 1);
        int uv_height = height / 2;
        
        // U plane
        guint8* u_plane = data + y_size;
        for (int i = 0; i < uv_height; i++) {
            memset(u_plane + i * uv_stride, 128, width / 2);
        }
        
        // V plane
        guint8* v_plane = u_plane + (uv_stride * uv_height);
        for (int i = 0; i < uv_height; i++) {
            memset(v_plane + i * uv_stride, 128, width / 2);
        }
    }
}

/**
 * @brief Adjusts brightness/contrast (CPU)
 */
void VideoProcessor::adjustBrightnessContrast(guint8* data, const GstVideoInfo* info) {
    int width = GST_VIDEO_INFO_WIDTH(info);
    int height = GST_VIDEO_INFO_HEIGHT(info);
    int stride = GST_VIDEO_INFO_PLANE_STRIDE(info, 0);
    
    double brightness = params_.brightness;
    double contrast = params_.contrast;
    
    // Process Y plane (brightness)
    for (int y = 0; y < height; y++) {
        guint8* row = data + y * stride;
        for (int x = 0; x < width; x++) {
            int pixel = row[x];
            
            // Apply contrast
            pixel = (int)((pixel - 128) * contrast + 128);
            
            // Add brightness
            pixel += brightness;
            
            // Check bounds
            row[x] = (guint8)CLAMP(pixel, 0, 255);
        }
    }
}

#ifdef HAVE_OPENCV
/**
 * @brief Converts buffer to OpenCV Mat
 */
cv::Mat VideoProcessor::bufferToMat(GstBuffer* buffer, const GstVideoInfo* info) {
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    
    cv::Mat mat;
    int width = GST_VIDEO_INFO_WIDTH(info);
    int height = GST_VIDEO_INFO_HEIGHT(info);
    
    switch (GST_VIDEO_INFO_FORMAT(info)) {
        case GST_VIDEO_FORMAT_I420:
            {
                // Convert I420 to BGR
                cv::Mat yuv(height + height/2, width, CV_8UC1, map.data);
                cv::cvtColor(yuv, mat, cv::COLOR_YUV2BGR_I420);
            }
            break;
            
        case GST_VIDEO_FORMAT_RGB:
            mat = cv::Mat(height, width, CV_8UC3, map.data).clone();
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
            break;
            
        case GST_VIDEO_FORMAT_BGR:
            mat = cv::Mat(height, width, CV_8UC3, map.data).clone();
            break;
            
        default:
            std::cerr << "[VideoProcessor] Unsupported video format!" << std::endl;
            mat = cv::Mat(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
            break;
    }
    
    gst_buffer_unmap(buffer, &map);
    return mat;
}

/**
 * @brief Converts OpenCV Mat to buffer
 */
GstBuffer* VideoProcessor::matToBuffer(const cv::Mat& mat, const GstVideoInfo* info) {
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, 
                                               GST_VIDEO_INFO_SIZE(info), nullptr);
    
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    
    int width = GST_VIDEO_INFO_WIDTH(info);
    int height = GST_VIDEO_INFO_HEIGHT(info);
    
    switch (GST_VIDEO_INFO_FORMAT(info)) {
        case GST_VIDEO_FORMAT_I420:
            {
                // Convert BGR to I420
                cv::Mat yuv;
                cv::cvtColor(mat, yuv, cv::COLOR_BGR2YUV_I420);
                memcpy(map.data, yuv.data, GST_VIDEO_INFO_SIZE(info));
            }
            break;
            
        case GST_VIDEO_FORMAT_RGB:
            {
                cv::Mat rgb;
                cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
                memcpy(map.data, rgb.data, width * height * 3);
            }
            break;
            
        case GST_VIDEO_FORMAT_BGR:
            memcpy(map.data, mat.data, width * height * 3);
            break;
            
        default:
            break;
    }
    
    gst_buffer_unmap(buffer, &map);
    return buffer;
}

/**
 * @brief Applies OpenCV-based filter
 */
void VideoProcessor::applyOpenCVFilter(cv::Mat& mat, FilterType type) {
    switch (type) {
        case FilterType::GRAYSCALE:
            cv::cvtColor(mat, mat, cv::COLOR_BGR2GRAY);
            cv::cvtColor(mat, mat, cv::COLOR_GRAY2BGR);
            break;
            
        case FilterType::BLUR:
            applyBlur(mat);
            break;
            
        case FilterType::SHARPEN:
            applySharpen(mat);
            break;
            
        case FilterType::EDGE_DETECT:
            applyEdgeDetection(mat);
            break;
            
        case FilterType::DENOISE:
            applyDenoise(mat);
            break;
            
        case FilterType::BRIGHTNESS:
        case FilterType::CONTRAST:
            mat.convertTo(mat, -1, params_.contrast, params_.brightness);
            break;
            
        default:
            break;
    }
    
    // Apply rotation
    if (params_.rotation != 0) {
        cv::Point2f center(mat.cols / 2.0, mat.rows / 2.0);
        cv::Mat rot_mat = cv::getRotationMatrix2D(center, params_.rotation, 1.0);
        cv::warpAffine(mat, mat, rot_mat, mat.size());
    }
    
    // Apply flip
    if (params_.flip_horizontal && params_.flip_vertical) {
        cv::flip(mat, mat, -1);
    } else if (params_.flip_horizontal) {
        cv::flip(mat, mat, 1);
    } else if (params_.flip_vertical) {
        cv::flip(mat, mat, 0);
    }
}

/**
 * @brief Blur filter
 */
void VideoProcessor::applyBlur(cv::Mat& mat) {
    int kernel_size = params_.blur_kernel_size;
    if (kernel_size % 2 == 0) kernel_size++; // Must be odd
    
    cv::GaussianBlur(mat, mat, cv::Size(kernel_size, kernel_size), 0);
}

/**
 * @brief Sharpening filter
 */
void VideoProcessor::applySharpen(cv::Mat& mat) {
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 
        0, -1, 0,
        -1, 5 + params_.sharpen_strength, -1,
        0, -1, 0);
    
    cv::filter2D(mat, mat, -1, kernel);
}

/**
 * @brief Edge detection filter
 */
void VideoProcessor::applyEdgeDetection(cv::Mat& mat) {
    cv::Mat gray, edges;
    cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);
    
    cv::Canny(gray, edges, params_.edge_threshold1, params_.edge_threshold2);
    
    // Show edges in color
    mat.setTo(cv::Scalar(0, 0, 0));
    mat.setTo(cv::Scalar(0, 255, 0), edges);
}

/**
 * @brief Noise reduction filter
 */
void VideoProcessor::applyDenoise(cv::Mat& mat) {
    cv::fastNlMeansDenoisingColored(mat, mat, params_.denoise_strength, 
                                    params_.denoise_strength, 7, 21);
}
#endif // HAVE_OPENCV

/**
 * @brief GStreamer transform callback
 */
static GstFlowReturn video_processor_transform(GstBaseTransform* trans, 
                                             GstBuffer* inbuf, 
                                             GstBuffer* outbuf) {
    VideoProcessorElement* element = VIDEO_PROCESSOR(trans);
    VideoProcessor* processor = element->processor;
    
    if (!processor) {
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
    
    // Process frame
    GstBuffer* result = processor->processFrame(inbuf, &info);
    
    // Copy result
    if (result != inbuf) {
        gst_buffer_copy_into(outbuf, result, GST_BUFFER_COPY_ALL, 0, -1);
        gst_buffer_unref(result);
    } else {
        gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_ALL, 0, -1);
    }
    
    return GST_FLOW_OK;
}

/**
 * @brief Caps setting callback
 */
static gboolean video_processor_set_caps(GstBaseTransform* trans,
                                       GstCaps* incaps,
                                       GstCaps* outcaps) {
    VideoProcessorElement* element = VIDEO_PROCESSOR(trans);
    VideoProcessor* processor = element->processor;
    
    if (!processor) {
        return FALSE;
    }
    
    // Store video info
    GstVideoInfo info;
    if (gst_video_info_from_caps(&info, incaps)) {
        processor->video_info_ = info;
        processor->video_info_valid_ = true;
    }
    
    return TRUE;
}

/**
 * @brief Transform caps callback
 */
static GstCaps* video_processor_transform_caps(GstBaseTransform* trans,
                                             GstPadDirection direction,
                                             GstCaps* caps,
                                             GstCaps* filter) {
    // Return the same caps (we don't change the format)
    GstCaps* result = gst_caps_copy(caps);
    
    if (filter) {
        GstCaps* tmp = gst_caps_intersect_full(result, filter, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(result);
        result = tmp;
    }
    
    return result;
}

/**
 * @brief Transform callback wrapper
 */
GstFlowReturn VideoProcessor::transformCallback(GstBaseTransform* trans,
                                              GstBuffer* inbuf,
                                              GstBuffer* outbuf) {
    return video_processor_transform(trans, inbuf, outbuf);
}

/**
 * @brief Set caps callback wrapper
 */
gboolean VideoProcessor::setCapsCallback(GstBaseTransform* trans,
                                       GstCaps* incaps,
                                       GstCaps* outcaps) {
    return video_processor_set_caps(trans, incaps, outcaps);
}