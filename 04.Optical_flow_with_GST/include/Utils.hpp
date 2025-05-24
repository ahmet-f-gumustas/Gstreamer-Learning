#ifndef UTILS_HPP
#define UTILS_HPP

#include <gst/gst.h>
#include <string>
#include <iostream>

class Utils {
public:
    // GStreamer utility functions
    static bool initializeGStreamer(int argc, char* argv[]);
    static void printGstMessage(GstMessage* message);
    static std::string gstStateToString(GstState state);
    static void cleanupGStreamer();
    
    // Debug and logging functions
    static void logInfo(const std::string& message);
    static void logError(const std::string& message);
    static void logWarning(const std::string& message);
    
    // Video source detection
    static std::string detectVideoSource();
    static bool isWebcamAvailable();
    
private:
    static bool gstreamerInitialized;
};

#endif // UTILS_HPP