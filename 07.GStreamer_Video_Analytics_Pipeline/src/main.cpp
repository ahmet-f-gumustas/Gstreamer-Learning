/**
 * @file main.cpp
 * @brief GStreamer Video Analytics Pipeline main program
 *
 * This file contains the application entry point and starts
 * the pipeline by processing command-line arguments.
 */

#include <iostream>
#include <memory>
#include <string>
#include <csignal>
#include <cstring>
#include <thread>
#include <chrono>
#include <iomanip>
#include <yaml-cpp/yaml.h>
#include <gst/gst.h>

#include "pipeline_manager.h"
#include "video_processor.h"
#include "motion_detector.h"
#include "rtsp_streamer.h"

// Global pipeline manager (for signal handler)
std::unique_ptr<PipelineManager> g_pipeline_manager;

/**
 * @brief Signal handler (SIGINT, SIGTERM)
 * @param signum Signal number
 */
void signalHandler(int signum) {
    std::cout << "\n[INFO] Signal received (" << signum << "), shutting down..." << std::endl;
    
    if (g_pipeline_manager && g_pipeline_manager->isRunning()) {
        g_pipeline_manager->stop();
    }
    
    // Exit
    std::exit(0);
}

/**
 * @brief Displays usage information
 * @param program_name Program name
 */
