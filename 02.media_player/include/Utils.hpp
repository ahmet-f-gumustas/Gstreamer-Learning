#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <gst/gst.h>

namespace Utils {
    // Convert GStreamer time format to human-readable string
    std::string formatTime(gint64 time);
    
    // Extract format information from caps
    std::string getMediaTypeFromCaps(GstCaps *caps);
    
    // Handle error messages
    void handleError(GError *error, const char *debug);
    
    // Simple input helpers
    bool getYesNoInput(const std::string& prompt);
    std::string getInput(const std::string& prompt);
}

#endif // UTILS_HPP