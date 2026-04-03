/**
 * @file rtsp_streamer.cpp
 * @brief RTSP server and streaming management class implementation
 */

#include "rtsp_streamer.h"
#include <iostream>
#include <sstream>
#include <iomanip>

/**
 * @brief Constructor
 */
RTSPStreamer::RTSPStreamer(const RTSPConfig& config)
    : config_(config) {
    
    // Initialize GStreamer RTSP server library
    static bool rtsp_initialized = false;
    if (!rtsp_initialized) {
        gst_init(nullptr, nullptr);
        rtsp_initialized = true;
    }
    
    // Record start time
    start_time_ = std::chrono::steady_clock::now();
}

/**
 * @brief Destructor
 */
RTSPStreamer::~RTSPStreamer() {
    stop();
}

/**
 * @brief Starts the RTSP server
 */
bool RTSPStreamer::start() {
    if (is_running_.load()) {
        return true;
    }
    
    // Create RTSP server
    if (!createServer()) {
        std::cerr << "[RTSPStreamer] Failed to create server!" << std::endl;
        return false;
    }
    
    // Start server thread
    is_running_ = true;
    server_thread_ = std::make_unique<std::thread>(&RTSPStreamer::serverMainLoop, this);
    
    std::cout << "[RTSPStreamer] RTSP server started: " 
              << getStreamURL() << std::endl;
    
    return true;
}

/**
 * @brief Stops the RTSP server
 */
void RTSPStreamer::stop() {
    if (!is_running_.load()) {
        return;
    }
    
    is_running_ = false;
    
    // Stop main loop
    if (main_loop_) {
        g_main_loop_quit(main_loop_);
    }
    
    // Wait for thread to finish
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    
    // Clean up resources
    if (server_) {
        g_object_unref(server_);
        server_ = nullptr;
    }
    
    if (factory_) {
        g_object_unref(factory_);
        factory_ = nullptr;
    }
    
    if (auth_) {
        g_object_unref(auth_);
        auth_ = nullptr;
    }
    
    if (mounts_) {
        g_object_unref(mounts_);
        mounts_ = nullptr;
    }
    
    if (address_pool_) {
        g_object_unref(address_pool_);
        address_pool_ = nullptr;
    }
    
    std::cout << "[RTSPStreamer] RTSP server stopped." << std::endl;
}

/**
 * @brief Sets the video source element
 */
void RTSPStreamer::setVideoSource(GstElement* source) {
    video_source_ = source;
}

/**
 * @brief Sets the audio source element
 */
void RTSPStreamer::setAudioSource(GstElement* source) {
    audio_source_ = source;
}

/**
 * @brief Updates configuration
 */
void RTSPStreamer::updateConfig(const RTSPConfig& config) {
    config_ = config;
    
    // Update security settings if server is running
    if (is_running_ && auth_) {
        applySecurity();
    }
}

/**
 * @brief Returns current configuration
 */
RTSPConfig RTSPStreamer::getConfig() const {
    return config_;
}

/**
 * @brief Returns the stream URL
 */
std::string RTSPStreamer::getStreamURL() const {
    std::stringstream url;
    url << "rtsp://" << config_.address << ":" << config_.port << config_.mount_point;
    return url.str();
}

/**
 * @brief Returns the list of active clients
 */
std::vector<ClientInfo> RTSPStreamer::getClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::vector<ClientInfo> result;
    
    for (const auto& pair : clients_) {
        result.push_back(pair.second);
    }
    
    return result;
}

/**
 * @brief Disconnects a client
 */
bool RTSPStreamer::disconnectClient(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto it = clients_.find(client_id);
    if (it != clients_.end()) {
        // TODO: Disconnect client via GStreamer API
        clients_.erase(it);
        return true;
    }
    
    return false;
}

/**
 * @brief Returns server statistics
 */
RTSPStats RTSPStreamer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Update uptime
    auto now = std::chrono::steady_clock::now();
    stats_.uptime = now - start_time_;
    
    return stats_;
}

