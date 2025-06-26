/**
 * @file pipeline_manager.h
 * @brief GStreamer pipeline yönetimi için ana sınıf
 * 
 * Bu sınıf, GStreamer pipeline'ının oluşturulması, yapılandırılması ve
 * yönetilmesinden sorumludur. Video giriş/çıkış kaynaklarını, işleme
 * elemanlarını ve akış kontrolünü yönetir.
 */

#ifndef PIPELINE_MANAGER_H
#define PIPELINE_MANAGER_H

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <map>

// Forward declarations
class VideoProcessor;
class MotionDetector;
class RTSPStreamer;

/**
 * @brief Video kaynağı türlerini tanımlar
 */
enum class SourceType {
    FILE,        // Video dosyası
    WEBCAM,      // USB/V4L2 kamera
    RTSP,        // RTSP stream
    HTTP,        // HTTP stream
    APPSRC       // Uygulama kaynağı
};

/**
 * @brief Çıkış hedef türlerini tanımlar
 */
enum class SinkType {
    DISPLAY,     // Ekrana görüntüleme
    FILE,        // Dosyaya kayıt
    RTSP,        // RTSP server
    APPSINK      // Uygulama havuzu
};

/**
 * @brief Pipeline yapılandırma parametreleri
 */
struct PipelineConfig {
    // Giriş parametreleri
    SourceType source_type = SourceType::FILE;
    std::string source_location = "";
    
    // Video parametreleri
    int width = 1920;
    int height = 1080;
    int framerate = 30;
    std::string video_format = "I420";
    
    // Çıkış parametreleri
    SinkType sink_type = SinkType::DISPLAY;
    std::string sink_location = "";
    
    // İşleme seçenekleri
    bool enable_motion_detection = false;
    bool enable_gpu_acceleration = false;
    bool enable_recording = false;
    std::string record_location = "";
    
    // Codec ayarları
    std::string encoder = "x264enc";
    int bitrate = 4000000; // 4 Mbps
    
    // RTSP server ayarları
    std::string rtsp_mount_point = "/live";
    int rtsp_port = 8554;
};

/**
 * @brief GStreamer pipeline yönetici sınıfı
 */
class PipelineManager {
public:
    /**
     * @brief Constructor
     * @param config Pipeline yapılandırma parametreleri
     */
    explicit PipelineManager(const PipelineConfig& config);
    
    /**
     * @brief Destructor - Kaynakları temizler
     */
    ~PipelineManager();
    
    // Kopya ve taşıma işlemlerini devre dışı bırak
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;
    
    /**
     * @brief Pipeline'ı başlatır
     * @return Başarılı ise true
     */
    bool start();
    
    /**
     * @brief Pipeline'ı durdurur
     */
    void stop();
    
    /**
     * @brief Pipeline'ın çalışıp çalışmadığını kontrol eder
     * @return Çalışıyor ise true
     */
    bool isRunning() const { return is_running_.load(); }
    
    /**
     * @brief Mevcut FPS değerini döndürür
     * @return Saniyedeki kare sayısı
     */
    double getCurrentFPS() const { return current_fps_; }
    
    /**
     * @brief Video işleyici nesnesini döndürür
     * @return VideoProcessor pointer'ı
     */
    VideoProcessor* getVideoProcessor() { return video_processor_.get(); }
    
    /**
     * @brief Hareket algılayıcı nesnesini döndürür
     * @return MotionDetector pointer'ı
     */
    MotionDetector* getMotionDetector() { return motion_detector_.get(); }
    
    /**
     * @brief RTSP yayıncı nesnesini döndürür
     * @return RTSPStreamer pointer'ı
     */
    RTSPStreamer* getRTSPStreamer() { return rtsp_streamer_.get(); }
    
    /**
     * @brief Pipeline'a yeni bir element ekler
     * @param element Eklenecek GStreamer elementi
     * @param name Element adı
     * @return Başarılı ise true
     */
    bool addElement(GstElement* element, const std::string& name);
    
    /**
     * @brief Pipeline'dan bir elementi kaldırır
     * @param name Kaldırılacak element adı
     * @return Başarılı ise true
     */
    bool removeElement(const std::string& name);
    
