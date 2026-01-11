#include "frame_extractor.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

namespace gst_frame_extractor {

FrameExtractor::FrameExtractor()
    : pipeline_(nullptr)
    , appsink_(nullptr)
    , bus_(nullptr)
    , main_loop_(nullptr)
    , running_(false)
    , frame_count_(0)
    , extracted_count_(0)
    , duration_(0)
    , current_position_(0)
{
}

FrameExtractor::~FrameExtractor() {
    stop();

    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
    }

    if (bus_) {
        gst_object_unref(bus_);
    }

    if (main_loop_) {
        g_main_loop_unref(main_loop_);
    }
}

ExtractorConfig FrameExtractor::loadConfig(const std::string& config_path) {
    ExtractorConfig config;

    try {
        YAML::Node yaml = YAML::LoadFile(config_path);

        // Input/Output settings
        config.input_uri = yaml["input"]["uri"].as<std::string>("");
        config.output_dir = yaml["output"]["directory"].as<std::string>("./frames");
        config.output_prefix = yaml["output"]["prefix"].as<std::string>("frame");

        // Extraction mode
        std::string mode_str = yaml["extraction"]["mode"].as<std::string>("interval");
        if (mode_str == "interval") {
            config.mode = ExtractionMode::INTERVAL;
        } else if (mode_str == "keyframe") {
            config.mode = ExtractionMode::KEYFRAME;
        } else if (mode_str == "all") {
            config.mode = ExtractionMode::ALL_FRAMES;
        } else if (mode_str == "time_based") {
            config.mode = ExtractionMode::TIME_BASED;
        }

        config.interval_frames = yaml["extraction"]["interval_frames"].as<int>(30);
        config.interval_seconds = yaml["extraction"]["interval_seconds"].as<double>(1.0);
        config.max_frames = yaml["extraction"]["max_frames"].as<int>(0);

        // Timestamps for TIME_BASED mode
        if (yaml["extraction"]["timestamps"]) {
            for (const auto& ts : yaml["extraction"]["timestamps"]) {
                config.timestamps.push_back(ts.as<double>());
            }
        }

        // Output format
        std::string format_str = yaml["output"]["format"].as<std::string>("png");
        if (format_str == "png") {
            config.format = OutputFormat::PNG;
        } else if (format_str == "jpeg" || format_str == "jpg") {
            config.format = OutputFormat::JPEG;
        } else if (format_str == "bmp") {
            config.format = OutputFormat::BMP;
        }

        config.jpeg_quality = yaml["output"]["jpeg_quality"].as<int>(95);

        // Resize settings
        config.resize_output = yaml["output"]["resize"]["enabled"].as<bool>(false);
        config.output_width = yaml["output"]["resize"]["width"].as<int>(0);
        config.output_height = yaml["output"]["resize"]["height"].as<int>(0);

        // Overlay settings
        config.add_timestamp_overlay = yaml["output"]["timestamp_overlay"].as<bool>(false);

    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
    }

    return config;
}

bool FrameExtractor::initialize(const ExtractorConfig& config) {
    config_ = config;

    // Create output directory
    std::filesystem::create_directories(config_.output_dir);

    // Initialize GStreamer if not already done
    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
    }

    // Create main loop
    main_loop_ = g_main_loop_new(nullptr, FALSE);

    // Create pipeline
    if (!createPipeline()) {
        last_error_ = "Failed to create pipeline";
        return false;
    }

    // Setup appsink
    if (!setupAppsink()) {
        last_error_ = "Failed to setup appsink";
        return false;
    }

    // Setup bus
    bus_ = gst_element_get_bus(pipeline_);
    gst_bus_add_watch(bus_, onBusMessage, this);

    return true;
}

bool FrameExtractor::createPipeline() {
    std::string pipeline_str;

    // Determine source element based on URI
    if (config_.input_uri.find("rtsp://") == 0) {
        pipeline_str = "rtspsrc location=" + config_.input_uri + " ! decodebin";
    } else if (config_.input_uri.find("http://") == 0 || config_.input_uri.find("https://") == 0) {
        pipeline_str = "souphttpsrc location=" + config_.input_uri + " ! decodebin";
    } else {
        pipeline_str = "filesrc location=" + config_.input_uri + " ! decodebin";
    }

    // Add video conversion and appsink
    pipeline_str += " ! videoconvert ! video/x-raw,format=BGR ! appsink name=sink";

    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);

    if (error) {
        last_error_ = std::string("Pipeline error: ") + error->message;
        g_error_free(error);
        return false;
    }

    return pipeline_ != nullptr;
}