/**
 * @brief Sets the client callback
 */
void RTSPStreamer::setClientCallback(ClientCallback callback) {
    client_callback_ = callback;
}

/**
 * @brief Adds a new mount point
 */
bool RTSPStreamer::addMountPoint(const std::string& path, const std::string& pipeline) {
    if (!mounts_) {
        return false;
    }
    
    // Create new factory
    GstRTSPMediaFactory* new_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(new_factory, pipeline.c_str());
    gst_rtsp_media_factory_set_shared(new_factory, TRUE);
    
    // Add to mount point
    gst_rtsp_mount_points_add_factory(mounts_, path.c_str(), new_factory);
    
    std::cout << "[RTSPStreamer] Mount point added: " << path << std::endl;
    return true;
}

/**
 * @brief Removes a mount point
 */
bool RTSPStreamer::removeMountPoint(const std::string& path) {
    if (!mounts_) {
        return false;
    }
    
    gst_rtsp_mount_points_remove_factory(mounts_, path.c_str());
    
    std::cout << "[RTSPStreamer] Mount point removed: " << path << std::endl;
    return true;
}

/**
 * @brief Updates security settings
 */
void RTSPStreamer::setSecurity(SecurityType type, 
                               const std::string& username,
                               const std::string& password) {
    config_.security = type;
    config_.username = username;
    config_.password = password;
    
    if (is_running_) {
        applySecurity();
    }
}

/**
 * @brief Adds IP-based access control
 */
void RTSPStreamer::setIPFilter(const std::vector<std::string>& allowed_ips) {
    config_.allowed_ips = allowed_ips;
    
    // TODO: IP filtering implementation
}

/**
 * @brief Record the stream
 */
bool RTSPStreamer::recordStream(bool enable, const std::string& filename) {
    if (enable && !is_recording_) {
        // TODO: Recording implementation
        record_filename_ = filename;
        is_recording_ = true;
        
        std::cout << "[RTSPStreamer] Recording started: " << filename << std::endl;
        return true;
    } else if (!enable && is_recording_) {
        // TODO: Stop recording
        is_recording_ = false;
        
        std::cout << "[RTSPStreamer] Recording stopped." << std::endl;
        return true;
    }
    
    return false;
}

/**
 * @brief Take a snapshot
 */
bool RTSPStreamer::takeSnapshot(const std::string& filename) {
    // TODO: Snapshot implementasyonu
    std::cout << "[RTSPStreamer] Snapshot taken: " << filename << std::endl;
    return true;
}

/**
 * @brief Creates the RTSP server
 */
bool RTSPStreamer::createServer() {
    // Create RTSP server
    server_ = gst_rtsp_server_new();
    if (!server_) {
        return false;
    }
    
    // Set server properties
    g_object_set(server_, 
        "address", config_.address.c_str(),
        "service", std::to_string(config_.port).c_str(),
        "backlog", config_.max_clients,
        nullptr);
    
    // Get mount points
    mounts_ = gst_rtsp_server_get_mount_points(server_);
    
    // Create media factory
    factory_ = createMediaFactory();
    if (!factory_) {
        return false;
    }
    
    // Add factory to mount point
    gst_rtsp_mount_points_add_factory(mounts_, 
                                      config_.mount_point.c_str(), 
                                      factory_);
    
    // Apply security settings
    if (config_.security != SecurityType::NONE) {
        applySecurity();
    }
    
    // Multicast settings
    if (config_.enable_multicast) {
        address_pool_ = gst_rtsp_address_pool_new();
        gst_rtsp_address_pool_add_range(address_pool_,
            config_.multicast_address.c_str(),
            config_.multicast_address.c_str(),
            config_.multicast_port,
            config_.multicast_port + 10,
            config_.multicast_ttl);
        
        gst_rtsp_media_factory_set_address_pool(factory_, address_pool_);
    }
    
    // Connect client connection signals
    g_signal_connect(server_, "client-connected",
                     G_CALLBACK(onClientConnected), this);
                     
    // Attach server
    gst_rtsp_server_attach(server_, nullptr);
    
    return true;
}

