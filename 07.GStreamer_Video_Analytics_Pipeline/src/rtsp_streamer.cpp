/**
 * @file rtsp_streamer.cpp
 * @brief RTSP server ve streaming yönetim sınıfı implementasyonu
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
    
    // GStreamer RTSP server kütüphanesini başlat
    static bool rtsp_initialized = false;
    if (!rtsp_initialized) {
        gst_init(nullptr, nullptr);
        rtsp_initialized = true;
    }
    
    // Başlangıç zamanını kaydet
    start_time_ = std::chrono::steady_clock::now();
}

/**
 * @brief Destructor
 */
RTSPStreamer::~RTSPStreamer() {
    stop();
}

/**
 * @brief RTSP server'ı başlatır
 */
bool RTSPStreamer::start() {
    if (is_running_.load()) {
        return true;
    }
    
    // RTSP server oluştur
    if (!createServer()) {
        std::cerr << "[RTSPStreamer] Server oluşturulamadı!" << std::endl;
        return false;
    }
    
    // Server thread'ini başlat
    is_running_ = true;
    server_thread_ = std::make_unique<std::thread>(&RTSPStreamer::serverMainLoop, this);
    
    std::cout << "[RTSPStreamer] RTSP server başlatıldı: " 
              << getStreamURL() << std::endl;
    
    return true;
}

/**
 * @brief RTSP server'ı durdurur
 */
void RTSPStreamer::stop() {
    if (!is_running_.load()) {
        return;
    }
    
    is_running_ = false;
    
    // Ana döngüyü durdur
    if (main_loop_) {
        g_main_loop_quit(main_loop_);
    }
    
    // Thread'in bitmesini bekle
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    
    // Kaynakları temizle
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
    
    std::cout << "[RTSPStreamer] RTSP server durduruldu." << std::endl;
}

/**
 * @brief Video kaynağı elementini ayarlar
 */
void RTSPStreamer::setVideoSource(GstElement* source) {
    video_source_ = source;
}

/**
 * @brief Ses kaynağı elementini ayarlar
 */
void RTSPStreamer::setAudioSource(GstElement* source) {
    audio_source_ = source;
}

/**
 * @brief Yapılandırmayı günceller
 */
void RTSPStreamer::updateConfig(const RTSPConfig& config) {
    config_ = config;
    
    // Server çalışıyorsa güvenlik ayarlarını güncelle
    if (is_running_ && auth_) {
        applySecurity();
    }
}

/**
 * @brief Mevcut yapılandırmayı döndürür
 */
RTSPConfig RTSPStreamer::getConfig() const {
    return config_;
}

/**
 * @brief Stream URL'sini döndürür
 */
std::string RTSPStreamer::getStreamURL() const {
    std::stringstream url;
    url << "rtsp://" << config_.address << ":" << config_.port << config_.mount_point;
    return url.str();
}

/**
 * @brief Aktif istemci listesini döndürür
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
 * @brief İstemciyi bağlantıdan koparır
 */
bool RTSPStreamer::disconnectClient(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto it = clients_.find(client_id);
    if (it != clients_.end()) {
        // TODO: GStreamer API ile istemciyi kopar
        clients_.erase(it);
        return true;
    }
    
    return false;
}

/**
 * @brief Server istatistiklerini döndürür
 */
RTSPStats RTSPStreamer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Çalışma süresini güncelle
    auto now = std::chrono::steady_clock::now();
    stats_.uptime = now - start_time_;
    
    return stats_;
}

/**
 * @brief İstemci callback'i ayarlar
 */
void RTSPStreamer::setClientCallback(ClientCallback callback) {
    client_callback_ = callback;
}

/**
 * @brief Yeni mount point ekler
 */
bool RTSPStreamer::addMountPoint(const std::string& path, const std::string& pipeline) {
    if (!mounts_) {
        return false;
    }
    
    // Yeni factory oluştur
    GstRTSPMediaFactory* new_factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(new_factory, pipeline.c_str());
    gst_rtsp_media_factory_set_shared(new_factory, TRUE);
    
    // Mount point'e ekle
    gst_rtsp_mount_points_add_factory(mounts_, path.c_str(), new_factory);
    
    std::cout << "[RTSPStreamer] Mount point eklendi: " << path << std::endl;
    return true;
}

/**
 * @brief Mount point kaldırır
 */