bool FrameExtractor::setupAppsink() {
    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
    if (!appsink_) {
        return false;
    }

    // Configure appsink
    GstAppSinkCallbacks callbacks = {nullptr, nullptr, onNewSample};
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink_), &callbacks, this, nullptr);

    g_object_set(appsink_,
        "emit-signals", TRUE,
        "sync", FALSE,
        "max-buffers", 1,
        "drop", TRUE,
        nullptr);

    return true;
}

bool FrameExtractor::start() {
    if (running_) {
        return false;
    }

    // Set pipeline to playing state
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        last_error_ = "Failed to start pipeline";
        return false;
    }

    running_ = true;

    // Query duration after starting
    queryDuration();

    // Start main loop in separate thread
    loop_thread_ = std::make_unique<std::thread>([this]() {
        g_main_loop_run(main_loop_);
    });

    std::cout << "Frame extraction started..." << std::endl;
    return true;
}

void FrameExtractor::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (main_loop_ && g_main_loop_is_running(main_loop_)) {
        g_main_loop_quit(main_loop_);
    }

    if (loop_thread_ && loop_thread_->joinable()) {
        loop_thread_->join();
    }

    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }

    std::cout << "Frame extraction stopped. Total frames extracted: "
              << extracted_count_ << std::endl;
}

void FrameExtractor::waitForCompletion() {
    if (loop_thread_ && loop_thread_->joinable()) {
        loop_thread_->join();
    }
}

GstFlowReturn FrameExtractor::onNewSample(GstAppSink* sink, gpointer user_data) {
    FrameExtractor* self = static_cast<FrameExtractor*>(user_data);

    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (sample) {
        self->processFrame(sample);
        gst_sample_unref(sample);
    }

    return GST_FLOW_OK;
}

gboolean FrameExtractor::onBusMessage(GstBus* bus, GstMessage* msg, gpointer user_data) {
    FrameExtractor* self = static_cast<FrameExtractor*>(user_data);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            std::cout << "End of stream reached." << std::endl;
            self->stop();
            break;

        case GST_MESSAGE_ERROR: {
            GError* error = nullptr;
            gchar* debug_info = nullptr;
            gst_message_parse_error(msg, &error, &debug_info);

            std::cerr << "Error: " << error->message << std::endl;
            if (debug_info) {
                std::cerr << "Debug info: " << debug_info << std::endl;
                g_free(debug_info);
            }

            self->last_error_ = error->message;
            g_error_free(error);
            self->stop();
            break;
        }

        case GST_MESSAGE_STATE_CHANGED:
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(self->pipeline_)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

                if (new_state == GST_STATE_PLAYING) {
                    self->queryDuration();
                }
            }
            break;

        default:
            break;
    }

    return TRUE;
}

