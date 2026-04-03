/**
 * @file pipeline_manager.cpp
 * @brief GStreamer pipeline management class implementation
 */

#include "pipeline_manager.h"
#include "video_processor.h"
#include "motion_detector.h"
#include "rtsp_streamer.h"
#include <iostream>
#include <sstream>

/**
 * @brief Constructor - Takes the pipeline configuration and creates the necessary objects
 */
PipelineManager::PipelineManager(const PipelineConfig& config)
    : config_(config) {
    
    // Create video processor
    video_processor_ = std::make_unique<VideoProcessor>();
    
    // Create motion detector (if enabled)
    if (config_.enable_motion_detection) {
        motion_detector_ = std::make_unique<MotionDetector>();
    }
    
    // Create RTSP streamer (if RTSP output is configured)
    if (config_.sink_type == SinkType::RTSP) {
        RTSPConfig rtsp_config;
        rtsp_config.port = config_.rtsp_port;
        rtsp_config.mount_point = config_.rtsp_mount_point;
        rtsp_config.width = config_.width;
        rtsp_config.height = config_.height;
        rtsp_config.framerate = config_.framerate;
        rtsp_config.bitrate = config_.bitrate;
        rtsp_config.encoder = config_.encoder;
        
        rtsp_streamer_ = std::make_unique<RTSPStreamer>(rtsp_config);
    }
}

/**
 * @brief Destructor - Cleans up resources
 */
PipelineManager::~PipelineManager() {
    stop();
    cleanup();
}

/**
 * @brief Starts the pipeline
 */
bool PipelineManager::start() {
    // Don't start if already running
    if (is_running_.load()) {
        return true;
    }
    
    // Create pipeline
    if (!createPipeline()) {
        std::cerr << "[PipelineManager] Failed to create pipeline!" << std::endl;
        return false;
    }
    
    // Start RTSP server (if available)
    if (rtsp_streamer_ && config_.sink_type == SinkType::RTSP) {
        if (!rtsp_streamer_->start()) {
            std::cerr << "[PipelineManager] Failed to start RTSP server!" << std::endl;
            cleanup();
            return false;
        }
    }
    
    // Set pipeline to PLAYING state
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[PipelineManager] Failed to start pipeline!" << std::endl;
        cleanup();
        return false;
    }
    
    // Start main loop thread
    is_running_ = true;
    main_loop_thread_ = std::make_unique<std::thread>(&PipelineManager::mainLoopThread, this);
    
    // Record start time for FPS calculation
    last_fps_time_ = gst_clock_get_time(gst_element_get_clock(pipeline_));
    
    return true;
}

/**
 * @brief Stops the pipeline
 */
void PipelineManager::stop() {
    if (!is_running_.load()) {
        return;
    }
    
    is_running_ = false;
    
    // Stop main loop
    if (main_loop_) {
        g_main_loop_quit(main_loop_);
    }
    
    // Wait for thread to finish
    if (main_loop_thread_ && main_loop_thread_->joinable()) {
        main_loop_thread_->join();
    }

    // Stop pipeline
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }
    
    // Stop RTSP server
    if (rtsp_streamer_) {
        rtsp_streamer_->stop();
    }
}

/**
 * @brief Creates the pipeline and links elements
 */
