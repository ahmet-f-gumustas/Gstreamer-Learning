#ifndef VIDEO_MOSAIC_H
#define VIDEO_MOSAIC_H

#include <gst/gst.h>
#include <vector>
#include <memory>
#include <string>

class InputManager;
class MosaicLayout;

class VideoMosaic {
public:
    VideoMosaic();
    ~VideoMosaic();

    bool initialize(const std::string& config_file);
    bool addVideoSource(const std::string& uri, const std::string& name);
    bool start();
    void stop();
    bool isRunning() const { return is_running_; }

private:
    GstElement* pipeline_;
    GstElement* compositor_;
    GstElement* video_sink_;
    
    std::unique_ptr<InputManager> input_manager_;
    std::unique_ptr<MosaicLayout> layout_manager_;
    
    bool is_running_;
    GMainLoop* main_loop_;
    
    bool createPipeline();
    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer data);
};

#endif // VIDEO_MOSAIC_H