void FrameExtractor::processFrame(GstSample* sample) {
    if (!running_) {
        return;
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* caps = gst_sample_get_caps(sample);

    if (!buffer || !caps) {
        return;
    }

    // Get video info
    GstVideoInfo video_info;
    if (!gst_video_info_from_caps(&video_info, caps)) {
        return;
    }

    int width = GST_VIDEO_INFO_WIDTH(&video_info);
    int height = GST_VIDEO_INFO_HEIGHT(&video_info);

    // Get PTS (presentation timestamp)
    int64_t pts = GST_BUFFER_PTS(buffer);
    current_position_ = pts;

    int current_frame = frame_count_++;

    // Check if we should extract this frame
    if (!shouldExtractFrame(current_frame, pts)) {
        return;
    }

    // Check max frames limit
    if (config_.max_frames > 0 && extracted_count_ >= config_.max_frames) {
        stop();
        return;
    }

    // Map buffer
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        return;
    }

    // Create OpenCV Mat from buffer
    cv::Mat frame(height, width, CV_8UC3, map.data);

    // Apply resize if configured
    cv::Mat output_frame;
    if (config_.resize_output && config_.output_width > 0 && config_.output_height > 0) {
        cv::resize(frame, output_frame, cv::Size(config_.output_width, config_.output_height));
    } else {
        output_frame = frame.clone();
    }

    // Add timestamp overlay if configured
    if (config_.add_timestamp_overlay) {
        std::string timestamp = formatTimestamp(pts);
        cv::putText(output_frame, timestamp, cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
        cv::putText(output_frame, timestamp, cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 1);
    }

    // Call frame callback if set
    if (frame_callback_) {
        frame_callback_(output_frame, pts, current_frame);
    }

    // Save frame
    if (saveFrame(output_frame, current_frame, pts)) {
        extracted_count_++;

        // Progress output
        if (extracted_count_ % 10 == 0 || extracted_count_ == 1) {
            double progress = 0.0;
            if (duration_ > 0) {
                progress = (static_cast<double>(pts) / duration_) * 100.0;
            }
            std::cout << "Extracted frame " << extracted_count_
                      << " (Frame #" << current_frame << ")"
                      << " - Progress: " << std::fixed << std::setprecision(1)
                      << progress << "%" << std::endl;
        }
    }

    gst_buffer_unmap(buffer, &map);
}

bool FrameExtractor::shouldExtractFrame(int frame_number, int64_t pts) {
    switch (config_.mode) {
        case ExtractionMode::ALL_FRAMES:
            return true;

        case ExtractionMode::INTERVAL:
            return (frame_number % config_.interval_frames) == 0;

        case ExtractionMode::TIME_BASED: {
            double current_time = static_cast<double>(pts) / GST_SECOND;

            // Check interval-based time extraction
            if (config_.interval_seconds > 0) {
                static double last_extracted_time = -config_.interval_seconds;
                if (current_time - last_extracted_time >= config_.interval_seconds) {
                    last_extracted_time = current_time;
                    return true;
                }
            }

            // Check specific timestamps
            for (double ts : config_.timestamps) {
                if (std::abs(current_time - ts) < 0.05) { // 50ms tolerance
                    return true;
                }
            }
            return false;
        }

        case ExtractionMode::KEYFRAME:
            // For keyframe mode, we'd need to check buffer flags
            // This is a simplified implementation
            return (frame_number % 30) == 0; // Approximate keyframe interval

        default:
            return false;
    }
}

bool FrameExtractor::saveFrame(const cv::Mat& frame, int frame_number, int64_t pts) {
    std::string output_path = getOutputPath(frame_number, pts);

    std::vector<int> params;
    switch (config_.format) {
        case OutputFormat::JPEG:
            params = {cv::IMWRITE_JPEG_QUALITY, config_.jpeg_quality};
            break;
        case OutputFormat::PNG:
            params = {cv::IMWRITE_PNG_COMPRESSION, 3};
            break;
        default:
            break;
    }

    return cv::imwrite(output_path, frame, params);
}

std::string FrameExtractor::getOutputPath(int frame_number, int64_t pts) {
    std::ostringstream oss;
    oss << config_.output_dir << "/"
        << config_.output_prefix << "_"
        << std::setfill('0') << std::setw(6) << frame_number;

    switch (config_.format) {
        case OutputFormat::PNG:
            oss << ".png";
            break;
        case OutputFormat::JPEG:
            oss << ".jpg";
            break;
        case OutputFormat::BMP:
            oss << ".bmp";
            break;
    }

    return oss.str();
}

std::string FrameExtractor::formatTimestamp(int64_t pts) {
    int64_t total_seconds = pts / GST_SECOND;
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;
    int milliseconds = (pts % GST_SECOND) / 1000000;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds << "."
        << std::setfill('0') << std::setw(3) << milliseconds;

    return oss.str();
}

void FrameExtractor::queryDuration() {
    gint64 duration = 0;
    if (gst_element_query_duration(pipeline_, GST_FORMAT_TIME, &duration)) {
        duration_ = duration;
    }
}

int FrameExtractor::getExtractedFrameCount() const {
    return extracted_count_;
}

int64_t FrameExtractor::getTotalFrameCount() const {
    return frame_count_;
}

int64_t FrameExtractor::getDuration() const {
    return duration_;
}

int64_t FrameExtractor::getCurrentPosition() const {
    return current_position_;
}

void FrameExtractor::setFrameCallback(FrameCallback callback) {
    frame_callback_ = std::move(callback);
}

bool FrameExtractor::isRunning() const {
    return running_;
}

std::string FrameExtractor::getLastError() const {
    return last_error_;
}

} // namespace gst_frame_extractor
