#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <gst/gst.h>

namespace Utils {
    // GStreamer zaman formatını okunabilir hale getirme
    std::string formatTime(gint64 time);
    
    // Caps'ten format bilgisini çıkarma
    std::string getMediaTypeFromCaps(GstCaps *caps);
    
    // Hata mesajlarını işleme
    void handleError(GError *error, const char *debug);
    
    // Basit input yardımcıları
    bool getYesNoInput(const std::string& prompt);
    std::string getInput(const std::string& prompt);
}

#endif // UTILS_HPP