bool PipelineManager::createPipeline() {
    // Create main pipeline
    pipeline_ = gst_pipeline_new("video-analytics-pipeline");
    if (!pipeline_) {
        return false;
    }
    
    // Create video source
    source_ = createSource();
    if (!source_) {
        std::cerr << "[PipelineManager] Failed to create video source!" << std::endl;
        return false;
    }
    
    // Video converter (for format compatibility)
    GstElement* videoconvert1 = gst_element_factory_make("videoconvert", "videoconvert1");
    
    // Video scaling (for size adjustment)
    GstElement* videoscale = gst_element_factory_make("videoscale", "videoscale");
    
    // Video rate adjuster
    GstElement* videorate = gst_element_factory_make("videorate", "videorate");
    
    // Caps filter (enforce video format)
    GstElement* capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, config_.width,
        "height", G_TYPE_INT, config_.height,
        "framerate", GST_TYPE_FRACTION, config_.framerate, 1,
        "format", G_TYPE_STRING, config_.video_format.c_str(),
        nullptr);
    g_object_set(capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);
    
    // Tee element (to split the stream)
    tee_ = gst_element_factory_make("tee", "tee");
    
    // Queue'lar
    GstElement* queue1 = gst_element_factory_make("queue", "queue1");
    GstElement* queue2 = gst_element_factory_make("queue", "queue2");
    
    // Video processing element
    GstElement* video_processor_element = nullptr;
    if (video_processor_) {
        video_processor_element = video_processor_->createElement();
        if (config_.enable_gpu_acceleration) {
            video_processor_->setGPUAcceleration(true);
        }
    }
    
    // Motion detection element
    GstElement* motion_detector_element = nullptr;
    if (motion_detector_ && config_.enable_motion_detection) {
        motion_detector_element = motion_detector_->createElement();
    }
    
    // Video converter (before sink)
    GstElement* videoconvert2 = gst_element_factory_make("videoconvert", "videoconvert2");
    
    // Create video sink
    sink_ = createSink();
    if (!sink_) {
        std::cerr << "[PipelineManager] Failed to create video sink!" << std::endl;
        return false;
    }
    
    // Add all elements to pipeline
    gst_bin_add_many(GST_BIN(pipeline_),
        source_, videoconvert1, videoscale, videorate, capsfilter,
        tee_, queue1, nullptr);
    
    // Add video processing element
    if (video_processor_element) {
        gst_bin_add(GST_BIN(pipeline_), video_processor_element);
    }
    
    // Add motion detection element
    if (motion_detector_element) {
        gst_bin_add(GST_BIN(pipeline_), motion_detector_element);
    }
    
    gst_bin_add_many(GST_BIN(pipeline_), videoconvert2, sink_, nullptr);
    
    // Create recording branch (if enabled)
    if (config_.enable_recording) {
        gst_bin_add(GST_BIN(pipeline_), queue2);
        
        recording_queue_ = gst_element_factory_make("queue", "recording_queue");
        GstElement* recording_convert = gst_element_factory_make("videoconvert", "recording_convert");
        GstElement* encoder = createEncoder();
        GstElement* muxer = gst_element_factory_make("mp4mux", "muxer");
        recording_sink_ = gst_element_factory_make("filesink", "recording_sink");
        
        g_object_set(recording_sink_, "location", config_.record_location.c_str(), nullptr);
        
        gst_bin_add_many(GST_BIN(pipeline_),
            recording_queue_, recording_convert, encoder, muxer, recording_sink_, nullptr);
        
        // Link recording branch
        if (!gst_element_link_many(queue2, recording_queue_, recording_convert,
                                   encoder, muxer, recording_sink_, nullptr)) {
            std::cerr << "[PipelineManager] Failed to link recording branch!" << std::endl;
            return false;
        }
    }
    
    // Link elements
    bool link_success = true;
    
    // Link based on source type
    if (config_.source_type == SourceType::RTSP || config_.source_type == SourceType::HTTP) {
        // Connect signal for dynamic pads
        g_signal_connect(source_, "pad-added", G_CALLBACK(onPadAdded), videoconvert1);
        
        // Link the rest of the pipeline
        link_success = gst_element_link_many(videoconvert1, videoscale, videorate, 
                                            capsfilter, tee_, nullptr);
    } else {
        // Link directly for sources with static pads
        link_success = gst_element_link_many(source_, videoconvert1, videoscale, 
                                            videorate, capsfilter, tee_, nullptr);
    }
    
    if (!link_success) {
        std::cerr << "[PipelineManager] Failed to link pipeline initial elements!" << std::endl;
        return false;
    }
    
    // Link tee to queue
    GstPad* tee_src1 = gst_element_get_request_pad(tee_, "src_%u");
    GstPad* queue1_sink = gst_element_get_static_pad(queue1, "sink");
    if (gst_pad_link(tee_src1, queue1_sink) != GST_PAD_LINK_OK) {
        std::cerr << "[PipelineManager] Tee -> Queue1 link failed!" << std::endl;
        return false;
    }
    gst_object_unref(queue1_sink);
    
    // Link main processing branch
    GstElement* current = queue1;
    
    if (video_processor_element) {
        if (!gst_element_link(current, video_processor_element)) {
            std::cerr << "[PipelineManager] Failed to link video processor!" << std::endl;
            return false;
        }
        current = video_processor_element;
    }
    
    if (motion_detector_element) {
        if (!gst_element_link(current, motion_detector_element)) {
            std::cerr << "[PipelineManager] Failed to link motion detector!" << std::endl;
            return false;
        }
        current = motion_detector_element;
    }
    
    if (!gst_element_link_many(current, videoconvert2, sink_, nullptr)) {
        std::cerr << "[PipelineManager] Failed to link final elements!" << std::endl;
        return false;
    }
    
    // Tee link for recording branch
    if (config_.enable_recording) {
        GstPad* tee_src2 = gst_element_get_request_pad(tee_, "src_%u");
        GstPad* queue2_sink = gst_element_get_static_pad(queue2, "sink");
        if (gst_pad_link(tee_src2, queue2_sink) != GST_PAD_LINK_OK) {
            std::cerr << "[PipelineManager] Tee -> Queue2 link failed!" << std::endl;
            return false;
        }
        gst_object_unref(queue2_sink);
    }
    
    // Listen to bus
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
    gst_bus_add_watch(bus, busCallback, this);
    gst_object_unref(bus);
    
    // Add elements to map
    elements_["source"] = source_;
    elements_["tee"] = tee_;
    elements_["sink"] = sink_;
    
    return true;
}

