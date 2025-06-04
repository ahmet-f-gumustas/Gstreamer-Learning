#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <string>
#include <thread>

class RTSPCameraServer {
private:
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    guint server_id;
    
    // Low-latency pipeline configuration
    std::string pipeline_str;
    
public:
    RTSPCameraServer(int port = 8554) {
        gst_init(nullptr, nullptr);
        
        // Create RTSP server
        server = gst_rtsp_server_new();
        g_object_set(server, "service", std::to_string(port).c_str(), nullptr);
        
        // Get mount points
        mounts = gst_rtsp_server_get_mount_points(server);
        
        // Create media factory
        factory = gst_rtsp_media_factory_new();
        
        // Configure low-latency H.264 pipeline
        configureLowLatencyPipeline();
        
        // Set pipeline
        gst_rtsp_media_factory_set_launch(factory, pipeline_str.c_str());
        
        // Configure factory for low latency
        configureFactoryForLowLatency();
        
        // Mount the factory at /camera
        gst_rtsp_mount_points_add_factory(mounts, "/camera", factory);
        g_object_unref(mounts);
    }
    
    ~RTSPCameraServer() {
        if (server_id) {
            g_source_remove(server_id);
        }
        g_object_unref(server);
    }
    
    void configureLowLatencyPipeline() {
        // Pipeline optimized for low latency
        // Using v4l2src for camera, x264enc with low-latency settings
        pipeline_str = 
            "( v4l2src device=/dev/video0 ! "
            "video/x-raw,width=640,height=480,framerate=30/1 ! "
            "videoconvert ! "
            "x264enc tune=zerolatency speed-preset=ultrafast "
            "key-int-max=15 intra-refresh=true ! "
            "video/x-h264,profile=baseline ! "
            "h264parse ! "
            "rtph264pay config-interval=1 pt=96 ! "
            "queue max-size-buffers=1 max-size-time=0 max-size-bytes=0 "
            "min-threshold-buffers=0 leaky=downstream ! "
            "udpsink host=127.0.0.1 port=5000 sync=false )";
    }
    
    void configureFactoryForLowLatency() {
        // Enable shared memory for better performance
        gst_rtsp_media_factory_set_shared(factory, TRUE);
        
        // Set low latency mode
        gst_rtsp_media_factory_set_latency(factory, 0);
        
        // Configure buffer mode - no buffering for minimal latency
        gst_rtsp_media_factory_set_buffer_size(factory, 0);
        
        // Set transport mode to UDP for lower latency
        gst_rtsp_media_factory_set_transport_mode(factory, 
            GST_RTSP_TRANSPORT_MODE_PLAY);
            
        // Configure retransmission
        gst_rtsp_media_factory_set_retransmission_time(factory, 0);
    }
    
    bool start() {
        // Attach server to main context
        server_id = gst_rtsp_server_attach(server, nullptr);
        
        if (server_id == 0) {
            std::cerr << "Failed to attach RTSP server" << std::endl;
            return false;
        }
        
        gchar *address = gst_rtsp_server_get_address(server);
        std::cout << "RTSP server started at rtsp://" << address 
                  << ":8554/camera" << std::endl;
        g_free(address);
        
        return true;
    }
    
    void run() {
        GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
        
        std::cout << "Low-latency RTSP camera server is running..." << std::endl;
        std::cout << "Stream URL: rtsp://localhost:8554/camera" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        g_main_loop_run(loop);
        g_main_loop_unref(loop);
    }
    
    // Utility function to set custom pipeline
    void setCustomPipeline(const std::string& pipeline) {
        pipeline_str = pipeline;
        gst_rtsp_media_factory_set_launch(factory, pipeline_str.c_str());
    }
    
    // Get current latency stats
    void printLatencyInfo() {
        std::cout << "\n=== Low-Latency Configuration ===" << std::endl;
        std::cout << "Encoder: x264enc with tune=zerolatency" << std::endl;
        std::cout << "Speed Preset: ultrafast" << std::endl;
        std::cout << "Profile: baseline (lowest complexity)" << std::endl;
        std::cout << "Queue: max-size-buffers=1 (minimal buffering)" << std::endl;
        std::cout << "Transport: UDP (lower latency than TCP)" << std::endl;
        std::cout << "Target Latency: < 250ms" << std::endl;
        std::cout << "================================\n" << std::endl;
    }
};

// Signal handler for clean shutdown
static bool quit = false;
static void sigint_handler(int sig) {
    quit = true;
}

int main(int argc, char *argv[]) {
    // Install signal handler
    signal(SIGINT, sigint_handler);
    
    // Create RTSP server
    RTSPCameraServer server(8554);
    
    // Print configuration info
    server.printLatencyInfo();
    
    // Start server
    if (!server.start()) {
        std::cerr << "Failed to start RTSP server" << std::endl;
        return -1;
    }
    
    // Run main loop
    server.run();
    
    return 0;
}