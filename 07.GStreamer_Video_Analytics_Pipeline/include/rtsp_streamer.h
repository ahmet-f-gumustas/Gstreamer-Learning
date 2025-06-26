/**
 * @file rtsp_streamer.h
 * @brief RTSP server ve streaming yönetim sınıfı
 * 
 * Bu sınıf, GStreamer RTSP Server kullanarak video akışlarını
 * ağ üzerinden yayınlamak için gerekli işlevleri sağlar.
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
 * @brief RTSP akış kalitesi profilleri
 */
enum class StreamProfile {
    LOW,        // 480p, 1 Mbps
    MEDIUM,     // 720p, 2.5 Mbps
    HIGH,       // 1080p, 5 Mbps
    ULTRA,      // 4K, 15 Mbps
    CUSTOM      // Özel ayarlar
};

/**
 * @brief RTSP güvenlik türleri
 */
enum class SecurityType {
    NONE,           // Güvenlik yok
    BASIC_AUTH,     // Basit kullanıcı adı/şifre
    DIGEST_AUTH,    // Digest authentication
    TOKEN_AUTH      // Token tabanlı
};

/**
 * @brief RTSP akış yapılandırması
 */
struct RTSPConfig {
    // Server ayarları
    std::string address = "0.0.0.0";        // Dinleme adresi
    int port = 8554;                        // RTSP portu
    std::string mount_point = "/live";      // Mount noktası
    
    // Video ayarları
    StreamProfile profile = StreamProfile::MEDIUM;
    int width = 1280;
    int height = 720;
    int framerate = 30;
    int bitrate = 2500000;                  // 2.5 Mbps
    std::string encoder = "x264enc";        // Video kodlayıcı
    
    // Ses ayarları
    bool enable_audio = true;
    std::string audio_encoder = "opus";
    int audio_bitrate = 128000;             // 128 kbps
    int audio_channels = 2;
    int audio_samplerate = 48000;
    
    // Güvenlik ayarları
    SecurityType security = SecurityType::NONE;
    std::string username = "";
    std::string password = "";
    std::vector<std::string> allowed_ips;   // İzin verilen IP listesi
    
    // Performans ayarları
    int max_clients = 10;                   // Maksimum istemci sayısı
    int buffer_size = 200;                  // Buffer boyutu (kare)
    int latency = 200;                      // Hedef gecikme (ms)
    bool enable_rtcp = true;                // RTCP etkin
    
    // Multicast ayarları
    bool enable_multicast = false;
    std::string multicast_address = "224.1.1.1";
    int multicast_port = 5000;
    int multicast_ttl = 1;
    
    // Kayıt ayarları
    bool enable_recording = false;
    std::string record_path = "/tmp/rtsp_recordings";
    int max_record_duration = 3600;         // Maksimum kayıt süresi (saniye)
};

/**
 * @brief İstemci bilgisi
 */
struct ClientInfo {
    std::string id;                         // İstemci ID
    std::string address;                    // IP adresi
    int port;                               // Port numarası
    std::chrono::time_point<std::chrono::steady_clock> connect_time;
    guint64 bytes_sent = 0;                 // Gönderilen veri
    guint64 frames_sent = 0;                // Gönderilen kare sayısı
    double bandwidth = 0.0;                 // Anlık bant genişliği (Mbps)
    bool is_playing = false;                // Oynatma durumu
};

/**
 * @brief RTSP server istatistikleri
 */
struct RTSPStats {
    int active_clients = 0;                 // Aktif istemci sayısı
    guint64 total_bytes_sent = 0;           // Toplam gönderilen veri
    guint64 total_frames_sent = 0;          // Toplam gönderilen kare
    double average_bandwidth = 0.0;         // Ortalama bant genişliği
    int total_connections = 0;              // Toplam bağlantı sayısı
    std::chrono::duration<double> uptime;   // Çalışma süresi
};

/**
 * @brief İstemci bağlantı callback'i
 * @param client_info İstemci bilgisi
 * @param connected true: bağlandı, false: ayrıldı
 */
using ClientCallback = std::function<void(const ClientInfo&, bool)>;

/**
 * @brief RTSP yayıncı sınıfı
 */
class RTSPStreamer {
public:
    /**
     * @brief Constructor
     * @param config RTSP yapılandırması
     */
    explicit RTSPStreamer(const RTSPConfig& config = RTSPConfig());
    
    /**
     * @brief Destructor
     */
    ~RTSPStreamer();
    
    /**
     * @brief RTSP server'ı başlatır
     * @return Başarılı ise true
     */
    bool start();
    
    /**
     * @brief RTSP server'ı durdurur
     */
    void stop();
    
    /**
     * @brief Server çalışıyor mu kontrol eder
     * @return Çalışıyor ise true
     */
    bool isRunning() const { return is_running_; }
    
    /**
     * @brief Video kaynağı elementini ayarlar
     * @param source Kaynak elementi
     */
    void setVideoSource(GstElement* source);
    
    /**
     * @brief Ses kaynağı elementini ayarlar
     * @param source Kaynak elementi
     */
    void setAudioSource(GstElement* source);
    
    /**
     * @brief Yapılandırmayı günceller
     * @param config Yeni yapılandırma
     */
    void updateConfig(const RTSPConfig& config);
    
    /**
     * @brief Mevcut yapılandırmayı döndürür
     * @return RTSP yapılandırması
     */
    RTSPConfig getConfig() const;
    
    /**
     * @brief Stream URL'sini döndürür
     * @return RTSP URL
     */
    std::string getStreamURL() const;
    
    /**
     * @brief Aktif istemci listesini döndürür
     * @return İstemci bilgileri
     */
    std::vector<ClientInfo> getClients() const;
    