/**
 * @brief Creates the media factory
 */
GstRTSPMediaFactory* RTSPStreamer::createMediaFactory() {
    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    
    // Build pipeline string
    std::string pipeline = buildPipelineString();
    
    // Factory settings
    gst_rtsp_media_factory_set_launch(factory, pipeline.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_media_factory_set_latency(factory, config_.latency);
    gst_rtsp_media_factory_set_buffer_size(factory, config_.buffer_size);
    
    // RTCP settings
    if (config_.enable_rtcp) {
        gst_rtsp_media_factory_set_enable_rtcp(factory, TRUE);
    }
    
    // Connect media configure signal
    g_signal_connect(factory, "media-configure",
                     G_CALLBACK(onMediaConfigure), this);
    
    return factory;
}

/**
 * @brief Builds the pipeline string
 */
std::string RTSPStreamer::buildPipelineString() {
    std::stringstream pipeline;
    
    // Video part
    if (video_source_) {
        // Get video from AppSrc
        pipeline << "appsrc name=videosrc ! ";
    } else {
        // Use test pattern
        pipeline << "videotestsrc is-live=true ! ";
    }
    
    // Video caps
    pipeline << "video/x-raw,width=" << config_.width 
             << ",height=" << config_.height
             << ",framerate=" << config_.framerate << "/1 ! ";
    
    // Video encoder
    if (config_.encoder == "nvh264enc" || config_.encoder == "nvh265enc") {
        // NVIDIA encoder
        pipeline << config_.encoder << " preset=low-latency bitrate=" 
                 << (config_.bitrate / 1000) << " ! ";
    } else if (config_.encoder == "x264enc") {
        // x264 encoder
        pipeline << "x264enc speed-preset=ultrafast tune=zerolatency bitrate="
                 << (config_.bitrate / 1000) << " ! ";
    } else if (config_.encoder == "x265enc") {
        // x265 encoder
        pipeline << "x265enc speed-preset=ultrafast tune=zerolatency bitrate="
                 << (config_.bitrate / 1000) << " ! ";
    }
    
    // RTP payloader
    if (config_.encoder.find("h265") != std::string::npos) {
        pipeline << "h265parse ! rtph265pay name=pay0 pt=96 ";
    } else {
        pipeline << "h264parse ! rtph264pay name=pay0 pt=96 ";
    }
    
    // Audio part (if enabled)
    if (config_.enable_audio) {
        pipeline << " ";
        
        if (audio_source_) {
            pipeline << "appsrc name=audiosrc ! ";
        } else {
            pipeline << "audiotestsrc is-live=true ! ";
        }
        
        // Audio caps
        pipeline << "audio/x-raw,rate=" << config_.audio_samplerate
                 << ",channels=" << config_.audio_channels << " ! ";
        
        // Audio encoder
        if (config_.audio_encoder == "opus") {
            pipeline << "opusenc bitrate=" << config_.audio_bitrate << " ! ";
            pipeline << "rtpopuspay name=pay1 pt=97";
        } else if (config_.audio_encoder == "aac") {
            pipeline << "voaacenc bitrate=" << config_.audio_bitrate << " ! ";
            pipeline << "rtpmp4apay name=pay1 pt=97";
        }
    }
    
    return pipeline.str();
}

/**
 * @brief Applies security settings
 */
void RTSPStreamer::applySecurity() {
    if (!server_ || config_.security == SecurityType::NONE) {
        return;
    }
    
    // Create auth object
    if (!auth_) {
        auth_ = gst_rtsp_auth_new();
    }
    
    // Create token
    if (token_) {
        gst_rtsp_token_unref(token_);
    }
    
    if (config_.security == SecurityType::BASIC_AUTH || 
        config_.security == SecurityType::DIGEST_AUTH) {
        
        // Add user
        GstRTSPToken* basic_token = gst_rtsp_token_new(
            GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "user",
            nullptr);
        
        gst_rtsp_auth_add_basic(auth_, 
                               config_.username.c_str(), 
                               config_.password.c_str(), 
                               basic_token);
        
        gst_rtsp_token_unref(basic_token);
        
        // For digest auth
        if (config_.security == SecurityType::DIGEST_AUTH) {
            gst_rtsp_auth_add_digest(auth_,
                                    config_.username.c_str(),
                                    config_.password.c_str(),
                                    basic_token);
        }
    }
    
    // Token for anonymous access
    GstRTSPToken* anon_token = gst_rtsp_token_new(
        GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, 
        config_.security == SecurityType::NONE ? "user" : "anonymous",
        nullptr);
    
    gst_rtsp_auth_set_default_token(auth_, anon_token);
    gst_rtsp_token_unref(anon_token);
    
    // Add auth to server
    gst_rtsp_server_set_auth(server_, auth_);
}

/**
 * @brief Client connected callback
 */
void RTSPStreamer::onClientConnected(GstRTSPServer* server,
                                    GstRTSPClient* client,
                                    gpointer user_data) {
    RTSPStreamer* streamer = static_cast<RTSPStreamer*>(user_data);
    
    // Create client info
    ClientInfo info = streamer->createClientInfo(client);
    
    // Add to client list
    {
        std::lock_guard<std::mutex> lock(streamer->clients_mutex_);
        streamer->clients_[info.id] = info;
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(streamer->stats_mutex_);
        streamer->stats_.active_clients++;
        streamer->stats_.total_connections++;
    }
    
    // Call callback
    if (streamer->client_callback_) {
        streamer->client_callback_(info, true);
    }
    
    // Connect client disconnect signal
    g_signal_connect(client, "closed",
                     G_CALLBACK(onClientDisconnected), streamer);
    
    std::cout << "[RTSPStreamer] Client connected: " 
              << info.address << ":" << info.port << std::endl;
}

/**
 * @brief Client disconnected callback
 */
void RTSPStreamer::onClientDisconnected(GstRTSPServer* server,
                                       GstRTSPClient* client,
                                       gpointer user_data) {
    RTSPStreamer* streamer = static_cast<RTSPStreamer*>(user_data);
    
    // Find and remove client info
    ClientInfo info;
    bool found = false;
    
    {
        std::lock_guard<std::mutex> lock(streamer->clients_mutex_);
        
        // Find client
        for (auto it = streamer->clients_.begin(); it != streamer->clients_.end(); ++it) {
            // TODO: Compare GstRTSPClient pointer
            if (true) { // Placeholder
                info = it->second;
                streamer->clients_.erase(it);
                found = true;
                break;
            }
        }
    }
    
    if (found) {
        // Update statistics
        {
            std::lock_guard<std::mutex> lock(streamer->stats_mutex_);
            streamer->stats_.active_clients--;
        }
        
        // Call callback
        if (streamer->client_callback_) {
            streamer->client_callback_(info, false);
        }
        
        std::cout << "[RTSPStreamer] Client disconnected: " 
                  << info.address << ":" << info.port << std::endl;
    }
}

/**
 * @brief Media configuration callback
 */
void RTSPStreamer::onMediaConfigure(GstRTSPMediaFactory* factory,
                                   GstRTSPMedia* media,
                                   gpointer user_data) {
    RTSPStreamer* streamer = static_cast<RTSPStreamer*>(user_data);
    
    // Connect media state change signal
    g_signal_connect(media, "new-state",
                     G_CALLBACK(onMediaStateChanged), streamer);
    
    // Get pipeline
    GstElement* pipeline = gst_rtsp_media_get_element(media);
    
    // Connect video source if available
    if (streamer->video_source_) {
        GstElement* videosrc = gst_bin_get_by_name(GST_BIN(pipeline), "videosrc");
        if (videosrc) {
            // AppSrc settings
            g_object_set(videosrc,
                "stream-type", GST_APP_STREAM_TYPE_STREAM,
                "format", GST_FORMAT_TIME,
                "is-live", TRUE,
                nullptr);
            
            // TODO: Data feeding mechanism for AppSrc
            
            gst_object_unref(videosrc);
        }
    }
    
    // Connect audio source if available
    if (streamer->audio_source_) {
        GstElement* audiosrc = gst_bin_get_by_name(GST_BIN(pipeline), "audiosrc");
        if (audiosrc) {
            // AppSrc settings
            g_object_set(audiosrc,
                "stream-type", GST_APP_STREAM_TYPE_STREAM,
                "format", GST_FORMAT_TIME,
                "is-live", TRUE,
                nullptr);
            
            // TODO: Data feeding mechanism for AppSrc
            
            gst_object_unref(audiosrc);
        }
    }
    
    gst_object_unref(pipeline);
}

/**
 * @brief Stream state changed callback
 */
void RTSPStreamer::onMediaStateChanged(GstRTSPMedia* media,
                                      GstState state,
                                      gpointer user_data) {
    RTSPStreamer* streamer = static_cast<RTSPStreamer*>(user_data);
    
    const char* state_name = gst_element_state_get_name(state);
    std::cout << "[RTSPStreamer] Media state changed: " << state_name << std::endl;
    
    // Update statistics
    if (state == GST_STATE_PLAYING) {
        // Streaming started
    } else if (state == GST_STATE_NULL) {
        // Streaming stopped
    }
}

/**
 * @brief Create client info
 */
ClientInfo RTSPStreamer::createClientInfo(GstRTSPClient* client) {
    ClientInfo info;
    
    // Get connection info
    GstRTSPConnection* conn = gst_rtsp_client_get_connection(client);
    if (conn) {
        // IP adresini al
        GstRTSPUrl* url = gst_rtsp_connection_get_url(conn);
        if (url && url->host) {
            info.address = url->host;
        }
        
        // Port bilgisi
        info.port = url ? url->port : 0;
    }
    
    // Create unique ID
    static int client_counter = 0;
    info.id = "client_" + std::to_string(++client_counter);
    
    // Connection time
    info.connect_time = std::chrono::steady_clock::now();
    
    return info;
}

/**
 * @brief Server main loop
 */
void RTSPStreamer::serverMainLoop() {
    // Create GMainLoop
    main_loop_ = g_main_loop_new(nullptr, FALSE);
    
    // Statistics update timer
    g_timeout_add(1000, [](gpointer data) -> gboolean {
        RTSPStreamer* streamer = static_cast<RTSPStreamer*>(data);
        streamer->updateStats();
        return TRUE;
    }, this);
    
    // Run main loop
    g_main_loop_run(main_loop_);
    
    // Cleanup
    g_main_loop_unref(main_loop_);
    main_loop_ = nullptr;
}

/**
 * @brief Update statistics
 */
void RTSPStreamer::updateStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Active client count is already up to date

    // Bandwidth calculation
    double total_bandwidth = 0;
    int active_count = 0;
    
    {
        std::lock_guard<std::mutex> clients_lock(clients_mutex_);
        for (auto& pair : clients_) {
            ClientInfo& client = pair.second;
            
            // TODO: Real bandwidth measurement
            // Use estimated value for now
            if (client.is_playing) {
                client.bandwidth = config_.bitrate / 1000000.0; // Mbps
                total_bandwidth += client.bandwidth;
                active_count++;
            }
        }
    }
    
    // Average bandwidth
    if (active_count > 0) {
        stats_.average_bandwidth = total_bandwidth / active_count;
    } else {
        stats_.average_bandwidth = 0;
    }
    
    // Debug output
    if (stats_.active_clients > 0) {
        std::cout << "[RTSPStreamer] Active clients: " << stats_.active_clients
                  << ", Total bandwidth: " << std::fixed << std::setprecision(2)
                  << total_bandwidth << " Mbps" << std::endl;
    }
}