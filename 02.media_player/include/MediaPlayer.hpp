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

    // Dosya açma
    bool openFile(const std::string& filePath);
    
    // Temel kontroller
    bool play();
    bool pause();
    bool stop();
    bool isPlaying() const;
    bool isPaused() const;
    
    // Konum fonksiyonları
    bool seek(gint64 position);
    bool seekRelative(gint64 offset);
    gint64 getPosition() const;
    gint64 getDuration() const;
    
    // Medya bilgileri
    std::string getMediaInfo() const;
    
private:
    // GStreamer yapıları
    GstElement *pipeline;
    GstBus *bus;
    guint busWatchId;
    
    // Durum değişkenleri
    enum class State {
        STOPPED,
        PLAYING,
        PAUSED
    };
    
    State currentState;
    
    // Bus callback
    static gboolean busCallback(GstBus *bus, GstMessage *message, gpointer data);
    
    // Pipeline oluşturma
    bool buildPipeline(const std::string& filePath);
    
    // Temizleme
    void cleanup();
};

#endif // MEDIA_PLAYER_HPP