/**
 * @file rtsp_streamer.h
 * @brief RTSP server and streaming management class
 *
 * This class provides the necessary functionality for streaming
 * video over the network using GStreamer RTSP Server.
 */

#ifndef RTSP_STREAMER_H
#define RTSP_STREAMER_H

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <map>

/**
 * @brief RTSP stream quality profiles
 */
enum class StreamProfile {
    LOW,        // 480p, 1 Mbps
    MEDIUM,     // 720p, 2.5 Mbps
    HIGH,       // 1080p, 5 Mbps
    ULTRA,      // 4K, 15 Mbps
    CUSTOM      // Custom settings
};

/**
 * @brief RTSP security types
 */
enum class SecurityType {
    NONE,           // No security
    BASIC_AUTH,     // Basic username/password
    DIGEST_AUTH,    // Digest authentication
    TOKEN_AUTH      // Token-based
};

/**
 * @brief RTSP stream configuration
 */
struct RTSPConfig {
    // Server settings
    std::string address = "0.0.0.0";        // Listen address
    int port = 8554;                        // RTSP port
    std::string mount_point = "/live";      // Mount point

    // Video settings
    StreamProfile profile = StreamProfile::MEDIUM;
    int width = 1280;
    int height = 720;
    int framerate = 30;
    int bitrate = 2500000;                  // 2.5 Mbps
    std::string encoder = "x264enc";        // Video encoder

    // Audio settings
    bool enable_audio = true;
    std::string audio_encoder = "opus";
    int audio_bitrate = 128000;             // 128 kbps
    int audio_channels = 2;
    int audio_samplerate = 48000;

    // Security settings
    SecurityType security = SecurityType::NONE;
    std::string username = "";
    std::string password = "";
    std::vector<std::string> allowed_ips;   // Allowed IP list

    // Performance settings
    int max_clients = 10;                   // Maximum client count
    int buffer_size = 200;                  // Buffer size (frames)
    int latency = 200;                      // Target latency (ms)
    bool enable_rtcp = true;                // RTCP enabled

    // Multicast settings
    bool enable_multicast = false;
    std::string multicast_address = "224.1.1.1";
    int multicast_port = 5000;
    int multicast_ttl = 1;

    // Recording settings
    bool enable_recording = false;
    std::string record_path = "/tmp/rtsp_recordings";
    int max_record_duration = 3600;         // Maximum recording duration (seconds)
};

/**
 * @brief Client information
 */
struct ClientInfo {
    std::string id;                         // Client ID
    std::string address;                    // IP address
    int port;                               // Port number
    std::chrono::time_point<std::chrono::steady_clock> connect_time;
    guint64 bytes_sent = 0;                 // Data sent
    guint64 frames_sent = 0;                // Frames sent
    double bandwidth = 0.0;                 // Current bandwidth (Mbps)
    bool is_playing = false;                // Playback state
};

/**
 * @brief RTSP server statistics
 */
struct RTSPStats {
    int active_clients = 0;                 // Active client count
    guint64 total_bytes_sent = 0;           // Total data sent
    guint64 total_frames_sent = 0;          // Total frames sent
    double average_bandwidth = 0.0;         // Average bandwidth
    int total_connections = 0;              // Total connection count
    std::chrono::duration<double> uptime;   // Uptime
};

/**
 * @brief Client connection callback
 * @param client_info Client information
 * @param connected true: connected, false: disconnected
 */
using ClientCallback = std::function<void(const ClientInfo&, bool)>;

/**
 * @brief RTSP streamer class
 */
class RTSPStreamer {
public:
    /**
     * @brief Constructor
     * @param config RTSP configuration
     */
    explicit RTSPStreamer(const RTSPConfig& config = RTSPConfig());

    /**
     * @brief Destructor
     */
    ~RTSPStreamer();

    /**
     * @brief Starts the RTSP server
     * @return true if successful
     */
    bool start();

    /**
     * @brief Stops the RTSP server
     */
    void stop();

    /**
     * @brief Checks if server is running
     * @return true if running
     */
    bool isRunning() const { return is_running_; }

    /**
     * @brief Sets the video source element
     * @param source Source element
     */
    void setVideoSource(GstElement* source);

    /**
     * @brief Sets the audio source element
     * @param source Source element
     */
    void setAudioSource(GstElement* source);

    /**
     * @brief Updates configuration
     * @param config New configuration
     */
    void updateConfig(const RTSPConfig& config);

    /**
     * @brief Returns current configuration
     * @return RTSP configuration
     */
    RTSPConfig getConfig() const;

    /**
     * @brief Returns the stream URL
     * @return RTSP URL
     */
    std::string getStreamURL() const;

    /**
     * @brief Returns the list of active clients
     * @return Client information
     */
    std::vector<ClientInfo> getClients() const;

