#include "MediaPlayer.hpp"
#include "Utils.hpp"
#include <iostream>

MediaPlayer::MediaPlayer() 
    : pipeline(nullptr), bus(nullptr), busWatchId(0), currentState(State::STOPPED) {
    
    // Initialize GStreamer
    gst_init(nullptr, nullptr);
}

MediaPlayer::~MediaPlayer() {
    cleanup();
}

bool MediaPlayer::openFile(const std::string& filePath) {
    // If a pipeline already exists, clean it up first
    cleanup();
    
    // Create new pipeline
    return buildPipeline(filePath);
}

bool MediaPlayer::buildPipeline(const std::string& filePath) {
    // Create a pipeline using playbin
    pipeline = gst_element_factory_make("playbin", "player");
    if (!pipeline) {
        std::cerr << "Failed to create pipeline!" << std::endl;
        return false;
    }
    
    // Convert file path to URI format
    gchar *uri;
    if (gst_uri_is_valid(filePath.c_str())) {
        uri = g_strdup(filePath.c_str());
    } else {
        uri = gst_filename_to_uri(filePath.c_str(), nullptr);
    }
    
    // Set the URI on the pipeline
    g_object_set(G_OBJECT(pipeline), "uri", uri, nullptr);
    g_free(uri);
    
    // Create bus listener
    bus = gst_element_get_bus(pipeline);
    busWatchId = gst_bus_add_watch(bus, busCallback, this);
    
    // Set to ready state
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to PAUSED state!" << std::endl;
        cleanup();
        return false;
    }
    
    currentState = State::PAUSED;
    return true;
}

bool MediaPlayer::play() {
    if (!pipeline)
        return false;
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to PLAYING state!" << std::endl;
        return false;
    }
    
    currentState = State::PLAYING;
    return true;
}

bool MediaPlayer::pause() {
    if (!pipeline)
        return false;
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Pipeline PAUSED durumuna getirilemedi!" << std::endl;
        return false;
    }
    
    currentState = State::PAUSED;
    return true;
}

bool MediaPlayer::stop() {
    if (!pipeline)
        return false;
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to READY state!" << std::endl;
        return false;
    }
    
    currentState = State::STOPPED;
    return true;
}

bool MediaPlayer::isPlaying() const {
    return currentState == State::PLAYING;
}

bool MediaPlayer::isPaused() const {
    return currentState == State::PAUSED;
}

gint64 MediaPlayer::getPosition() const {
    if (!pipeline)
        return -1;
    
    gint64 position;
    if (!gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
        std::cerr << "Failed to get position!" << std::endl;
        return -1;
    }
    
    return position;
}

gint64 MediaPlayer::getDuration() const {
    if (!pipeline)
        return -1;
    
    gint64 duration;
    if (!gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration)) {
        std::cerr << "Failed to get duration!" << std::endl;
        return -1;
    }
    
    return duration;
}

bool MediaPlayer::seek(gint64 position) {
    if (!pipeline)
        return false;
    
    return gst_element_seek_simple(
        pipeline,
        GST_FORMAT_TIME,
        static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
        position
    );
}

bool MediaPlayer::seekRelative(gint64 offset) {
    gint64 position = getPosition();
    if (position < 0)
        return false;
    
    // Prevent negative positions
    if (offset < 0 && position < -offset)
        position = 0;
    else
        position += offset;
    
    return seek(position);
}

std::string MediaPlayer::getMediaInfo() const {
    if (!pipeline)
        return "No media";
    
    std::string info;
    gint64 duration = getDuration();
    
    if (duration > 0) {
        info += "Duration: " + Utils::formatTime(duration) + "\n";
    }
    
    // Get stream information
    g_object_get(pipeline, "n-video", &duration, nullptr);
    info += "Video streams: " + std::to_string(duration) + "\n";
    
    g_object_get(pipeline, "n-audio", &duration, nullptr);
    info += "Audio streams: " + std::to_string(duration) + "\n";
    
    return info;
}

void MediaPlayer::cleanup() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    
    if (busWatchId > 0) {
        g_source_remove(busWatchId);
        busWatchId = 0;
    }
    
    if (bus) {
        gst_object_unref(bus);
        bus = nullptr;
    }
    
    currentState = State::STOPPED;
}

gboolean MediaPlayer::busCallback(GstBus *bus, GstMessage *message, gpointer data) {
    MediaPlayer *player = static_cast<MediaPlayer*>(data);
    
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(message, &err, &debug);
            
            Utils::handleError(err, debug);
            
            g_free(debug);
            g_error_free(err);
            
            player->stop();
            break;
        }
        case GST_MESSAGE_EOS:
            std::cout << "End of media file reached." << std::endl;
            player->stop();
            break;
        case GST_MESSAGE_STATE_CHANGED:
            // There may be specific state changes we want to monitor
            break;
        default:
            // Other message types
            break;
    }
    
    // Return TRUE to keep the callback active
    return TRUE;
}