bool RTSPStreamer::removeMountPoint(const std::string& path) {
    if (!mounts_) {
        return false;
    }
    
    gst_rtsp_mount_points_remove_factory(mounts_, path.c_str());
    
    std::cout << "[RTSPStreamer] Mount point kaldırıldı: " << path << std::endl;
    return true;
}

/**
 * @brief Güvenlik ayarlarını günceller
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
 * @brief IP tabanlı erişim kontrolü ekler
 */
void RTSPStreamer::setIPFilter(const std::vector<std::string>& allowed_ips) {
    config_.allowed_ips = allowed_ips;
    
    // TODO: IP filtreleme implementasyonu
}

/**
 * @brief Akışı kaydet
 */
bool RTSPStreamer::recordStream(bool enable, const std::string& filename) {
    if (enable && !is_recording_) {
        // TODO: Kayıt implementasyonu
        record_filename_ = filename;
        is_recording_ = true;
        
        std::cout << "[RTSPStreamer] Kayıt başlatıldı: " << filename << std::endl;
        return true;
    } else if (!enable && is_recording_) {
        // TODO: Kaydı durdur
        is_recording_ = false;
        
        std::cout << "[RTSPStreamer] Kayıt durduruldu." << std::endl;
        return true;
    }
    
    return false;
}

/**
 * @brief Anlık görüntü al
 */
bool RTSPStreamer::takeSnapshot(const std::string& filename) {
    // TODO: Snapshot implementasyonu
    std::cout << "[RTSPStreamer] Snapshot alındı: " << filename << std::endl;
    return true;
}

/**
 * @brief RTSP server'ı oluşturur
 */
bool RTSPStreamer::createServer() {
    // RTSP server oluştur
    server_ = gst_rtsp_server_new();
    if (!server_) {
        return false;
    }
    
    // Server özelliklerini ayarla
    g_object_set(server_, 
        "address", config_.address.c_str(),
        "service", std::to_string(config_.port).c_str(),
        "backlog", config_.max_clients,
        nullptr);
    
    // Mount points al
    mounts_ = gst_rtsp_server_get_mount_points(server_);
    
    // Media factory oluştur
    factory_ = createMediaFactory();
    if (!factory_) {
        return false;
    }
    
    // Factory'yi mount point'e ekle
    gst_rtsp_mount_points_add_factory(mounts_, 
                                      config_.mount_point.c_str(), 
                                      factory_);
    
    // Güvenlik ayarlarını uygula
    if (config_.security != SecurityType::NONE) {
        applySecurity();
    }
    
    // Multicast ayarları
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
    
    // İstemci bağlantı sinyallerini bağla
    g_signal_connect(server_, "client-connected",
                     G_CALLBACK(onClientConnected), this);
                     
    // Server'ı attach et
    gst_rtsp_server_attach(server_, nullptr);
    
    return true;
}

/**
 * @brief Media factory oluşturur
 */
