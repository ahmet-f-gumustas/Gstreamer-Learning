#ifndef MEDIA_PLAYER_HPP
#define MEDIA_PLAYER_HPP

#include <string>
#include <memory>
#include <gst/gst.h>

class MediaPlayer {
public:
    // Constructors & Destructor
    MediaPlayer();
    ~MediaPlayer();

    // Open file
    bool openFile(const std::string& filePath);
    
    // Basic controls
    bool play();
    bool pause();
    bool stop();
    bool isPlaying() const;
    bool isPaused() const;
    
    // Position functions
    bool seek(gint64 position);
    bool seekRelative(gint64 offset);
    gint64 getPosition() const;
    gint64 getDuration() const;
    
    // Media information
    std::string getMediaInfo() const;
    
private:
    // GStreamer structures
    GstElement *pipeline;
    GstBus *bus;
    guint busWatchId;
    
    // State variables
    enum class State {
        STOPPED,
        PLAYING,
        PAUSED
    };
    
    State currentState;
    
    // Bus callback
    static gboolean busCallback(GstBus *bus, GstMessage *message, gpointer data);
    
    // Pipeline creation
    bool buildPipeline(const std::string& filePath);
    
    // Cleanup
    void cleanup();
};

#endif // MEDIA_PLAYER_HPP