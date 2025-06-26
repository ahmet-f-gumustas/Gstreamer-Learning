/**
 * @file video_processor.h
 * @brief Video işleme ve filtre uygulama sınıfı
 * 
 * Bu sınıf, GStreamer pipeline'ında video karelerini işlemek,
 * filtreler uygulamak ve görüntü iyileştirmeleri yapmak için kullanılır.
 */

#ifndef VIDEO_PROCESSOR_H
#define VIDEO_PROCESSOR_H

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/base/gstbasetransform.h>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

/**
 * @brief Video filtre türleri
 */
enum class FilterType {
    NONE,           // Filtre yok
    GRAYSCALE,      // Gri tonlama
    BLUR,           // Bulanıklaştırma
    SHARPEN,        // Keskinleştirme
    EDGE_DETECT,    // Kenar algılama
    DENOISE,        // Gürültü azaltma
    BRIGHTNESS,     // Parlaklık ayarı
    CONTRAST,       // Kontrast ayarı
    CUSTOM          // Özel filtre
};

/**
 * @brief Video işleme parametreleri
 */
struct ProcessingParams {
    // Filtre parametreleri
    FilterType filter_type = FilterType::NONE;
    
    // Parlaklık ve kontrast ayarları
    double brightness = 0.0;    // -100 ile 100 arası
    double contrast = 1.0;      // 0.5 ile 3.0 arası
    double saturation = 1.0;    // 0.0 ile 2.0 arası
    double hue = 0.0;          // -180 ile 180 arası
    
    // Bulanıklaştırma parametreleri
    int blur_kernel_size = 5;   // Tek sayı olmalı
    
    // Keskinleştirme parametreleri
    double sharpen_strength = 1.0;
    
    // Gürültü azaltma parametreleri
    double denoise_strength = 10.0;
    
    // Kenar algılama parametreleri
    double edge_threshold1 = 100.0;
    double edge_threshold2 = 200.0;
    
    // Döndürme ve çevirme
    int rotation = 0;           // 0, 90, 180, 270 derece
    bool flip_horizontal = false;
    bool flip_vertical = false;
    
    // Kırpma parametreleri
    int crop_x = 0;
    int crop_y = 0;
    int crop_width = 0;
    int crop_height = 0;
    
    // Ölçekleme
    bool enable_scaling = false;
    int scale_width = 0;
    int scale_height = 0;
};

/**
 * @brief Video işleme istatistikleri
 */
struct ProcessingStats {
    guint64 frames_processed = 0;      // İşlenen toplam kare sayısı
    double avg_processing_time = 0.0;  // Ortalama işleme süresi (ms)
    double min_processing_time = 999999.0;
    double max_processing_time = 0.0;
    guint64 dropped_frames = 0;        // Atlanan kare sayısı
};

/**
 * @brief Video işleme callback tipi
 * @param buffer İşlenecek GStreamer buffer
 * @param width Video genişliği
 * @param height Video yüksekliği
 * @return İşlenmiş buffer (nullptr ise orijinal kullanılır)
 */
using ProcessingCallback = std::function<GstBuffer*(GstBuffer*, int, int)>;

/**
 * @brief Video işleme sınıfı
 */
class VideoProcessor {
public:
    /**
     * @brief Constructor
     */
    VideoProcessor();
    
    /**
     * @brief Destructor
     */
    ~VideoProcessor();
    
    /**
     * @brief Video işleme elementini oluşturur
     * @return GStreamer elementi
     */
    GstElement* createElement();
    
    /**
     * @brief İşleme parametrelerini ayarlar
     * @param params Yeni parametreler
     */
    void setParameters(const ProcessingParams& params);
    
    /**
     * @brief Mevcut parametreleri döndürür
     * @return İşleme parametreleri
     */
    ProcessingParams getParameters() const;
    
    /**
     * @brief Filtre türünü değiştirir
     * @param type Yeni filtre türü
     */
    void setFilter(FilterType type);
    
    /**
     * @brief Özel işleme callback'i ayarlar
     * @param callback İşleme fonksiyonu
     */
    void setCustomProcessor(ProcessingCallback callback);
    
    /**
     * @brief İşleme istatistiklerini döndürür
     * @return İstatistik yapısı
     */
    ProcessingStats getStats() const;
    
    /**
     * @brief İstatistikleri sıfırlar
     */
    void resetStats();
    
    /**
     * @brief GPU hızlandırmayı etkinleştirir/devre dışı bırakır
     * @param enable true ise etkinleştir
     */
    void setGPUAcceleration(bool enable);
    
    /**
     * @brief Anlık görüntü alır
     * @param filename Kayıt dosya adı
     * @return Başarılı ise true
     */
    bool takeSnapshot(const std::string& filename);
    
