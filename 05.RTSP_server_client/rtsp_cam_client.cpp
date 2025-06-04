#include <gst/gst.h>
#include <gst/rtsp/gstrtsptransport.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <iomanip>
#include <thread>

class RTSPCameraClient {
private:
    GstElement *pipeline;
    GstElement *rtspsrc;
    GstElement *decodebin;
    GstElement *videoconvert;
    GstElement *fpsdisplaysink;
    GstElement *queue;
    
    std::string rtsp_url;
    std::vector<double> latency_measurements;
    std::chrono::steady_clock::time_point start_time;
    
    // Latency measurement variables
    guint64 stream_time_start;
    guint64 stream_time_end;
    
public:
    RTSPCameraClient(const std::string& url) : rtsp_url(url) {
        gst_init(nullptr, nullptr);
        
        // Create pipeline elements
        pipeline = gst_pipeline_new("rtsp-client");
        rtspsrc = gst_element_factory_make("rtspsrc", "source");
        decodebin = gst_element_factory_make("decodebin", "decoder");
        videoconvert = gst_element_factory_make("videoconvert", "converter");
        queue = gst_element_factory_make("queue", "queue");
        fpsdisplaysink = gst_element_factory_make("fpsdisplaysink", "sink");
        
        if (!pipeline || !rtspsrc || !decodebin || !videoconvert || !queue || !fpsdisplaysink) {
            std::cerr << "Failed to create GStreamer elements" << std::endl;
            return;
        }
        
        // Configure elements for low latency
        configureLowLatency();
        
        // Build pipeline
        gst_bin_add_many(GST_BIN(pipeline), rtspsrc, decodebin, videoconvert, 
                         queue, fpsdisplaysink, nullptr);
        
        // Link elements (rtspsrc and decodebin will be linked dynamically)
        if (!gst_element_link_many(videoconvert, queue, fpsdisplaysink, nullptr)) {
            std::cerr << "Failed to link elements" << std::endl;
            return;
        }
        
        // Connect signals
        g_signal_connect(rtspsrc, "pad-added", G_CALLBACK(onPadAdded), decodebin);
        g_signal_connect(decodebin, "pad-added", G_CALLBACK(onDecodebinPadAdded), videoconvert);
        
        // Set up bus watch for messages
        GstBus *bus = gst_element_get_bus(pipeline);
        gst_bus_add_watch(bus, busCallback, this);
        gst_object_unref(bus);
    }
    
    ~RTSPCameraClient() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
        }
    }
    
    void configureLowLatency() {
        // Configure RTSP source for low latency
        g_object_set(rtspsrc,
            "location", rtsp_url.c_str(),
            "latency", 0,  // Minimal latency
            "buffer-mode", 0,  // No buffering
            "udp-buffer-size", 524288,  // 512KB UDP buffer
            "do-rtcp", TRUE,
            "protocols", 0x00000004,  // Use UDP for lower latency (GST_RTSP_LOWER_TRANS_UDP = 4)
            nullptr);
        
        // Configure queue for minimal buffering
        g_object_set(queue,
            "max-size-buffers", 1,
            "max-size-time", (guint64)0,
            "max-size-bytes", 0,
            "min-threshold-buffers", 0,
            "leaky", 2,  // Leak downstream (drop old buffers)
            nullptr);
        
        // Configure fpsdisplaysink
        g_object_set(fpsdisplaysink,
            "text-overlay", TRUE,
            "video-sink", "autovideosink",
            "sync", FALSE,  // Disable sync for lower latency
            nullptr);
    }
    
    bool start() {
        std::cout << "Starting RTSP client..." << std::endl;
        std::cout << "Connecting to: " << rtsp_url << std::endl;
        
        start_time = std::chrono::steady_clock::now();
        
        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            std::cerr << "Failed to start pipeline" << std::endl;
            return false;
        }
        
        return true;
    }
    
    void run() {
        GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
        
        std::cout << "RTSP client is running..." << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        // Start latency measurement timer
        g_timeout_add(1000, latencyMeasurementCallback, this);
        
        g_main_loop_run(loop);
        g_main_loop_unref(loop);
    }
    
    void measureLatency() {
        GstQuery *query = gst_query_new_latency();
        
        if (gst_element_query(pipeline, query)) {
            gboolean live;
            GstClockTime min_latency, max_latency;
            
            gst_query_parse_latency(query, &live, &min_latency, &max_latency);
            
            double latency_ms = min_latency / 1000000.0;  // Convert to milliseconds
            latency_measurements.push_back(latency_ms);
            
            std::cout << "Current latency: " << std::fixed << std::setprecision(2) 
                      << latency_ms << " ms";
            
            if (latency_ms < 250) {
                std::cout << " ✓ (Target achieved!)";
            } else {
                std::cout << " ✗ (Above target)";
            }
            std::cout << std::endl;
        }
        
        gst_query_unref(query);
    }
    
    void saveLatencyReport() {
        std::ofstream report("latency_plot.md");
        
        report << "# RTSP Low-Latency Streaming Report\n\n";
        report << "## Configuration\n";
        report << "- **Stream URL**: " << rtsp_url << "\n";
        report << "- **Target Latency**: < 250ms\n";
        report << "- **Transport Protocol**: UDP\n";
        report << "- **Encoder Settings**: x264enc tune=zerolatency\n";
        report << "- **Buffer Configuration**: max-size-buffers=1\n\n";
        
        report << "## Latency Measurements\n\n";
        report << "| Time (s) | Latency (ms) | Status |\n";
        report << "|----------|--------------|--------|\n";
        
        double sum = 0;
        double min_latency = 999999;
        double max_latency = 0;
        
        for (size_t i = 0; i < latency_measurements.size(); ++i) {
            double latency = latency_measurements[i];
            sum += latency;
            min_latency = std::min(min_latency, latency);
            max_latency = std::max(max_latency, latency);
            
            report << "| " << i << " | " << std::fixed << std::setprecision(2) 
                   << latency << " | ";
            
            if (latency < 250) {
                report << "✓ Pass |";
            } else {
                report << "✗ Fail |";
            }
            report << "\n";
        }
        
        double avg_latency = sum / latency_measurements.size();
        
        report << "\n## Summary Statistics\n";
        report << "- **Average Latency**: " << std::fixed << std::setprecision(2) 
               << avg_latency << " ms\n";
        report << "- **Minimum Latency**: " << min_latency << " ms\n";
        report << "- **Maximum Latency**: " << max_latency << " ms\n";
        report << "- **Target Achievement**: ";
        
        if (avg_latency < 250) {
            report << "✓ **SUCCESS** - Average latency below 250ms target\n";
        } else {
            report << "✗ **FAILED** - Average latency above 250ms target\n";
        }
        
        report << "\n## Latency Graph (ASCII)\n```\n";
        
        // Simple ASCII graph
        const int graph_height = 10;
        const int graph_width = std::min(static_cast<int>(latency_measurements.size()), 50);
        
        for (int h = graph_height; h >= 0; --h) {
            double threshold = (h * max_latency) / graph_height;
            report << std::setw(6) << std::fixed << std::setprecision(0) 
                   << threshold << " |";
            
            for (int w = 0; w < graph_width; ++w) {
                if (w < latency_measurements.size()) {
                    if (latency_measurements[w] >= threshold) {
                        report << "█";
                    } else {
                        report << " ";
                    }
                } else {
                    report << " ";
                }
            }
            report << "\n";
        }
        
        report << "       +";
        for (int w = 0; w < graph_width; ++w) {
            report << "-";
        }
        report << "\n        Time (seconds)\n```\n";
        
        report.close();
        std::cout << "\nLatency report saved to latency_plot.md" << std::endl;
    }
    