void showUsage(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  -i, --input <source>      Video source (file/webcam/rtsp://...)\n"
              << "  -o, --output <target>     Output target (display/file/rtsp://...)\n"
              << "  -c, --config <file>       Configuration file (YAML)\n"
              << "  --motion-detect           Enable motion detection\n"
              << "  --use-gpu                 Use GPU acceleration\n"
              << "  --record <file>           Record video\n"
              << "  --width <width>           Video width (default: 1920)\n"
              << "  --height <height>         Video height (default: 1080)\n"
              << "  --fps <fps>               Frame rate (default: 30)\n"
              << "  --bitrate <bitrate>       Bit rate (default: 4000000)\n"
              << "  --rtsp-port <port>        RTSP server port (default: 8554)\n"
              << "  -v, --verbose             Verbose output\n"
              << "  -h, --help                Show this help message\n\n"
              << "Examples:\n"
              << "  # Process a video file\n"
              << "  " << program_name << " -i video.mp4\n\n"
              << "  # Motion detection with webcam\n"
              << "  " << program_name << " -i webcam --motion-detect\n\n"
              << "  # Process and stream via RTSP\n"
              << "  " << program_name << " -i rtsp://192.168.1.100:554/stream -o rtsp://0.0.0.0:8554/live\n\n"
              << "  # GPU video processing and recording\n"
              << "  " << program_name << " -i video.mp4 --use-gpu --record output.mp4\n"
              << std::endl;
}

/**
 * @brief Parses command-line arguments
 * @param argc Argument count
 * @param argv Argument array
 * @param config Pipeline configuration
 * @return true if successful
 */
bool parseArguments(int argc, char* argv[], PipelineConfig& config) {
    bool verbose = false;
    std::string config_file;
    
    // Process arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showUsage(argv[0]);
            return false;
        }
        else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
        else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            std::string input = argv[++i];
            
            // Determine input type
            if (input == "webcam") {
                config.source_type = SourceType::WEBCAM;
                config.source_location = "/dev/video0";
            }
            else if (input.find("rtsp://") == 0) {
                config.source_type = SourceType::RTSP;
                config.source_location = input;
            }
            else if (input.find("http://") == 0 || input.find("https://") == 0) {
                config.source_type = SourceType::HTTP;
                config.source_location = input;
            }
            else {
                config.source_type = SourceType::FILE;
                config.source_location = input;
            }
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            std::string output = argv[++i];
            
            // Determine output type
            if (output == "display") {
                config.sink_type = SinkType::DISPLAY;
            }
            else if (output.find("rtsp://") == 0) {
                config.sink_type = SinkType::RTSP;
                config.sink_location = output;
            }
            else {
                config.sink_type = SinkType::FILE;
                config.sink_location = output;
            }
        }
        else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_file = argv[++i];
        }
        else if (arg == "--motion-detect") {
            config.enable_motion_detection = true;
        }
        else if (arg == "--use-gpu") {
            config.enable_gpu_acceleration = true;
        }
        else if (arg == "--record" && i + 1 < argc) {
            config.enable_recording = true;
            config.record_location = argv[++i];
        }
        else if (arg == "--width" && i + 1 < argc) {
            config.width = std::stoi(argv[++i]);
        }
        else if (arg == "--height" && i + 1 < argc) {
            config.height = std::stoi(argv[++i]);
        }
        else if (arg == "--fps" && i + 1 < argc) {
            config.framerate = std::stoi(argv[++i]);
        }
        else if (arg == "--bitrate" && i + 1 < argc) {
            config.bitrate = std::stoi(argv[++i]);
        }
        else if (arg == "--rtsp-port" && i + 1 < argc) {
            config.rtsp_port = std::stoi(argv[++i]);
        }
        else {
            std::cerr << "[ERROR] Unknown argument: " << arg << std::endl;
            showUsage(argv[0]);
            return false;
        }
    }
    
    // Load config file if provided
    if (!config_file.empty()) {
        try {
            YAML::Node yaml_config = YAML::LoadFile(config_file);
            
            // Pipeline settings
            if (yaml_config["pipeline"]) {
                auto pipeline = yaml_config["pipeline"];
                
                // Input settings
                if (pipeline["input"]) {
                    auto input = pipeline["input"];
                    std::string type = input["type"].as<std::string>();
                    
                    if (type == "file") config.source_type = SourceType::FILE;
                    else if (type == "webcam") config.source_type = SourceType::WEBCAM;
                    else if (type == "rtsp") config.source_type = SourceType::RTSP;
                    else if (type == "http") config.source_type = SourceType::HTTP;
                    
                    config.source_location = input["location"].as<std::string>();
                }
                
                // Processing settings
                if (pipeline["processing"]) {
                    auto processing = pipeline["processing"];
                    config.enable_motion_detection = processing["motion_detection"].as<bool>(false);
                    config.enable_gpu_acceleration = processing["gpu_acceleration"].as<bool>(false);
                }
                
                // Output settings
                if (pipeline["output"]) {
                    auto output = pipeline["output"];
                    std::string type = output["type"].as<std::string>();
                    
                    if (type == "display") config.sink_type = SinkType::DISPLAY;
                    else if (type == "file") config.sink_type = SinkType::FILE;
                    else if (type == "rtsp") config.sink_type = SinkType::RTSP;
                    
                    config.sink_location = output["location"].as<std::string>("");
                }
            }
        }
        catch (const YAML::Exception& e) {
            std::cerr << "[ERROR] Failed to read config file: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Default values
    if (config.source_location.empty()) {
        std::cerr << "[ERROR] No video source specified!" << std::endl;
        showUsage(argv[0]);
        return false;
    }
    
    // Debug output
    if (verbose) {
        std::cout << "\n[CONFIG] Pipeline Configuration:" << std::endl;
        std::cout << "  Source: " << config.source_location << std::endl;
        std::cout << "  Resolution: " << config.width << "x" << config.height << "@" << config.framerate << "fps" << std::endl;
        std::cout << "  Bitrate: " << config.bitrate << " bps" << std::endl;
        std::cout << "  GPU Acceleration: " << (config.enable_gpu_acceleration ? "Yes" : "No") << std::endl;
        std::cout << "  Motion Detection: " << (config.enable_motion_detection ? "Yes" : "No") << std::endl;

        if (config.sink_type == SinkType::RTSP) {
            std::cout << "  RTSP Output: " << config.sink_location << std::endl;
        }

        if (config.enable_recording) {
            std::cout << "  Recording: " << config.record_location << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    return true;
}

/**
 * @brief Displays performance information
 * @param pipeline Pipeline manager
 */
void showPerformanceInfo(PipelineManager* pipeline) {
    // Clear terminal and write header
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "=== GStreamer Video Analytics Pipeline ===" << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;
    std::cout << std::string(42, '-') << std::endl;
    
    // FPS info
    std::cout << "FPS: " << std::fixed << std::setprecision(2) 
              << pipeline->getCurrentFPS() << std::endl;
    
    // Video processing statistics
    if (auto* processor = pipeline->getVideoProcessor()) {
        auto stats = processor->getStats();
        std::cout << "Processed Frames: " << stats.frames_processed << std::endl;
        std::cout << "Average Processing Time: " << stats.avg_processing_time << " ms" << std::endl;
    }
    
    // Motion detection statistics
    if (auto* detector = pipeline->getMotionDetector()) {
        auto stats = detector->getStats();
        std::cout << "\nMotion Detection:" << std::endl;
        std::cout << "  Motion Frames: " << stats.motion_frames
                  << " / " << stats.total_frames << std::endl;
        std::cout << "  Average Motion Area: " << std::fixed << std::setprecision(2)
                  << stats.average_motion_area << "%" << std::endl;
        
        // Active motion regions
        auto motions = detector->getCurrentMotions();
        if (!motions.empty()) {
            std::cout << "  Active Regions: " << motions.size() << std::endl;
        }
    }
    
    // RTSP server statistics
    if (auto* streamer = pipeline->getRTSPStreamer()) {
        auto stats = streamer->getStats();
        std::cout << "\nRTSP Server:" << std::endl;
        std::cout << "  URL: " << streamer->getStreamURL() << std::endl;
        std::cout << "  Active Clients: " << stats.active_clients << std::endl;
        std::cout << "  Total Bandwidth: " << std::fixed << std::setprecision(2)
                  << stats.average_bandwidth << " Mbps" << std::endl;
    }
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Startup message
    std::cout << "GStreamer Video Analytics Pipeline v1.0.0" << std::endl;
    std::cout << "GStreamer " << gst_version_string() << std::endl;
    std::cout << std::string(42, '-') << std::endl;
    
    // Read configuration
    PipelineConfig config;
    if (!parseArguments(argc, argv, config)) {
        return 1;
    }
    
    try {
        // Create pipeline manager
        g_pipeline_manager = std::make_unique<PipelineManager>(config);
        
        // Set error callback
        g_pipeline_manager->setErrorCallback([](const std::string& error) {
            std::cerr << "\n[ERROR] " << error << std::endl;
        });

        // Set EOS callback
        g_pipeline_manager->setEOSCallback([]() {
            std::cout << "\n[INFO] End of video detected." << std::endl;
            g_pipeline_manager->stop();
        });
        
        // Motion detection callback
        if (config.enable_motion_detection && g_pipeline_manager->getMotionDetector()) {
            g_pipeline_manager->getMotionDetector()->setMotionEventCallback(
                [](const std::vector<MotionRegion>& regions, guint64 timestamp, double percentage) {
                    if (percentage > 5.0) { // More than 5% motion
                        std::cout << "\n[MOTION] " << regions.size()
                                  << " region(s) with motion detected ("
                                  << std::fixed << std::setprecision(1) << percentage 
                                  << "% area)" << std::endl;
                    }
                }
            );
        }
        
        // Start the pipeline
        std::cout << "[INFO] Starting pipeline..." << std::endl;
        if (!g_pipeline_manager->start()) {
            std::cerr << "[ERROR] Failed to start pipeline!" << std::endl;
            return 1;
        }
        
        std::cout << "[INFO] Pipeline started successfully." << std::endl;

        // Main loop - display performance information
        while (g_pipeline_manager->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            showPerformanceInfo(g_pipeline_manager.get());
        }
        
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        return 1;
    }
    
    // Cleanup
    std::cout << "\n[INFO] Program terminating..." << std::endl;
    g_pipeline_manager.reset();
    
    return 0;
}