    /**
     * @brief Disconnects a client
     * @param client_id Client ID
     * @return true if successful
     */
    bool disconnectClient(const std::string& client_id);

    /**
     * @brief Returns server statistics
     * @return Statistics
     */
    RTSPStats getStats() const;

    /**
     * @brief Sets the client callback
     * @param callback For connection/disconnection events
     */
    void setClientCallback(ClientCallback callback);

    /**
     * @brief Adds a new mount point
     * @param path Mount path
     * @param pipeline Pipeline definition
     * @return true if successful
     */
    bool addMountPoint(const std::string& path, const std::string& pipeline);

    /**
     * @brief Removes a mount point
     * @param path Mount path
     * @return true if successful
     */
    bool removeMountPoint(const std::string& path);

    /**
     * @brief Updates security settings
     * @param type Security type
     * @param username Username
     * @param password Password
     */
    void setSecurity(SecurityType type,
                     const std::string& username = "",
                     const std::string& password = "");

    /**
     * @brief Adds IP-based access control
     * @param allowed_ips Allowed IP list
     */
    void setIPFilter(const std::vector<std::string>& allowed_ips);

    /**
     * @brief Record the stream
     * @param enable true: start recording, false: stop
     * @param filename Recording file name
     * @return true if successful
     */
    bool recordStream(bool enable, const std::string& filename = "");

    /**
     * @brief Take a snapshot
     * @param filename File name
     * @return true if successful
     */
    bool takeSnapshot(const std::string& filename);

private:
    /**
     * @brief Creates the RTSP server
     * @return true if successful
     */
    bool createServer();

    /**
     * @brief Creates the media factory
     * @return Media factory pointer
     */
    GstRTSPMediaFactory* createMediaFactory();

    /**
     * @brief Builds the pipeline string
     * @return GStreamer pipeline definition
     */
    std::string buildPipelineString();

    /**
     * @brief Applies security settings
     */
    void applySecurity();

    /**
     * @brief Client connected callback
     * @param server RTSP server
     * @param client Client object
     * @param user_data User data
     */
    static void onClientConnected(GstRTSPServer* server,
                                  GstRTSPClient* client,
                                  gpointer user_data);

    /**
     * @brief Client disconnected callback
     * @param server RTSP server
     * @param client Client object
     * @param user_data User data
     */
    static void onClientDisconnected(GstRTSPServer* server,
                                    GstRTSPClient* client,
                                    gpointer user_data);

    /**
     * @brief Media configuration callback
     * @param factory Media factory
     * @param media Media object
     * @param user_data User data
     */
    static void onMediaConfigure(GstRTSPMediaFactory* factory,
                                GstRTSPMedia* media,
                                gpointer user_data);

    /**
     * @brief Stream state changed callback
     * @param media Media object
     * @param state New state
     * @param user_data User data
     */
    static void onMediaStateChanged(GstRTSPMedia* media,
                                   GstState state,
                                   gpointer user_data);

    /**
     * @brief Create client info
     * @param client RTSP client object
     * @return Client information
     */
    ClientInfo createClientInfo(GstRTSPClient* client);

    /**
     * @brief Server main loop
     */
    void serverMainLoop();

    /**
     * @brief Update statistics
     */
    void updateStats();

    // Member variables
    RTSPConfig config_;                         // Server configuration

    // RTSP Server objects
    GstRTSPServer* server_ = nullptr;           // Main server
    GstRTSPMediaFactory* factory_ = nullptr;    // Media factory
    GstRTSPAuth* auth_ = nullptr;              // Authorization
    GstRTSPToken* token_ = nullptr;            // Security token
    GstRTSPMountPoints* mounts_ = nullptr;      // Mount points

    // Video/Audio sources
    GstElement* video_source_ = nullptr;        // Video source
    GstElement* audio_source_ = nullptr;        // Audio source

    // Thread and synchronization
    std::unique_ptr<std::thread> server_thread_; // Server thread
    GMainLoop* main_loop_ = nullptr;            // GLib main loop
    std::atomic<bool> is_running_{false};       // Running state

    // Client management
    std::map<std::string, ClientInfo> clients_; // Active clients
    mutable std::mutex clients_mutex_;          // Client mutex
    ClientCallback client_callback_;            // Client callback

    // Statistics
    RTSPStats stats_;                          // Server statistics
    mutable std::mutex stats_mutex_;           // Statistics mutex
    std::chrono::time_point<std::chrono::steady_clock> start_time_;

    // For recording
    GstElement* recording_bin_ = nullptr;       // Recording element
    bool is_recording_ = false;                 // Recording state
    std::string record_filename_;               // Recording file name

    // Multicast group
    GstRTSPAddressPool* address_pool_ = nullptr; // Address pool
};

#endif // RTSP_STREAMER_H