    /**
     * @brief Video meta verilerini ekler
     * @param key Meta veri anahtarı
     * @param value Meta veri değeri
     */
    void addMetadata(const std::string& key, const std::string& value);
    
#ifdef HAVE_OPENCV
    /**
     * @brief OpenCV Mat nesnesine dönüştürür
     * @param buffer GStreamer buffer
     * @param info Video bilgisi
     * @return OpenCV Mat nesnesi
     */
    cv::Mat bufferToMat(GstBuffer* buffer, const GstVideoInfo* info);
    
    /**
     * @brief OpenCV Mat nesnesinden buffer oluşturur
     * @param mat OpenCV Mat nesnesi
     * @param info Video bilgisi
     * @return GStreamer buffer
     */
    GstBuffer* matToBuffer(const cv::Mat& mat, const GstVideoInfo* info);
#endif

private:
    /**
     * @brief Video karesini işler
     * @param buffer İşlenecek buffer
     * @param info Video bilgisi
     * @return İşlenmiş buffer
     */
    GstBuffer* processFrame(GstBuffer* buffer, const GstVideoInfo* info);
    
    /**
     * @brief Gri tonlama filtresi uygular
     * @param data Video verisi
     * @param info Video bilgisi
     */
    void applyGrayscale(guint8* data, const GstVideoInfo* info);
    
    /**
     * @brief Parlaklık/kontrast ayarı yapar
     * @param data Video verisi
     * @param info Video bilgisi
     */
    void adjustBrightnessContrast(guint8* data, const GstVideoInfo* info);
    
#ifdef HAVE_OPENCV
    /**
     * @brief OpenCV tabanlı filtre uygular
     * @param mat OpenCV Mat nesnesi
     * @param type Filtre türü
     */
    void applyOpenCVFilter(cv::Mat& mat, FilterType type);
    
    /**
     * @brief Bulanıklaştırma filtresi (OpenCV)
     * @param mat Görüntü matrisi
     */
    void applyBlur(cv::Mat& mat);
    
    /**
     * @brief Keskinleştirme filtresi (OpenCV)
     * @param mat Görüntü matrisi
     */
    void applySharpen(cv::Mat& mat);
    
    /**
     * @brief Kenar algılama filtresi (OpenCV)
     * @param mat Görüntü matrisi
     */
    void applyEdgeDetection(cv::Mat& mat);
    
    /**
     * @brief Gürültü azaltma filtresi (OpenCV)
     * @param mat Görüntü matrisi
     */
    void applyDenoise(cv::Mat& mat);
#endif
    
    /**
     * @brief Transform callback (GStreamer)
     * @param trans Transform elementi
     * @param inbuf Giriş buffer
     * @param outbuf Çıkış buffer
     * @return Akış durumu
     */
    static GstFlowReturn transformCallback(GstBaseTransform* trans,
                                         GstBuffer* inbuf,
                                         GstBuffer* outbuf);
    
    /**
     * @brief Caps ayarlama callback'i
     * @param trans Transform elementi
     * @param incaps Giriş caps
     * @param outcaps Çıkış caps
     * @return Başarılı ise TRUE
     */
    static gboolean setCapsCallback(GstBaseTransform* trans,
                                  GstCaps* incaps,
                                  GstCaps* outcaps);
    
    // Üye değişkenler
    GstElement* element_ = nullptr;                    // GStreamer elementi
    ProcessingParams params_;                          // İşleme parametreleri
    ProcessingStats stats_;                            // İstatistikler
    mutable std::mutex params_mutex_;                  // Parametre erişim mutex'i
    mutable std::mutex stats_mutex_;                   // İstatistik erişim mutex'i
    
    ProcessingCallback custom_processor_;              // Özel işleme callback'i
    bool gpu_enabled_ = false;                         // GPU hızlandırma durumu
    
    GstVideoInfo video_info_;                          // Video format bilgisi
    bool video_info_valid_ = false;                    // Video bilgisi geçerli mi
    
    // Anlık görüntü için
    std::mutex snapshot_mutex_;                        // Snapshot mutex'i
    bool take_snapshot_ = false;                       // Snapshot al bayrağı
    std::string snapshot_filename_;                    // Snapshot dosya adı
    
    // Meta veri depolama
    std::map<std::string, std::string> metadata_;      // Meta veriler
    std::mutex metadata_mutex_;                        // Meta veri mutex'i
    
    // Performans ölçümü için
    GstClockTime last_timestamp_ = 0;                  // Son zaman damgası
    guint64 processing_time_sum_ = 0;                  // Toplam işleme süresi
};

#endif // VIDEO_PROCESSOR_H