    /**
     * @brief İstemciyi bağlantıdan koparır
     * @param client_id İstemci ID
     * @return Başarılı ise true
     */
    bool disconnectClient(const std::string& client_id);
    
    /**
     * @brief Server istatistiklerini döndürür
     * @return İstatistikler
     */
    RTSPStats getStats() const;
    
    /**
     * @brief İstemci callback'i ayarlar
     * @param callback Bağlantı/kopma olayları için
     */
    void setClientCallback(ClientCallback callback);
    
    /**
     * @brief Yeni mount point ekler
     * @param path Mount yolu
     * @param pipeline Pipeline tanımı
     * @return Başarılı ise true
     */
    bool addMountPoint(const std::string& path, const std::string& pipeline);
    
    /**
     * @brief Mount point kaldırır
     * @param path Mount yolu
     * @return Başarılı ise true
     */
    bool removeMountPoint(const std::string& path);
    
    /**
     * @brief Güvenlik ayarlarını günceller
     * @param type Güvenlik türü
     * @param username Kullanıcı adı
     * @param password Şifre
     */
    void setSecurity(SecurityType type, 
                     const std::string& username = "",
                     const std::string& password = "");
    
    /**
     * @brief IP tabanlı erişim kontrolü ekler
     * @param allowed_ips İzin verilen IP listesi
     */
    void setIPFilter(const std::vector<std::string>& allowed_ips);
    
    /**
     * @brief Akışı kaydet
     * @param enable true: kayıt başlat, false: durdur
     * @param filename Kayıt dosya adı
     * @return Başarılı ise true
     */
    bool recordStream(bool enable, const std::string& filename = "");
    
    /**
     * @brief Anlık görüntü al
     * @param filename Dosya adı
     * @return Başarılı ise true
     */
    bool takeSnapshot(const std::string& filename);

private:
    /**
     * @brief RTSP server'ı oluşturur
     * @return Başarılı ise true
     */
    bool createServer();
    
    /**
     * @brief Media factory oluşturur
     * @return Media factory pointer'ı
     */
    GstRTSPMediaFactory* createMediaFactory();
    
    /**
     * @brief Pipeline string'i oluşturur
     * @return GStreamer pipeline tanımı
     */
    std::string buildPipelineString();
    
    /**
     * @brief Güvenlik ayarlarını uygular
     */
    void applySecurity();
    
    /**
     * @brief İstemci bağlandı callback'i
     * @param server RTSP server
     * @param client İstemci nesnesi
     * @param user_data Kullanıcı verisi
     */
    static void onClientConnected(GstRTSPServer* server,
                                  GstRTSPClient* client,
                                  gpointer user_data);
    
    /**
     * @brief İstemci ayrıldı callback'i
     * @param server RTSP server
     * @param client İstemci nesnesi
     * @param user_data Kullanıcı verisi
     */
    static void onClientDisconnected(GstRTSPServer* server,
                                    GstRTSPClient* client,
                                    gpointer user_data);
    
    /**
     * @brief Media yapılandırma callback'i
     * @param factory Media factory
     * @param media Media nesnesi
     * @param user_data Kullanıcı verisi
     */
    static void onMediaConfigure(GstRTSPMediaFactory* factory,
                                GstRTSPMedia* media,
                                gpointer user_data);
    
    /**
     * @brief Stream durumu değişti callback'i
     * @param media Media nesnesi
     * @param state Yeni durum
     * @param user_data Kullanıcı verisi
     */
    static void onMediaStateChanged(GstRTSPMedia* media,
                                   GstState state,
                                   gpointer user_data);
    
    /**
     * @brief İstemci bilgisi oluştur
     * @param client RTSP istemci nesnesi
     * @return İstemci bilgisi
     */
    ClientInfo createClientInfo(GstRTSPClient* client);
    
    /**
     * @brief Server ana döngüsü
     */
    void serverMainLoop();
    
    /**
     * @brief İstatistikleri güncelle
     */
    void updateStats();
    
    // Üye değişkenler
    RTSPConfig config_;                         // Server yapılandırması
    
    // RTSP Server nesneleri
    GstRTSPServer* server_ = nullptr;           // Ana server
    GstRTSPMediaFactory* factory_ = nullptr;    // Media factory
    GstRTSPAuth* auth_ = nullptr;              // Yetkilendirme
    GstRTSPToken* token_ = nullptr;            // Güvenlik token'ı
    GstRTSPMountPoints* mounts_ = nullptr;      // Mount noktaları
    
    // Video/Ses kaynakları
    GstElement* video_source_ = nullptr;        // Video kaynağı
    GstElement* audio_source_ = nullptr;        // Ses kaynağı
    
    // Thread ve senkronizasyon
    std::unique_ptr<std::thread> server_thread_; // Server thread'i
    GMainLoop* main_loop_ = nullptr;            // GLib ana döngüsü
    std::atomic<bool> is_running_{false};       // Çalışma durumu
    
    // İstemci yönetimi
    std::map<std::string, ClientInfo> clients_; // Aktif istemciler
    mutable std::mutex clients_mutex_;          // İstemci mutex'i
    ClientCallback client_callback_;            // İstemci callback'i
    
    // İstatistikler
    RTSPStats stats_;                          // Server istatistikleri
    mutable std::mutex stats_mutex_;           // İstatistik mutex'i
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
    
    // Kayıt için
    GstElement* recording_bin_ = nullptr;       // Kayıt elementi
    bool is_recording_ = false;                 // Kayıt durumu
    std::string record_filename_;               // Kayıt dosya adı
    
    // Multicast grubu
    GstRTSPAddressPool* address_pool_ = nullptr; // Adres havuzu
};

#endif // RTSP_STREAMER_H