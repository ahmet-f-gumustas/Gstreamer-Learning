/**
 * @file motion_detector.h
 * @brief Video akışında hareket algılama sınıfı
 * 
 * Bu sınıf, video karelerini analiz ederek hareket algılama,
 * hareket bölgelerini işaretleme ve hareket olaylarını raporlama
 * işlevlerini sağlar.
 */

#ifndef MOTION_DETECTOR_H
#define MOTION_DETECTOR_H

#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <chrono>
#include <deque>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>
#endif

/**
 * @brief Hareket algılama algoritmaları
 */
enum class MotionAlgorithm {
    FRAME_DIFF,         // Basit kare farkı
    BACKGROUND_SUB,     // Arka plan çıkarma
    OPTICAL_FLOW,       // Optik akış
    MOG2,              // Mixture of Gaussians v2
    KNN                // K-Nearest Neighbors
};

/**
 * @brief Hareket bölgesi bilgisi
 */
struct MotionRegion {
    int x;              // Sol üst köşe X koordinatı
    int y;              // Sol üst köşe Y koordinatı
    int width;          // Genişlik
    int height;         // Yükseklik
    double intensity;   // Hareket yoğunluğu (0.0 - 1.0)
    guint64 timestamp;  // Algılama zamanı
};

/**
 * @brief Hareket algılama parametreleri
 */
struct MotionDetectionParams {
    // Algoritma seçimi
    MotionAlgorithm algorithm = MotionAlgorithm::MOG2;
    
    // Hassasiyet ayarları
    double sensitivity = 0.5;           // 0.0 (düşük) - 1.0 (yüksek)
    double threshold = 25.0;            // Piksel değişim eşiği
    
    // Arka plan çıkarma parametreleri
    int history_length = 500;           // Arka plan geçmişi uzunluğu
    double learning_rate = 0.005;       // Öğrenme oranı
    bool detect_shadows = true;         // Gölge tespiti
    
    // Morfolojik işlemler
    int erosion_size = 2;              // Erozyon kernel boyutu
    int dilation_size = 4;             // Dilatasyon kernel boyutu
    
    // Bölge filtreleme
    int min_area = 500;                // Minimum hareket alanı (piksel²)
    int max_area = 100000;             // Maksimum hareket alanı
    double min_aspect_ratio = 0.3;     // Minimum en/boy oranı
    double max_aspect_ratio = 3.0;     // Maksimum en/boy oranı
    
    // Hareket takibi
    bool enable_tracking = true;        // Nesne takibi
    int max_tracked_objects = 10;       // Maksimum takip edilecek nesne
    
    // Görselleştirme
    bool draw_motion_regions = true;    // Hareket bölgelerini çiz
    bool draw_motion_vectors = false;   // Hareket vektörlerini çiz
    bool show_debug_view = false;       // Debug görünümü
    
    // Alarm ayarları
    bool enable_alerts = true;          // Hareket alarmları
    double alert_threshold = 0.1;       // Alarm eşiği (toplam alan yüzdesi)
    int alert_cooldown_ms = 5000;      // Alarm bekleme süresi
};

/**
 * @brief Hareket istatistikleri
 */
struct MotionStats {
    guint64 total_frames = 0;          // İşlenen toplam kare
    guint64 motion_frames = 0;         // Hareket algılanan kareler
    double average_motion_area = 0.0;   // Ortalama hareket alanı
    double max_motion_area = 0.0;       // Maksimum hareket alanı
    guint64 total_regions = 0;         // Toplam hareket bölgesi
    std::chrono::time_point<std::chrono::steady_clock> last_motion_time;
};

/**
 * @brief Hareket olayı callback tipi
 * @param regions Algılanan hareket bölgeleri
 * @param timestamp Olay zamanı
 * @param motion_percentage Hareketin toplam alan yüzdesi
 */
using MotionEventCallback = std::function<void(
    const std::vector<MotionRegion>&, guint64, double)>;

/**
 * @brief Hareket algılama sınıfı
 */
class MotionDetector {
public:
    /**
     * @brief Constructor
     */
    MotionDetector();
    
    /**
     * @brief Destructor
     */
    ~MotionDetector();
    
    /**
     * @brief GStreamer elementi oluşturur
     * @return Hareket algılama elementi
     */
    GstElement* createElement();
    
    /**
     * @brief Algılama parametrelerini ayarlar
     * @param params Yeni parametreler
     */
    void setParameters(const MotionDetectionParams& params);
    
    /**
     * @brief Mevcut parametreleri döndürür
     * @return Algılama parametreleri
     */
    MotionDetectionParams getParameters() const;
    
    /**
     * @brief Hareket algılama algoritmasını değiştirir
     * @param algorithm Yeni algoritma
     */
    void setAlgorithm(MotionAlgorithm algorithm);
    
    /**
     * @brief Hassasiyet seviyesini ayarlar
     * @param sensitivity 0.0 (düşük) - 1.0 (yüksek)
     */
    void setSensitivity(double sensitivity);
    
    /**
     * @brief Hareket olay callback'i ayarlar
     * @param callback Olay fonksiyonu
     */
    void setMotionEventCallback(MotionEventCallback callback);
    
    /**
     * @brief İstatistikleri döndürür
     * @return Hareket istatistikleri
     */
    MotionStats getStats() const;
    
    /**
     * @brief İstatistikleri sıfırlar
     */
    void resetStats();
    
    /**
     * @brief Mevcut hareket bölgelerini döndürür
     * @return Hareket bölgeleri listesi
     */
    std::vector<MotionRegion> getCurrentMotions() const;
    
    /**
     * @brief Hareket algılamayı başlatır/durdurur
     * @param enable true ise başlat
     */
    void setEnabled(bool enable);
    
