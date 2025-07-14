#include "video_mosaic.h"
#include "input_manager.h"
#include "mosaic_layout.h"
#include <iostream>

VideoMosaic::VideoMosaic() 
    : pipeline_(nullptr)
    , compositor_(nullptr)
    , video_sink_(nullptr)
    , is_running_(false)
    , main_loop_(nullptr) {
}

VideoMosaic::~VideoMosaic() {
    stop();
    if (pipeline_) {
        gst_object_unref(pipeline_);
    }
    if (main_loop_) {
        g_main_loop_unref(main_loop_);
    }
}

bool VideoMosaic::initialize(const std::string& config_file) {
    // Create layout manager
    layout_manager_ = std::make_unique<MosaicLayout>();
    if (!layout_manager_->loadConfig(config_file)) {
        std::cerr << "Failed to load layout configuration" << std::endl;
        return false;
    }
    
    // Create pipeline
    if (!createPipeline()) {
        return false;
    }
    
    // Create input manager
    input_manager_ = std::make_unique<InputManager>(pipeline_);
    
    // Create main loop
    main_loop_ = g_main_loop_new(nullptr, FALSE);
    
    return true;
}

bool VideoMosaic::createPipeline() {
    // Create pipeline
    pipeline_ = gst_pipeline_new("video-mosaic-pipeline");
    if (!pipeline_) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return false;
    }
    
    // Create compositor
    compositor_ = gst_element_factory_make("compositor", "mosaic-compositor");
    if (!compositor_) {
        std::cerr << "Failed to create compositor" << std::endl;
        return false;
    }
    
    // Set compositor background
    auto& config = layout_manager_->getConfig();
    g_object_set(compositor_, 
        "background", 1,  // Enable background
        nullptr);
    
    // Create video converter
    GstElement* converter = gst_element_factory_make("videoconvert", "output-converter");
    
    // Create video sink
    video_sink_ = gst_element_factory_make("autovideosink", "video-output");
    if (!video_sink_) {
        video_sink_ = gst_element_factory_make("xvimagesink", "video-output");
    }
    
    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(pipeline_), compositor_, converter, video_sink_, nullptr);
    
    // Link elements
    if (!gst_element_link_many(compositor_, converter, video_sink_, nullptr)) {
        std::cerr << "Failed to link pipeline elements" << std::endl;
        return false;
    }
    
    // Set up bus watch
    GstBus* bus = gst_element_get_bus(pipeline_);
    gst_bus_add_watch(bus, busCallback, this);
    gst_object_unref(bus);
    
    return true;
}

bool VideoMosaic::addVideoSource(const std::string& uri, const std::string& name) {
    if (!input_manager_) {
        return false;
    }
    
    // Add input
    if (!input_manager_->addInput(uri, name)) {
        return false;
    }
    
    // Connect to compositor
    input_manager_->connectToCompositor(compositor_);
    
    // Update layout
    auto& inputs = input_manager_->getInputs();
    int grid_cols = layout_manager_->getConfig().grid_cols;
    
    for (size_t i = 0; i < inputs.size(); ++i) {
        int grid_x = i % grid_cols;
        int grid_y = i / grid_cols;
        input_manager_->setGridPosition(inputs[i]->name, grid_x, grid_y);
    }
    
    return true;
}

bool VideoMosaic::start() {
    if (!pipeline_ || is_running_) {
        return false;
    }
    
    // Start pipeline
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to start pipeline" << std::endl;
        return false;
    }
    
    is_running_ = true;
    
    // Start main loop in a separate thread
    std::thread loop_thread([this]() {
        g_main_loop_run(main_loop_);
    });
    loop_thread.detach();
    
    return true;
}

void VideoMosaic::stop() {
    if (!is_running_) {
        return;
    }
    
    is_running_ = false;
    
    // Stop main loop
    if (main_loop_) {
        g_main_loop_quit(main_loop_);
    }
    
    // Stop pipeline
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }
}

gboolean VideoMosaic::busCallback(GstBus* bus, GstMessage* msg, gpointer data) {
    VideoMosaic* mosaic = static_cast<VideoMosaic*>(data);
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err;
            gchar* debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            std::cerr << "Error: " << err->message << std::endl;
            std::cerr << "Debug info: " << (debug_info ? debug_info : "none") << std::endl;
            g_clear_error(&err);
            g_free(debug_info);
            mosaic->stop();
            break;
        }
        case GST_MESSAGE_EOS:
            std::cout << "End of stream" << std::endl;
            mosaic->stop();
            break;
        case GST_MESSAGE_STATE_CHANGED: {
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(mosaic->pipeline_)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                std::cout << "Pipeline state changed from " 
                          << gst_element_state_get_name(old_state) << " to "
                          << gst_element_state_get_name(new_state) << std::endl;
            }
            break;
        }
        default:
            break;
    }
    
    return TRUE;
}