#include "Utils.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>  // Bu satırı ekleyin


namespace Utils {

std::string formatTime(gint64 time) {
    int hours, minutes, seconds, milliseconds;
    
    // Nanosaniyeden diğer birimlere dönüştür
    time /= GST_MSECOND;  // Nanosaniyeden milisaniyeye
    milliseconds = time % 1000;
    
    time /= 1000;  // Milisaniyeden saniyeye
    seconds = time % 60;
    
    time /= 60;  // Saniyeden dakikaya
    minutes = time % 60;
    
    time /= 60;  // Dakikadan saate
    hours = static_cast<int>(time);
    
    // Formatla
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
        return "Bilinmeyen format";
    
    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(structure);
    
    std::string type;
    if (g_str_has_prefix(name, "video/")) {
        type = "Video: ";
        
        // Video formatı detayları
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
            // Son çare olarak yapı adını kullan
            type += std::string(name).substr(6); // "video/" kısmını at
        }
    }
    else if (g_str_has_prefix(name, "audio/")) {
        type = "Ses: ";
        
        // Ses formatı detayları
        int rate, channels;
        if (gst_structure_get_int(structure, "rate", &rate)) {
            type += std::to_string(rate) + " Hz ";
        }
        
        if (gst_structure_get_int(structure, "channels", &channels)) {
            type += std::to_string(channels) + " kanal ";
        }
        
        // Codec
        const gchar *encoding = gst_structure_get_string(structure, "encoding-name");
        if (encoding) {
            type += std::string(encoding);
        } else {
            // Son çare olarak yapı adını kullan
            type += std::string(name).substr(6); // "audio/" kısmını at
        }
    }
    else {
        type = "Diğer: " + std::string(name);
    }
    
    return type;
}

void handleError(GError *error, const char *debug) {
    std::cerr << "HATA: " << error->message << std::endl;
    if (debug) {
        std::cerr << "Hata ayıklama bilgisi: " << debug << std::endl;
    }
}

bool getYesNoInput(const std::string& prompt) {
    std::string response;
    std::cout << prompt << " (e/h): ";
    std::cin >> response;
    
    return (response == "e" || response == "E" || 
            response == "evet" || response == "Evet" || 
            response == "EVET" || response == "y" || 
            response == "Y" || response == "yes");
}

std::string getInput(const std::string& prompt) {
    std::string response;
    std::cout << prompt << ": ";
    
    // Son satır sonu karakterini temizle
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    std::getline(std::cin, response);
    return response;
}

} // namespace Utils