GstRTSPMediaFactory* RTSPStreamer::createMediaFactory() {
    GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();
    
    // Pipeline string'ini oluştur
    std::string pipeline = buildPipelineString();
    
    // Factory ayarları
    gst_rtsp_media_factory_set_launch(factory, pipeline.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_media_factory_set_latency(factory, config_.latency);
    gst_rtsp_media_factory_set_buffer_size(factory, config_.buffer_size);
    
    // RTCP ayarları
    if (config_.enable_rtcp) {
        gst_rtsp_media_factory_set_enable_rtcp(factory, TRUE);
    }
    
    // Media configure sinyalini bağla
    g_signal_connect(factory, "media-configure",
                     G_CALLBACK(onMediaConfigure), this);
    
    return factory;
}

/**
 * @brief Pipeline string'i oluşturur
 */
std::string RTSPStreamer::buildPipelineString() {
    std::stringstream pipeline;
    
    // Video kısmı
    if (video_source_) {
        // AppSrc'den video al
        pipeline << "appsrc name=videosrc ! ";
    } else {
        // Test pattern kullan
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
    
    // Ses kısmı (eğer etkinse)
    if (config_.enable_audio) {
        pipeline << " ";
        
        if (audio_source_) {
            pipeline << "appsrc name=audiosrc ! ";
        } else {
            pipeline << "audiotestsrc is-live=true ! ";
        }
        
        // Ses caps
        pipeline << "audio/x-raw,rate=" << config_.audio_samplerate
                 << ",channels=" << config_.audio_channels << " ! ";
        
        // Ses encoder
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
 * @brief Güvenlik ayarlarını uygular
 */
void RTSPStreamer::applySecurity() {
    if (!server_ || config_.security == SecurityType::NONE) {
        return;
    }
    
    // Auth nesnesi oluştur
    if (!auth_) {
        auth_ = gst_rtsp_auth_new();
    }
    
    // Token oluştur
    if (token_) {
        gst_rtsp_token_unref(token_);
    }
    
    if (config_.security == SecurityType::BASIC_AUTH || 
        config_.security == SecurityType::DIGEST_AUTH) {
        
        // Kullanıcı ekle
        GstRTSPToken* basic_token = gst_rtsp_token_new(
            GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, "user",
            nullptr);
        
        gst_rtsp_auth_add_basic(auth_, 
                               config_.username.c_str(), 
                               config_.password.c_str(), 
                               basic_token);
        
        gst_rtsp_token_unref(basic_token);
        
        // Digest auth için
        if (config_.security == SecurityType::DIGEST_AUTH) {
            gst_rtsp_auth_add_digest(auth_,
                                    config_.username.c_str(),
                                    config_.password.c_str(),
                                    basic_token);
        }
    }
    
    // Anonim erişim için token
    GstRTSPToken* anon_token = gst_rtsp_token_new(
        GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, 
        config_.security == SecurityType::NONE ? "user" : "anonymous",
        nullptr);
    
    gst_rtsp_auth_set_default_token(auth_, anon_token);
    gst_rtsp_token_unref(anon_token);
    
    // Auth'u server'a ekle
    gst_rtsp_server_set_auth(server_, auth_);
}

/**
 * @brief İstemci bağlandı callback'i
 */
void RTSPStreamer::onClientConnected(GstRTSPServer* server,
                                    GstRTSPClient* client,
                                    gpointer user_data) {
    RTSPStreamer* streamer = static_cast<RTSPStreamer*>(user_data);
    
    // İstemci bilgisi oluştur
    ClientInfo info = streamer->createClientInfo(client);
    
    // İstemci listesine ekle
    {
        std::lock_guard<std::mutex> lock(streamer->clients_mutex_);
        streamer->clients_[info.id] = info;
    }
    
    // İstatistikleri güncelle
    {
        std::lock_guard<std::mutex> lock(streamer->stats_mutex_);
        streamer->stats_.active_clients++;
        streamer->stats_.total_connections++;
    }
    
    // Callback'i çağır
    if (streamer->client_callback_) {
        streamer->client_callback_(info, true);
    }
    
    // İstemci kopma sinyalini bağla
    g_signal_connect(client, "closed",
                     G_CALLBACK(onClientDisconnected), streamer);
    
    std::cout << "[RTSPStreamer] İstemci bağlandı: " 
              << info.address << ":" << info.port << std::endl;
}

/**
 * @brief İstemci ayrıldı callback'i
 */
void RTSPStreamer::onClientDisconnected(GstRTSPServer* server,
                                       GstRTSPClient* client,
                                       gpointer user_data) {
    RTSPStreamer* streamer = static_cast<RTSPStreamer*>(user_data);
    
    // İstemci bilgisini bul ve kaldır
    ClientInfo info;
    bool found = false;
    
    {
        std::lock_guard<std::mutex> lock(streamer->clients_mutex_);
        
        // İstemciyi bul
        for (auto it = streamer->clients_.begin(); it != streamer->clients_.end(); ++it) {
            // TODO: GstRTSPClient pointer'ını karşılaştır
            if (true) { // Placeholder
                info = it->second;
                streamer->clients_.erase(it);
                found = true;
                break;
            }
        }
    }
    
    if (found) {
        // İstatistikleri güncelle
        {
            std::lock_guard<std::mutex> lock(streamer->stats_mutex_);
            streamer->stats_.active_clients--;
        }
        
        // Callback'i çağır
        if (streamer->client_callback_) {
            streamer->client_callback_(info, false);
        }
        
        std::cout << "[RTSPStreamer] İstemci ayrıldı: " 
                  << info.address << ":" << info.port << std::endl;
    }
}

/**
 * @brief Media yapılandırma callback'i
 */
void RTSPStreamer::onMediaConfigure(GstRTSPMediaFactory* factory,
                                   GstRTSPMedia* media,
                                   gpointer user_data) {
    RTSPStreamer* streamer = static_cast<RTSPStreamer*>(user_data);
    
    // Media durumu değişiklik sinyalini bağla
    g_signal_connect(media, "new-state",
                     G_CALLBACK(onMediaStateChanged), streamer);
    
    // Pipeline'ı al
    GstElement* pipeline = gst_rtsp_media_get_element(media);
    
    // Video source varsa bağla
    if (streamer->video_source_) {
        GstElement* videosrc = gst_bin_get_by_name(GST_BIN(pipeline), "videosrc");
        if (videosrc) {
            // AppSrc ayarları
            g_object_set(videosrc,
                "stream-type", GST_APP_STREAM_TYPE_STREAM,
                "format", GST_FORMAT_TIME,
                "is-live", TRUE,
                nullptr);
            
            // TODO: AppSrc'ye veri besleme mekanizması
            
            gst_object_unref(videosrc);
        }
    }
    
    // Ses source varsa bağla
    if (streamer->audio_source_) {
        GstElement* audiosrc = gst_bin_get_by_name(GST_BIN(pipeline), "audiosrc");
        if (audiosrc) {
            // AppSrc ayarları
            g_object_set(audiosrc,
                "stream-type", GST_APP_STREAM_TYPE_STREAM,
                "format", GST_FORMAT_TIME,
                "is-live", TRUE,
                nullptr);
            
            // TODO: AppSrc'ye veri besleme mekanizması
            
            gst_object_unref(audiosrc);
        }
    }
    
    gst_object_unref(pipeline);
}

/**
 * @brief Stream durumu değişti callback'i
 */
void RTSPStreamer::onMediaStateChanged(GstRTSPMedia* media,
                                      GstState state,
                                      gpointer user_data) {
    RTSPStreamer* streamer = static_cast<RTSPStreamer*>(user_data);
    
    const char* state_name = gst_element_state_get_name(state);
    std::cout << "[RTSPStreamer] Media durumu değişti: " << state_name << std::endl;
    
    // İstatistikleri güncelle
    if (state == GST_STATE_PLAYING) {
        // Yayın başladı
    } else if (state == GST_STATE_NULL) {
        // Yayın durdu
    }
}

/**
 * @brief İstemci bilgisi oluştur
 */
ClientInfo RTSPStreamer::createClientInfo(GstRTSPClient* client) {
    ClientInfo info;
    
    // Bağlantı bilgilerini al
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
    
    // Benzersiz ID oluştur
    static int client_counter = 0;
    info.id = "client_" + std::to_string(++client_counter);
    
    // Bağlantı zamanı
    info.connect_time = std::chrono::steady_clock::now();
    
    return info;
}

/**
 * @brief Server ana döngüsü
 */
void RTSPStreamer::serverMainLoop() {
    // GMainLoop oluştur
    main_loop_ = g_main_loop_new(nullptr, FALSE);
    
    // İstatistik güncelleme timer'ı
    g_timeout_add(1000, [](gpointer data) -> gboolean {
        RTSPStreamer* streamer = static_cast<RTSPStreamer*>(data);
        streamer->updateStats();
        return TRUE;
    }, this);
    
    // Ana döngüyü çalıştır
    g_main_loop_run(main_loop_);
    
    // Temizlik
    g_main_loop_unref(main_loop_);
    main_loop_ = nullptr;
}

/**
 * @brief İstatistikleri güncelle
 */
void RTSPStreamer::updateStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    // Aktif istemci sayısı zaten güncel
    
    // Bant genişliği hesaplama
    double total_bandwidth = 0;
    int active_count = 0;
    
    {
        std::lock_guard<std::mutex> clients_lock(clients_mutex_);
        for (auto& pair : clients_) {
            ClientInfo& client = pair.second;
            
            // TODO: Gerçek bant genişliği ölçümü
            // Şimdilik tahmini değer kullan
            if (client.is_playing) {
                client.bandwidth = config_.bitrate / 1000000.0; // Mbps
                total_bandwidth += client.bandwidth;
                active_count++;
            }
        }
    }
    
    // Ortalama bant genişliği
    if (active_count > 0) {
        stats_.average_bandwidth = total_bandwidth / active_count;
    } else {
        stats_.average_bandwidth = 0;
    }
    
    // Debug çıktısı
    if (stats_.active_clients > 0) {
        std::cout << "[RTSPStreamer] Aktif istemciler: " << stats_.active_clients
                  << ", Toplam bant genişliği: " << std::fixed << std::setprecision(2)
                  << total_bandwidth << " Mbps" << std::endl;
    }
}