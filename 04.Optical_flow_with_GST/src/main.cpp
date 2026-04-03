#include "OpticalFlowDetector.hpp"
#include "Utils.hpp"
#include <iostream>
#include <signal.h>
#include <memory>

// Global detector pointer for signal handling
std::unique_ptr<OpticalFlowDetector> g_detector;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        Utils::logInfo("Exit signal received, terminating program...");
        if (g_detector) {
            g_detector->stop();
        }
        Utils::cleanupGStreamer();
        exit(0);
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help          Show this help message" << std::endl;
    std::cout << "  -f, --file <path>   Use a video file" << std::endl;
    std::cout << "  -w, --webcam        Use webcam (default)" << std::endl;
    std::cout << "  -t, --test          Use test pattern" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << "                    # Use webcam" << std::endl;
    std::cout << "  " << programName << " -t                # Use test pattern" << std::endl;
    std::cout << "  " << programName << " -f video.mp4      # Use video file" << std::endl;
}

int main(int argc, char* argv[]) {
    // Signal handler setup
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize GStreamer
    if (!Utils::initializeGStreamer(argc, argv)) {
        Utils::logError("Failed to initialize GStreamer!");
        return -1;
    }
    
    // Parse command line arguments
    std::string videoSource = "";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                videoSource = argv[++i];
                Utils::logInfo("Using video file: " + videoSource);
            } else {
                Utils::logError("File path not specified!");
                printUsage(argv[0]);
                return -1;
            }
        } else if (arg == "-w" || arg == "--webcam") {
            videoSource = "webcam";
            Utils::logInfo("Using webcam");
        } else if (arg == "-t" || arg == "--test") {
            videoSource = "test";
            Utils::logInfo("Using test pattern");
        } else {
            Utils::logWarning("Unknown argument: " + arg);
        }
    }
    
    // Create Optical Flow Detector
    g_detector = std::make_unique<OpticalFlowDetector>();
    
    if (!g_detector->initialize(videoSource)) {
        Utils::logError("Failed to initialize OpticalFlowDetector!");
        Utils::cleanupGStreamer();
        return -1;
    }
    
    Utils::logInfo("=== GStreamer Optical Flow Detector ===");
    Utils::logInfo("Controls:");
    Utils::logInfo("  'q' key: Quit");
    Utils::logInfo("  Ctrl+C: Terminate program");
    Utils::logInfo("========================================");
    
    try {
        // Start main loop
        g_detector->run();
    } catch (const std::exception& e) {
        Utils::logError("An error occurred: " + std::string(e.what()));
    }
    
    // Cleanup
    g_detector->stop();
    g_detector.reset();
    Utils::cleanupGStreamer();
    
    Utils::logInfo("Program terminated successfully");
    return 0;
}