    /**
     * @brief Çalışma zamanında video kaynağını değiştirir
     * @param new_source Yeni kaynak konumu
     * @param type Yeni kaynak türü
     * @return Başarılı ise true
     */
    bool changeSource(const std::string& new_source, SourceType type);
    
    /**
     * @brief Kayıt işlemini başlatır/durdurur
     * @param start true ise başlat, false ise durdur
     * @param filename Kayıt dosya adı
     * @return Başarılı ise true
     */
    bool toggleRecording(bool start, const std::string& filename = "");
    
    /**
     * @brief Pipeline durumunu döndürür
     * @return GStreamer durum enum'u
     */
    GstState getState() const;
    
    /**
     * @brief Hata callback'i ayarlar
     * @param callback Hata durumunda çağrılacak fonksiyon
     */
    void setErrorCallback(std::function<void(const std::string&)> callback) {
        error_callback_ = callback;
    }
    
    /**
     * @brief EOS (End of Stream) callback'i ayarlar
     * @param callback Stream bittiğinde çağrılacak fonksiyon
     */
    void setEOSCallback(std::function<void()> callback) {
        eos_callback_ = callback;
    }

private:
    /**
     * @brief Pipeline'ı oluşturur
     * @return Başarılı ise true
     */
    bool createPipeline();
    
    /**
     * @brief Video kaynağını oluşturur
     * @return Oluşturulan kaynak elementi
     */
    GstElement* createSource();
    
    /**
     * @brief Video havuzunu oluşturur
     * @return Oluşturulan havuz elementi
     */
    GstElement* createSink();
    
    /**
     * @brief Video kodlayıcısını oluşturur
     * @return Oluşturulan kodlayıcı elementi
     */
    GstElement* createEncoder();
    
    /**
     * @brief Bus mesajlarını işler
     * @param bus GStreamer bus
     * @param msg Mesaj
     * @param user_data Kullanıcı verisi (this pointer)
     * @return TRUE devam et, FALSE dur
     */
    static gboolean busCallback(GstBus* bus, GstMessage* msg, gpointer user_data);
    
    /**
     * @brief Pad-added sinyalini işler
     * @param src Kaynak element
     * @param new_pad Yeni pad
     * @param data Kullanıcı verisi
     */
    static void onPadAdded(GstElement* src, GstPad* new_pad, gpointer data);
    
    /**
     * @brief Ana event loop thread fonksiyonu
     */
    void mainLoopThread();
    
    /**
     * @brief FPS hesaplama fonksiyonu
     */
    void calculateFPS();
    
    /**
     * @brief Pipeline'ı temizler
     */
    void cleanup();
    
    // Üye değişkenler
    PipelineConfig config_;                                    // Pipeline yapılandırması
    
    // GStreamer elementleri
    GstElement* pipeline_ = nullptr;                           // Ana pipeline
    GstElement* source_ = nullptr;                             // Video kaynağı
    GstElement* sink_ = nullptr;                               // Video havuzu
    GstElement* tee_ = nullptr;                                // Akışı bölmek için
    std::map<std::string, GstElement*> elements_;             // Tüm elementler
    
    // Yardımcı sınıflar
    std::unique_ptr<VideoProcessor> video_processor_;          // Video işleyici
    std::unique_ptr<MotionDetector> motion_detector_;          // Hareket algılayıcı
    std::unique_ptr<RTSPStreamer> rtsp_streamer_;             // RTSP yayıncı
    
    // Thread ve senkronizasyon
    std::unique_ptr<std::thread> main_loop_thread_;           // Ana döngü thread'i
    GMainLoop* main_loop_ = nullptr;                          // GLib ana döngüsü
    std::atomic<bool> is_running_{false};                      // Çalışma durumu
    
    // Performans metrikleri
    double current_fps_ = 0.0;                                 // Mevcut FPS
    guint64 frame_count_ = 0;                                 // Toplam kare sayısı
    GstClockTime last_fps_time_ = 0;                          // Son FPS hesaplama zamanı
    
    // Callback fonksiyonları
    std::function<void(const std::string&)> error_callback_;   // Hata callback'i
    std::function<void()> eos_callback_;                       // EOS callback'i
    
    // Kayıt için ek elementler
    GstElement* recording_queue_ = nullptr;                    // Kayıt kuyruğu
    GstElement* recording_sink_ = nullptr;                     // Kayıt havuzu
    bool is_recording_ = false;                                // Kayıt durumu
};