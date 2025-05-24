#include "Utils.hpp"
#include <fstream>

bool Utils::gstreamerInitialized = false;

bool Utils::initializeGStreamer(int argc, char* argv[]) {
    if (gstreamerInitialized) {
        return true;
    }
    
    GError* error = nullptr;
    if (!gst_init_check(&argc, &argv, &error)) {
        if (error) {
            logError("GStreamer başlatılamadı: " + std::string(error->message));
            g_error_free(error);
        }
        return false;
    }
    
    gstreamerInitialized = true;
    logInfo("GStreamer başarıyla başlatıldı");
    return true;
}

void Utils::printGstMessage(GstMessage* message) {
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError* error;
            gchar* debug;
            gst_message_parse_error(message, &error, &debug);
            logError("GStreamer Error: " + std::string(error->message));
            if (debug) {
                logError("Debug: " + std::string(debug));
                g_free(debug);
            }
            g_error_free(error);
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError* error;
            gchar* debug;
            gst_message_parse_warning(message, &error, &debug);
            logWarning("GStreamer Warning: " + std::string(error->message));
            if (debug) {
                logWarning("Debug: " + std::string(debug));
                g_free(debug);
            }
            g_error_free(error);
            break;
        }
        case GST_MESSAGE_EOS:
            logInfo("Stream sonu (EOS) alındı");
            break;
        case GST_MESSAGE_STATE_CHANGED: {
            GstState oldState, newState, pendingState;
            gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
            if (GST_MESSAGE_SRC(message) == GST_OBJECT(gst_element_get_parent(GST_ELEMENT(GST_MESSAGE_SRC(message))))) {
                logInfo("Pipeline durumu değişti: " + gstStateToString(oldState) + 
                       " -> " + gstStateToString(newState));
            }
            break;
        }
        default:
            break;
    }
}

std::string Utils::gstStateToString(GstState state) {
    switch (state) {
        case GST_STATE_VOID_PENDING: return "VOID_PENDING";
        case GST_STATE_NULL: return "NULL";
        case GST_STATE_READY: return "READY";
        case GST_STATE_PAUSED: return "PAUSED";
        case GST_STATE_PLAYING: return "PLAYING";
        default: return "UNKNOWN";
    }
}

void Utils::cleanupGStreamer() {
    if (gstreamerInitialized) {
        gst_deinit();
        gstreamerInitialized = false;
        logInfo("GStreamer temizlendi");
    }
}

void Utils::logInfo(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void Utils::logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void Utils::logWarning(const std::string& message) {
    std::cout << "[WARNING] " << message << std::endl;
}

std::string Utils::detectVideoSource() {
    // Önce webcam kontrol et
    if (isWebcamAvailable()) {
        logInfo("Webcam bulundu, video kaynağı olarak kullanılacak");
        return "webcam";
    }
    
    // Test pattern kullan
    logInfo("Webcam bulunamadı, test pattern kullanılacak");
    return "test";
}

bool Utils::isWebcamAvailable() {
    std::ifstream file("/dev/video0");
    bool available = file.good();
    file.close();
    
    if (!available) {
        // v4l2-ctl ile kontrol et
        int result = system("v4l2-ctl --list-devices > /dev/null 2>&1");
        available = (result == 0);
    }
    
    return available;
}