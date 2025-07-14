#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include "video_mosaic.h"

std::unique_ptr<VideoMosaic> g_mosaic;

void signalHandler(int signal) {
    if (g_mosaic && g_mosaic->isRunning()) {
        std::cout << "\nStopping video mosaic..." << std::endl;
        g_mosaic->stop();
    }
}

int main(int argc, char* argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Create video mosaic
    g_mosaic = std::make_unique<VideoMosaic>();
    
    // Load configuration
    std::string config_file = "config/mosaic_config.yaml";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    if (!g_mosaic->initialize(config_file)) {
        std::cerr << "Failed to initialize video mosaic" << std::endl;
        return 1;
    }
    
    // Add video sources (examples)
    // You can modify these to use your own sources
    g_mosaic->addVideoSource("v4l2src device=/dev/video0", "Camera1");
    g_mosaic->addVideoSource("videotestsrc pattern=smpte", "TestPattern1");
    g_mosaic->addVideoSource("videotestsrc pattern=ball", "TestPattern2");
    g_mosaic->addVideoSource("rtsp://localhost:8554/test", "RTSP_Stream");
    
    // Start the mosaic
    if (!g_mosaic->start()) {
        std::cerr << "Failed to start video mosaic" << std::endl;
        return 1;
    }
    
    std::cout << "Video mosaic is running. Press Ctrl+C to stop." << std::endl;
    
    // Keep the application running
    while (g_mosaic->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return 0;
}