/**
 * @brief Creates the video source
 */
GstElement* PipelineManager::createSource() {
    GstElement* source = nullptr;
    
    switch (config_.source_type) {
        case SourceType::FILE:
            source = gst_element_factory_make("filesrc", "source");
            g_object_set(source, "location", config_.source_location.c_str(), nullptr);
            
            // Decodebin ekle
            {
                GstElement* decodebin = gst_element_factory_make("decodebin", "decoder");
                gst_bin_add(GST_BIN(pipeline_), decodebin);
                gst_element_link(source, decodebin);
                
                // Connect pads from decodebin
                g_signal_connect(decodebin, "pad-added", G_CALLBACK(onPadAdded), nullptr);
                
                // Return decodebin as source
                return decodebin;
            }
            break;
            
        case SourceType::WEBCAM:
            source = gst_element_factory_make("v4l2src", "source");
            g_object_set(source, "device", config_.source_location.c_str(), nullptr);
            break;
            
        case SourceType::RTSP:
            source = gst_element_factory_make("rtspsrc", "source");
            g_object_set(source, 
                "location", config_.source_location.c_str(),
                "latency", 200,
                "buffer-mode", 0, // For live stream
                nullptr);
            break;
            
        case SourceType::HTTP:
            source = gst_element_factory_make("souphttpsrc", "source");
            g_object_set(source, 
                "location", config_.source_location.c_str(),
                "is-live", TRUE,
                nullptr);
            break;
            
        default:
            break;
    }
    
    return source;
}

/**
 * @brief Creates the video sink
 */