    /**
     * @brief Algılama durumunu döndürür
     * @return Aktif ise true
     */
    bool isEnabled() const { return enabled_; }
    
    /**
     * @brief İlgi alanı (ROI) ayarlar
     * @param x Sol üst X koordinatı
     * @param y Sol üst Y koordinatı
     * @param width Genişlik
     * @param height Yükseklik
     */
    void setROI(int x, int y, int width, int height);
    
    /**
     * @brief İlgi alanını temizler
     */
    void clearROI();
    
    /**
     * @brief Hariç tutulacak bölge ekler
     * @param x Sol üst X koordinatı
     * @param y Sol üst Y koordinatı
     * @param width Genişlik
     * @param height Yükseklik
     */
    void addExclusionZone(int x, int y, int width, int height);
    
    /**
     * @brief Tüm hariç tutma bölgelerini temizler
     */
    void clearExclusionZones();

#ifdef HAVE_OPENCV
    /**
     * @brief OpenCV tabanlı hareket maskesini döndürür
     * @return Hareket maskesi (binary görüntü)
     */
    cv::Mat getMotionMask() const;
    
    /**
     * @brief Arka plan modelini döndürür
     * @return Arka plan görüntüsü
     */
    cv::Mat getBackgroundModel() const;
#endif

private:
    /**
     * @brief Video karesini işler
     * @param buffer GStreamer buffer
     * @param info Video bilgisi
     * @return İşlenmiş buffer
     */
    GstBuffer* processFrame(GstBuffer* buffer, const GstVideoInfo* info);
    
#ifdef HAVE_OPENCV
    /**
     * @brief OpenCV ile hareket algıla
     * @param frame Mevcut kare
     * @return Algılanan hareket bölgeleri
     */
    std::vector<MotionRegion> detectMotion(const cv::Mat& frame);
    
    /**
     * @brief Kare farkı algoritması
     * @param frame Mevcut kare
     * @return Hareket maskesi
     */
    cv::Mat frameDifference(const cv::Mat& frame);
    
    /**
     * @brief Arka plan çıkarma algoritması
     * @param frame Mevcut kare
     * @return Hareket maskesi
     */
    cv::Mat backgroundSubtraction(const cv::Mat& frame);
    
    /**
     * @brief Optik akış algoritması
     * @param frame Mevcut kare
     * @return Hareket vektörleri
     */
    std::vector<cv::Point2f> opticalFlow(const cv::Mat& frame);
    
    /**
     * @brief Morfolojik işlemler uygula
     * @param mask İkili hareket maskesi
     */
    void applyMorphology(cv::Mat& mask);
    
    /**
     * @brief Hareket bölgelerini bul
     * @param mask İkili hareket maskesi
     * @return Hareket bölgeleri
     */
    std::vector<MotionRegion> findMotionRegions(const cv::Mat& mask);
    
    /**
     * @brief Hareket bölgelerini filtrele
     * @param regions Ham hareket bölgeleri
     * @return Filtrelenmiş bölgeler
     */
    std::vector<MotionRegion> filterRegions(const std::vector<MotionRegion>& regions);
    
    /**
     * @brief Hareket bölgelerini çiz
     * @param frame Üzerine çizilecek kare
     * @param regions Hareket bölgeleri
     */
    void drawMotionRegions(cv::Mat& frame, const std::vector<MotionRegion>& regions);
#endif
    
    /**
     * @brief Transform callback
     */
    static GstFlowReturn transformCallback(GstBaseTransform* trans,
                                         GstBuffer* inbuf,
                                         GstBuffer* outbuf);
    
    /**
     * @brief Hareket olayını tetikle
     * @param regions Hareket bölgeleri
     * @param timestamp Olay zamanı
     */
    void triggerMotionEvent(const std::vector<MotionRegion>& regions, guint64 timestamp);
    
    // Üye değişkenler
    GstElement* element_ = nullptr;              // GStreamer elementi
    MotionDetectionParams params_;               // Algılama parametreleri
    MotionStats stats_;                         // İstatistikler
    mutable std::mutex params_mutex_;           // Parametre mutex'i
    mutable std::mutex stats_mutex_;            // İstatistik mutex'i
    
    bool enabled_ = true;                       // Algılama durumu
    MotionEventCallback motion_callback_;       // Hareket olay callback'i
    
#ifdef HAVE_OPENCV
    // OpenCV değişkenleri
    cv::Mat previous_frame_;                    // Önceki kare
    cv::Mat background_model_;                  // Arka plan modeli
    cv::Mat motion_mask_;                       // Hareket maskesi
    
    // Arka plan çıkarıcılar
    cv::Ptr<cv::BackgroundSubtractor> bg_subtractor_;
    
    // Optik akış için
    cv::Mat previous_gray_;                     // Önceki gri kare
    std::vector<cv::Point2f> previous_points_;  // Önceki özellik noktaları
#endif
    
    // Hareket bölgeleri
    std::vector<MotionRegion> current_motions_; // Mevcut hareket bölgeleri
    mutable std::mutex motions_mutex_;          // Hareket bölgeleri mutex'i
    
    // ROI ve hariç tutma bölgeleri
    cv::Rect roi_;                              // İlgi alanı
    bool has_roi_ = false;                      // ROI tanımlı mı
    std::vector<cv::Rect> exclusion_zones_;     // Hariç tutma bölgeleri
    
    // Alarm yönetimi
    std::chrono::time_point<std::chrono::steady_clock> last_alert_time_;
    bool in_cooldown_ = false;                  // Alarm bekleme durumu
    
    // Hareket geçmişi (takip için)
    std::deque<std::vector<MotionRegion>> motion_history_;
    const size_t max_history_size_ = 30;        // 1 saniyelik geçmiş (30fps)
};

#endif // MOTION_DETECTOR_H