private:
    static void onPadAdded(GstElement *src, GstPad *pad, gpointer data) {
        GstElement *decodebin = GST_ELEMENT(data);
        GstPad *sink_pad = gst_element_get_static_pad(decodebin, "sink");
        
        if (!gst_pad_is_linked(sink_pad)) {
            gst_pad_link(pad, sink_pad);
        }
        
        gst_object_unref(sink_pad);
    }
    
    static void onDecodebinPadAdded(GstElement *decodebin, GstPad *pad, gpointer data) {
        GstElement *videoconvert = GST_ELEMENT(data);
        GstPad *sink_pad = gst_element_get_static_pad(videoconvert, "sink");
        
        GstCaps *caps = gst_pad_get_current_caps(pad);
        if (caps) {
            GstStructure *str = gst_caps_get_structure(caps, 0);
            const gchar *name = gst_structure_get_name(str);
            
            if (g_str_has_prefix(name, "video/")) {
                if (!gst_pad_is_linked(sink_pad)) {
                    gst_pad_link(pad, sink_pad);
                }
            }
            gst_caps_unref(caps);
        }
        
        gst_object_unref(sink_pad);
    }
    
    static gboolean busCallback(GstBus *bus, GstMessage *msg, gpointer data) {
        RTSPCameraClient *client = static_cast<RTSPCameraClient*>(data);
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR: {
                GError *err;
                gchar *debug;
                gst_message_parse_error(msg, &err, &debug);
                std::cerr << "Error: " << err->message << std::endl;
                g_error_free(err);
                g_free(debug);
                break;
            }
            case GST_MESSAGE_EOS:
                std::cout << "End of stream" << std::endl;
                break;
            case GST_MESSAGE_LATENCY:
                // Reconfigure latency
                gst_bin_recalculate_latency(GST_BIN(client->pipeline));
                break;
            default:
                break;
        }
        
        return TRUE;
    }
    
    static gboolean latencyMeasurementCallback(gpointer data) {
        RTSPCameraClient *client = static_cast<RTSPCameraClient*>(data);
        client->measureLatency();
        return TRUE;
    }
};

int main(int argc, char *argv[]) {
    std::string rtsp_url = "rtsp://localhost:8554/camera";
    
    if (argc > 1) {
        rtsp_url = argv[1];
    }
    
    RTSPCameraClient client(rtsp_url);
    
    if (!client.start()) {
        return -1;
    }
    
    // Run for 30 seconds to collect latency data
    std::thread([&client]() {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        client.saveLatencyReport();
        exit(0);
    }).detach();
    
    client.run();
    
    return 0;
}