GstElement* PipelineManager::createSink() {
    GstElement* sink = nullptr;
    
    switch (config_.sink_type) {
        case SinkType::DISPLAY:
            // Choose appropriate sink based on platform
            sink = gst_element_factory_make("autovideosink", "sink");
            break;
            
        case SinkType::FILE:
            {
                // Create encoder and muxer
                GstElement* encoder = createEncoder();
                GstElement* muxer = gst_element_factory_make("mp4mux", "muxer");
                GstElement* filesink = gst_element_factory_make("filesink", "filesink");
                
                g_object_set(filesink, "location", config_.sink_location.c_str(), nullptr);
                
                // Create bin
                GstElement* bin = gst_bin_new("file_sink_bin");
                gst_bin_add_many(GST_BIN(bin), encoder, muxer, filesink, nullptr);
                gst_element_link_many(encoder, muxer, filesink, nullptr);
                
                // Ghost pad ekle
                GstPad* sink_pad = gst_element_get_static_pad(encoder, "sink");
                GstPad* ghost_pad = gst_ghost_pad_new("sink", sink_pad);
                gst_element_add_pad(bin, ghost_pad);
                gst_object_unref(sink_pad);
                
                sink = bin;
            }
            break;
            
        case SinkType::RTSP:
            // Use appsink for RTSP
            sink = gst_element_factory_make("appsink", "sink");
            g_object_set(sink, 
                "emit-signals", TRUE,
                "max-buffers", 10,
                "drop", TRUE,
                nullptr);
            
            // Connect to RTSP streamer
            if (rtsp_streamer_) {
                rtsp_streamer_->setVideoSource(sink);
            }
            break;
            
        default:
            sink = gst_element_factory_make("fakesink", "sink");
            break;
    }
    
    return sink;
}

/**
 * @brief Creates the video encoder
 */
GstElement* PipelineManager::createEncoder() {
    GstElement* encoder = nullptr;
    
    if (config_.enable_gpu_acceleration) {
        // Use NVIDIA GPU encoder
        encoder = gst_element_factory_make("nvh264enc", "encoder");
        if (encoder) {
            g_object_set(encoder,
                "preset", 2, // Low latency, high quality
                "bitrate", config_.bitrate / 1000, // kbps
                nullptr);
            return encoder;
        }
    }
    
    // CPU encoder (x264)
    encoder = gst_element_factory_make("x264enc", "encoder");
    if (encoder) {
        g_object_set(encoder,
            "bitrate", config_.bitrate / 1000, // kbps
            "speed-preset", 2, // superfast
            "tune", 0x00000004, // zerolatency
            nullptr);
    }
    
    return encoder;
}

/**
 * @brief Handles bus messages
 */
gboolean PipelineManager::busCallback(GstBus* bus, GstMessage* msg, gpointer user_data) {
    PipelineManager* manager = static_cast<PipelineManager*>(user_data);
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            {
                GError* error;
                gchar* debug_info;
                gst_message_parse_error(msg, &error, &debug_info);
                
                std::string error_msg = error->message;
                if (manager->error_callback_) {
                    manager->error_callback_(error_msg);
                }
                
                g_clear_error(&error);
                g_free(debug_info);
                
                // Stop the pipeline
                manager->stop();
            }
            break;
            
        case GST_MESSAGE_WARNING:
            {
                GError* error;
                gchar* debug_info;
                gst_message_parse_warning(msg, &error, &debug_info);
                
                std::cerr << "[WARNING] " << error->message << std::endl;
                
                g_clear_error(&error);
                g_free(debug_info);
            }
            break;
            
        case GST_MESSAGE_EOS:
            if (manager->eos_callback_) {
                manager->eos_callback_();
            }
            break;
            
        case GST_MESSAGE_STATE_CHANGED:
            {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(manager->pipeline_)) {
                    std::cout << "[STATE] Pipeline state: " 
                              << gst_element_state_get_name(old_state) << " -> "
                              << gst_element_state_get_name(new_state) << std::endl;
                }
            }
            break;
            
        case GST_MESSAGE_BUFFERING:
            {
                gint percent = 0;
                gst_message_parse_buffering(msg, &percent);
                std::cout << "[BUFFERING] %" << percent << std::endl;
            }
            break;
            
        default:
            break;
    }
    
    return TRUE;
}

