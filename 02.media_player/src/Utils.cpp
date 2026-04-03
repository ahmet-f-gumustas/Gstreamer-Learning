#include "Utils.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>


namespace Utils {

std::string formatTime(gint64 time) {
    int hours, minutes, seconds, milliseconds;
    
    // Convert from nanoseconds to other units
    time /= GST_MSECOND;  // Nanoseconds to milliseconds
    milliseconds = time % 1000;
    
    time /= 1000;  // Milliseconds to seconds
    seconds = time % 60;
    
    time /= 60;  // Seconds to minutes
    minutes = time % 60;
    
    time /= 60;  // Minutes to hours
    hours = static_cast<int>(time);
    
    // Format
    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << ":";
    }
    
    oss << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << seconds << "."
        << std::setw(3) << std::setfill('0') << milliseconds;
    
    return oss.str();
}

std::string getMediaTypeFromCaps(GstCaps *caps) {
    if (!caps)
        return "Unknown format";
    
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(structure);
    
    std::string type;
    if (g_str_has_prefix(name, "video/")) {
        type = "Video: ";
        
        // Video format details
        int width, height;
        if (gst_structure_get_int(structure, "width", &width) &&
            gst_structure_get_int(structure, "height", &height)) {
            type += std::to_string(width) + "x" + std::to_string(height) + " ";
        }
        
        // Codec
        const gchar *encoding = gst_structure_get_string(structure, "encoding-name");
        if (encoding) {
            type += std::string(encoding);
        } else {
            // Use structure name as a last resort
            type += std::string(name).substr(6); // Strip "video/" prefix
        }
    }
    else if (g_str_has_prefix(name, "audio/")) {
        type = "Audio: ";

        // Audio format details
        int rate, channels;
        if (gst_structure_get_int(structure, "rate", &rate)) {
            type += std::to_string(rate) + " Hz ";
        }
        
        if (gst_structure_get_int(structure, "channels", &channels)) {
            type += std::to_string(channels) + " channels ";
        }
        
        // Codec
        const gchar *encoding = gst_structure_get_string(structure, "encoding-name");
        if (encoding) {
            type += std::string(encoding);
        } else {
            // Use structure name as a last resort
            type += std::string(name).substr(6); // Strip "audio/" prefix
        }
    }
    else {
        type = "Other: " + std::string(name);
    }
    
    return type;
}

void handleError(GError *error, const char *debug) {
    std::cerr << "ERROR: " << error->message << std::endl;
    if (debug) {
        std::cerr << "Debug info: " << debug << std::endl;
    }
}

bool getYesNoInput(const std::string& prompt) {
    std::string response;
    std::cout << prompt << " (y/n): ";
    std::cin >> response;

    return (response == "y" || response == "Y" ||
            response == "yes" || response == "Yes" ||
            response == "YES");
}

std::string getInput(const std::string& prompt) {
    std::string response;
    std::cout << prompt << ": ";
    
    // Clear the trailing newline character
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::getline(std::cin, response);
    return response;
}

} // namespace Utils