/**
 * @brief Callback for dynamic pad linking
 */
void PipelineManager::onPadAdded(GstElement* src, GstPad* new_pad, gpointer data) {
    GstElement* sink = static_cast<GstElement*>(data);
    GstPad* sink_pad = gst_element_get_static_pad(sink, "sink");
    
    if (gst_pad_is_linked(sink_pad)) {
        gst_object_unref(sink_pad);
        return;
    }
    
    // Check pad caps
    GstCaps* new_pad_caps = gst_pad_get_current_caps(new_pad);
    if (!new_pad_caps) {
        new_pad_caps = gst_pad_query_caps(new_pad, nullptr);
    }
    
    GstStructure* new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
    const gchar* new_pad_type = gst_structure_get_name(new_pad_struct);
    
    // Only link video pads
    if (g_str_has_prefix(new_pad_type, "video/")) {
        if (gst_pad_link(new_pad, sink_pad) == GST_PAD_LINK_OK) {
            std::cout << "[PAD] Video pad linked: " << new_pad_type << std::endl;
        }
    }
    
    gst_caps_unref(new_pad_caps);
    gst_object_unref(sink_pad);
}

/**
 * @brief Main event loop thread function
 */
void PipelineManager::mainLoopThread() {
    // Create GMainLoop
    main_loop_ = g_main_loop_new(nullptr, FALSE);

    // FPS calculation timer
    g_timeout_add(1000, [](gpointer data) -> gboolean {
        PipelineManager* manager = static_cast<PipelineManager*>(data);
        manager->calculateFPS();
        return TRUE;
    }, this);
    
    // Run main loop
    g_main_loop_run(main_loop_);
    
    // Cleanup
    g_main_loop_unref(main_loop_);
    main_loop_ = nullptr;
}

/**
 * @brief FPS calculation function
 */
void PipelineManager::calculateFPS() {
    if (!pipeline_ || !is_running_.load()) {
        return;
    }
    
    // Get current time
    GstClock* clock = gst_element_get_clock(pipeline_);
    if (!clock) {
        return;
    }
    
    GstClockTime current_time = gst_clock_get_time(clock);
    gst_object_unref(clock);
    
    // FPS hesapla
    if (last_fps_time_ > 0) {
        GstClockTime diff = current_time - last_fps_time_;
        if (diff > 0) {
            // Get frame count from video processor
            if (video_processor_) {
                auto stats = video_processor_->getStats();
                guint64 current_frames = stats.frames_processed;
                guint64 frame_diff = current_frames - frame_count_;
                
                current_fps_ = (double)frame_diff * GST_SECOND / diff;
                frame_count_ = current_frames;
            }
        }
    }
    
    last_fps_time_ = current_time;
}

/**
 * @brief Cleans up the pipeline
 */
void PipelineManager::cleanup() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    
    elements_.clear();
}

/**
 * @brief Returns the pipeline state
 */
GstState PipelineManager::getState() const {
    if (!pipeline_) {
        return GST_STATE_NULL;
    }
    
    GstState state;
    GstState pending;
    gst_element_get_state(pipeline_, &state, &pending, GST_CLOCK_TIME_NONE);
    
    return state;
}

/**
 * @brief Changes the video source at runtime
 */
bool PipelineManager::changeSource(const std::string& new_source, SourceType type) {
    // TODO: Dynamic source switching implementation
    return false;
}

/**
 * @brief Starts/stops the recording
 */
bool PipelineManager::toggleRecording(bool start, const std::string& filename) {
    if (start && !is_recording_) {
        // Start recording
        if (recording_sink_) {
            g_object_set(recording_sink_, "location", filename.c_str(), nullptr);
            is_recording_ = true;
            return true;
        }
    } else if (!start && is_recording_) {
        // Stop recording
        is_recording_ = false;
        return true;
    }
    
    return false;
}