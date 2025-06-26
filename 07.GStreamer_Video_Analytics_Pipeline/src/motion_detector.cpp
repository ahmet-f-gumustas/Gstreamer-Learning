/**
 * @file motion_detector.cpp
 * @brief Hareket algılama sınıfı implementasyonu
 */

#include "motion_detector.h"
#include <iostream>
#include <algorithm>
#include <cmath>

// GStreamer base transform için sanal tablo
typedef struct {
    GstBaseTransformClass parent_class;
} MotionDetectorClass;

typedef struct {
    GstBaseTransform parent;
    MotionDetector* detector;
} MotionDetectorElement;

// GObject tip tanımlamaları
#define MOTION_DETECTOR_TYPE (motion_detector_get_type())
#define MOTION_DETECTOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), MOTION_DETECTOR_TYPE, MotionDetectorElement))

G_DEFINE_TYPE(MotionDetectorElement, motion_detector, GST_TYPE_BASE_TRANSFORM)

// Forward declarations
static GstFlowReturn motion_detector_transform(GstBaseTransform* trans, GstBuffer* inbuf, GstBuffer* outbuf);
static gboolean motion_detector_set_caps(GstBaseTransform* trans, GstCaps* incaps, GstCaps* outcaps);

/**
 * @brief GObject sınıf başlatma
 */
static void motion_detector_class_init(MotionDetectorClass* klass) {
    GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    
    base_transform_class->transform = GST_DEBUG_FUNCPTR(motion_detector_transform);
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(motion_detector_set_caps);
    
    // In-place transform kullan
    base_transform_class->transform_ip_on_passthrough = FALSE;
}

/**
 * @brief GObject örnek başlatma
 */
static void motion_detector_init(MotionDetectorElement* element) {
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(element), TRUE);
}

/**
 * @brief Constructor
 */
MotionDetector::MotionDetector() {
    // İstatistikleri sıfırla
    resetStats();
    
    // Son alarm zamanını başlat
    last_alert_time_ = std::chrono::steady_clock::now();
    
#ifdef HAVE_OPENCV
    // Arka plan çıkarıcı oluştur
    if (params_.algorithm == MotionAlgorithm::MOG2) {
        bg_subtractor_ = cv::createBackgroundSubtractorMOG2(
            params_.history_length, 
            16.0, 
            params_.detect_shadows
        );
    } else if (params_.algorithm == MotionAlgorithm::KNN) {
        bg_subtractor_ = cv::createBackgroundSubtractorKNN(
            params_.history_length,
            400.0,
            params_.detect_shadows
        );
    }
    
    std::cout << "[MotionDetector] OpenCV hareket algılama etkin." << std::endl;
#endif
}

/**
 * @brief Destructor
 */
MotionDetector::~MotionDetector() {
    if (element_) {
        gst_object_unref(element_);
    }
}

/**
 * @brief GStreamer elementi oluşturur
 */
GstElement* MotionDetector::createElement() {
    element_ = g_object_new(MOTION_DETECTOR_TYPE, nullptr);
    MOTION_DETECTOR(element_)->detector = this;
    
    return GST_ELEMENT(element_);
}

/**
 * @brief Algılama parametrelerini ayarlar
 */
void MotionDetector::setParameters(const MotionDetectionParams& params) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    params_ = params;
    
#ifdef HAVE_OPENCV
    // Algoritma değiştiyse yeni arka plan çıkarıcı oluştur
    if (params_.algorithm == MotionAlgorithm::MOG2) {
        bg_subtractor_ = cv::createBackgroundSubtractorMOG2(
            params_.history_length, 
            16.0, 
            params_.detect_shadows
        );
    } else if (params_.algorithm == MotionAlgorithm::KNN) {
        bg_subtractor_ = cv::createBackgroundSubtractorKNN(
            params_.history_length,
            400.0,
            params_.detect_shadows
        );
    }
#endif
}

/**
 * @brief Mevcut parametreleri döndürür
 */
MotionDetectionParams MotionDetector::getParameters() const {
    std::lock_guard<std::mutex> lock(params_mutex_);
    return params_;
}

/**
 * @brief Hareket algılama algoritmasını değiştirir
 */
void MotionDetector::setAlgorithm(MotionAlgorithm algorithm) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    params_.algorithm = algorithm;
    
#ifdef HAVE_OPENCV
    // Yeni algoritma için arka plan modelini sıfırla
    previous_frame_ = cv::Mat();
    background_model_ = cv::Mat();
    
    if (algorithm == MotionAlgorithm::MOG2) {
        bg_subtractor_ = cv::createBackgroundSubtractorMOG2();
    } else if (algorithm == MotionAlgorithm::KNN) {
        bg_subtractor_ = cv::createBackgroundSubtractorKNN();
    }
#endif
}

/**
 * @brief Hassasiyet seviyesini ayarlar
 */
void MotionDetector::setSensitivity(double sensitivity) {
    std::lock_guard<std::mutex> lock(params_mutex_);
    params_.sensitivity = std::clamp(sensitivity, 0.0, 1.0);
    
    // Hassasiyete göre eşik değerlerini ayarla
    params_.threshold = 25.0 * (1.0 - params_.sensitivity) + 5.0;
    params_.min_area = (int)(1000 * (1.0 - params_.sensitivity) + 100);
}

/**
 * @brief Hareket olay callback'i ayarlar
 */
void MotionDetector::setMotionEventCallback(MotionEventCallback callback) {
    motion_callback_ = callback;
}

/**
 * @brief İstatistikleri döndürür
 */
MotionStats MotionDetector::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

/**
 * @brief İstatistikleri sıfırlar
 */
void MotionDetector::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = MotionStats();
    stats_.last_motion_time = std::chrono::steady_clock::now();
}

/**
 * @brief Mevcut hareket bölgelerini döndürür
 */
std::vector<MotionRegion> MotionDetector::getCurrentMotions() const {
    std::lock_guard<std::mutex> lock(motions_mutex_);
    return current_motions_;
}

/**
 * @brief Hareket algılamayı etkinleştirir/devre dışı bırakır
 */
void MotionDetector::setEnabled(bool enable) {
    enabled_ = enable;
}

/**
 * @brief İlgi alanı ayarlar
 */
void MotionDetector::setROI(int x, int y, int width, int height) {
    roi_ = cv::Rect(x, y, width, height);
    has_roi_ = true;
}

/**
 * @brief İlgi alanını temizler
 */
void MotionDetector::clearROI() {
    has_roi_ = false;
}

/**
 * @brief Hariç tutulacak bölge ekler
 */
void MotionDetector::addExclusionZone(int x, int y, int width, int height) {
    exclusion_zones_.push_back(cv::Rect(x, y, width, height));
}

/**
 * @brief Tüm hariç tutma bölgelerini temizler
 */
void MotionDetector::clearExclusionZones() {
    exclusion_zones_.clear();
}

#ifdef HAVE_OPENCV
/**
 * @brief Hareket maskesini döndürür
 */
cv::Mat MotionDetector::getMotionMask() const {
    return motion_mask_.clone();
}

/**
 * @brief Arka plan modelini döndürür
 */
cv::Mat MotionDetector::getBackgroundModel() const {
    if (bg_subtractor_) {
        cv::Mat background;
        bg_subtractor_->getBackgroundImage(background);
        return background;
    }
    return background_model_.clone();
}
#endif

/**
 * @brief Video karesini işler
 */
GstBuffer* MotionDetector::processFrame(GstBuffer* buffer, const GstVideoInfo* info) {
    if (!enabled_) {
        return buffer;
    }
    
    // İstatistikleri güncelle
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_frames++;
    }
    
#ifdef HAVE_OPENCV
    // Buffer'ı OpenCV Mat'e dönüştür
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READWRITE)) {
        return buffer;
    }
    
    int width = GST_VIDEO_INFO_WIDTH(info);
    int height = GST_VIDEO_INFO_HEIGHT(info);
    
    cv::Mat frame;
    
    // Video formatına göre dönüştür
    switch (GST_VIDEO_INFO_FORMAT(info)) {
        case GST_VIDEO_FORMAT_I420:
            {
                cv::Mat yuv(height + height/2, width, CV_8UC1, map.data);
                cv::cvtColor(yuv, frame, cv::COLOR_YUV2BGR_I420);
            }
            break;
            
        case GST_VIDEO_FORMAT_RGB:
            frame = cv::Mat(height, width, CV_8UC3, map.data).clone();
            cv::cvtColor(frame, frame, cv::COLOR_RGB2BGR);
            break;
            
        case GST_VIDEO_FORMAT_BGR:
            frame = cv::Mat(height, width, CV_8UC3, map.data).clone();
            break;
            
        default:
            gst_buffer_unmap(buffer, &map);
            return buffer;
    }
    
    // Hareket algıla
    std::vector<MotionRegion> regions = detectMotion(frame);
    
    // Hareket bölgelerini sakla
    {
        std::lock_guard<std::mutex> lock(motions_mutex_);
        current_motions_ = regions;
    }
    
    // Hareket geçmişini güncelle
    motion_history_.push_back(regions);
    if (motion_history_.size() > max_history_size_) {
        motion_history_.pop_front();
    }
    
    // İstatistikleri güncelle
    if (!regions.empty()) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.motion_frames++;
        stats_.last_motion_time = std::chrono::steady_clock::now();
        stats_.total_regions += regions.size();
        
        // Hareket alanını hesapla
        double total_area = 0;
        for (const auto& region : regions) {
            total_area += region.width * region.height;
        }
        double motion_percentage = (total_area / (width * height)) * 100.0;
        
        stats_.average_motion_area = 
            (stats_.average_motion_area * (stats_.motion_frames - 1) + motion_percentage) 
            / stats_.motion_frames;
        
        stats_.max_motion_area = std::max(stats_.max_motion_area, motion_percentage);
        
        // Hareket olayını tetikle
        if (motion_percentage > params_.alert_threshold) {
            triggerMotionEvent(regions, GST_BUFFER_PTS(buffer));
        }
    }
    
    // Görselleştirme
    if (params_.draw_motion_regions || params_.show_debug_view) {
        drawMotionRegions(frame, regions);
        
        // Debug görünümü
        if (params_.show_debug_view && !motion_mask_.empty()) {
            cv::Mat debug_view;
            cv::cvtColor(motion_mask_, debug_view, cv::COLOR_GRAY2BGR);
            
            // Ekranı böl: sol taraf orijinal, sağ taraf hareket maskesi
            cv::Mat combined(height, width * 2, CV_8UC3);
            frame.copyTo(combined(cv::Rect(0, 0, width, height)));
            debug_view.copyTo(combined(cv::Rect(width, 0, width, height)));
            frame = combined;
        }
    }
    
    // Frame'i buffer'a geri yaz
    switch (GST_VIDEO_INFO_FORMAT(info)) {
        case GST_VIDEO_FORMAT_I420:
            {
                cv::Mat yuv;
                cv::cvtColor(frame, yuv, cv::COLOR_BGR2YUV_I420);
                memcpy(map.data, yuv.data, map.size);
            }
            break;
            
        case GST_VIDEO_FORMAT_RGB:
            {
                cv::Mat rgb;
                cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
                memcpy(map.data, rgb.data, map.size);
            }
            break;
            
        case GST_VIDEO_FORMAT_BGR:
            memcpy(map.data, frame.data, map.size);
            break;
            
        default:
            break;
    }
    
    gst_buffer_unmap(buffer, &map);
#endif // HAVE_OPENCV
    
    return buffer;
}

#ifdef HAVE_OPENCV
/**
 * @brief OpenCV ile hareket algıla
 */
std::vector<MotionRegion> MotionDetector::detectMotion(const cv::Mat& frame) {
    cv::Mat mask;
    
    // Algoritma seçimi
    switch (params_.algorithm) {
        case MotionAlgorithm::FRAME_DIFF:
            mask = frameDifference(frame);
            break;
            
        case MotionAlgorithm::BACKGROUND_SUB:
        case MotionAlgorithm::MOG2:
        case MotionAlgorithm::KNN:
            mask = backgroundSubtraction(frame);
            break;
            
        case MotionAlgorithm::OPTICAL_FLOW:
            // TODO: Optik akış implementasyonu
            mask = cv::Mat::zeros(frame.size(), CV_8UC1);
            break;
            
        default:
            mask = cv::Mat::zeros(frame.size(), CV_8UC1);
            break;
    }
    
    // ROI varsa uygula
    if (has_roi_) {
        cv::Mat roi_mask = cv::Mat::zeros(mask.size(), CV_8UC1);
        roi_mask(roi_) = 255;
        cv::bitwise_and(mask, roi_mask, mask);
    }
    
    // Hariç tutma bölgelerini uygula
    for (const auto& zone : exclusion_zones_) {
        mask(zone) = 0;
    }
    
    // Morfolojik işlemler
    applyMorphology(mask);
    
    // Hareket maskesini sakla
    motion_mask_ = mask.clone();
    
    // Hareket bölgelerini bul
    std::vector<MotionRegion> regions = findMotionRegions(mask);
    
    // Bölgeleri filtrele
    return filterRegions(regions);
}

/**
 * @brief Kare farkı algoritması
 */
cv::Mat MotionDetector::frameDifference(const cv::Mat& frame) {
    cv::Mat gray, diff, mask;
    
    // Gri tonlamaya çevir
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    
    // İlk kare ise sakla
    if (previous_frame_.empty()) {
        previous_frame_ = gray.clone();
        return cv::Mat::zeros(frame.size(), CV_8UC1);
    }
    
    // Kare farkını hesapla
    cv::absdiff(previous_frame_, gray, diff);
    
    // Eşikleme
    cv::threshold(diff, mask, params_.threshold, 255, cv::THRESH_BINARY);
    
    // Önceki kareyi güncelle
    previous_frame_ = gray.clone();
    
    return mask;
}

/**
 * @brief Arka plan çıkarma algoritması
 */
cv::Mat MotionDetector::backgroundSubtraction(const cv::Mat& frame) {
    cv::Mat mask;
    
    if (bg_subtractor_) {
        // OpenCV arka plan çıkarıcı kullan
        bg_subtractor_->apply(frame, mask, params_.learning_rate);
        
        // Gölgeleri kaldır (eğer etkinse)
        if (params_.detect_shadows) {
            cv::threshold(mask, mask, 128, 255, cv::THRESH_BINARY);
        }
    } else {
        // Basit arka plan modeli
        if (background_model_.empty()) {
            background_model_ = frame.clone();
            return cv::Mat::zeros(frame.size(), CV_8UC1);
        }
        
        cv::Mat diff;
        cv::absdiff(frame, background_model_, diff);
        cv::cvtColor(diff, diff, cv::COLOR_BGR2GRAY);
        cv::threshold(diff, mask, params_.threshold, 255, cv::THRESH_BINARY);
        
        // Arka plan modelini güncelle
        cv::addWeighted(frame, params_.learning_rate, background_model_, 
                       1.0 - params_.learning_rate, 0, background_model_);
    }
    
    return mask;
}

/**
 * @brief Morfolojik işlemler uygula
 */
void MotionDetector::applyMorphology(cv::Mat& mask) {
    // Erozyon (küçük gürültüleri kaldır)
    if (params_.erosion_size > 0) {
        cv::Mat erosion_kernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE,
            cv::Size(params_.erosion_size * 2 + 1, params_.erosion_size * 2 + 1)
        );
        cv::erode(mask, mask, erosion_kernel);
    }
    
    // Dilatasyon (boşlukları doldur)
    if (params_.dilation_size > 0) {
        cv::Mat dilation_kernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE,
            cv::Size(params_.dilation_size * 2 + 1, params_.dilation_size * 2 + 1)
        );
        cv::dilate(mask, mask, dilation_kernel);
    }
    
    // Açma işlemi (opening) - küçük nesneleri kaldır
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    
    // Kapama işlemi (closing) - küçük delikleri kapat
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
}

/**
 * @brief Hareket bölgelerini bul
 */
std::vector<MotionRegion> MotionDetector::findMotionRegions(const cv::Mat& mask) {
    std::vector<MotionRegion> regions;
    std::vector<std::vector<cv::Point>> contours;
    
    // Konturları bul
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Her kontur için bölge oluştur
    for (const auto& contour : contours) {
        cv::Rect bbox = cv::boundingRect(contour);
        double area = cv::contourArea(contour);
        
        MotionRegion region;
        region.x = bbox.x;
        region.y = bbox.y;
        region.width = bbox.width;
        region.height = bbox.height;
        region.intensity = std::min(1.0, area / (params_.max_area / 2.0));
        region.timestamp = 0; // Frame timestamp'i daha sonra eklenecek
        
        regions.push_back(region);
    }
    
    return regions;
}

/**
 * @brief Hareket bölgelerini filtrele
 */
std::vector<MotionRegion> MotionDetector::filterRegions(const std::vector<MotionRegion>& regions) {
    std::vector<MotionRegion> filtered;
    
    for (const auto& region : regions) {
        // Alan filtresi
        double area = region.width * region.height;
        if (area < params_.min_area || area > params_.max_area) {
            continue;
        }
        
        // En/boy oranı filtresi
        double aspect_ratio = (double)region.width / region.height;
        if (aspect_ratio < params_.min_aspect_ratio || 
            aspect_ratio > params_.max_aspect_ratio) {
            continue;
        }
        
        filtered.push_back(region);
    }
    
    // Takip etme sınırı
    if (params_.enable_tracking && filtered.size() > params_.max_tracked_objects) {
        // En büyük bölgeleri tut
        std::sort(filtered.begin(), filtered.end(), 
                  [](const MotionRegion& a, const MotionRegion& b) {
                      return (a.width * a.height) > (b.width * b.height);
                  });
        filtered.resize(params_.max_tracked_objects);
    }
    
    return filtered;
}

/**
 * @brief Hareket bölgelerini çiz
 */
void MotionDetector::drawMotionRegions(cv::Mat& frame, const std::vector<MotionRegion>& regions) {
    for (const auto& region : regions) {
        // Yoğunluğa göre renk belirle
        cv::Scalar color;
        if (region.intensity > 0.7) {
            color = cv::Scalar(0, 0, 255); // Kırmızı - yüksek hareket
        } else if (region.intensity > 0.4) {
            color = cv::Scalar(0, 165, 255); // Turuncu - orta hareket
        } else {
            color = cv::Scalar(0, 255, 0); // Yeşil - düşük hareket
        }
        
        // Dikdörtgen çiz
        cv::rectangle(frame, 
                     cv::Rect(region.x, region.y, region.width, region.height),
                     color, 2);
        
        // Bilgi metni
        std::string info = "Motion: " + 
                          std::to_string(static_cast<int>(region.intensity * 100)) + "%";
        cv::putText(frame, info, 
                   cv::Point(region.x, region.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
    }
    
    // Genel bilgiler
    if (!regions.empty()) {
        std::string status = "MOTION DETECTED - " + std::to_string(regions.size()) + " regions";
        cv::putText(frame, status,
                   cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
    }
}
#endif // HAVE_OPENCV

/**
 * @brief Transform callback
 */
static GstFlowReturn motion_detector_transform(GstBaseTransform* trans,
                                             GstBuffer* inbuf,
                                             GstBuffer* outbuf) {
    MotionDetectorElement* element = MOTION_DETECTOR(trans);
    MotionDetector* detector = element->detector;
    
    if (!detector) {
        return GST_FLOW_ERROR;
    }
    
    // Video bilgisini al
    GstVideoInfo info;
    GstCaps* caps = gst_pad_get_current_caps(GST_BASE_TRANSFORM_SINK_PAD(trans));
    if (!gst_video_info_from_caps(&info, caps)) {
        gst_caps_unref(caps);
        return GST_FLOW_ERROR;
    }
    gst_caps_unref(caps);
    
    // Kareyi işle (in-place)
    detector->processFrame(inbuf, &info);
    
    return GST_FLOW_OK;
}

/**
 * @brief Set caps callback
 */
static gboolean motion_detector_set_caps(GstBaseTransform* trans,
                                       GstCaps* incaps,
                                       GstCaps* outcaps) {
    return TRUE;
}

/**
 * @brief Hareket olayını tetikle
 */
void MotionDetector::triggerMotionEvent(const std::vector<MotionRegion>& regions, 
                                       guint64 timestamp) {
    if (!motion_callback_ || !params_.enable_alerts) {
        return;
    }
    
    // Cooldown kontrolü
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_alert_time_).count();
    
    if (elapsed < params_.alert_cooldown_ms) {
        return;
    }
    
    // Toplam hareket alanını hesapla
    double total_area = 0;
    for (const auto& region : regions) {
        total_area += region.width * region.height;
    }
    
    // Yüzde olarak hesapla (frame boyutuna göre)
    double motion_percentage = 0;
    if (element_) {
        GstCaps* caps = gst_pad_get_current_caps(
            GST_BASE_TRANSFORM_SINK_PAD(GST_BASE_TRANSFORM(element_)));
        if (caps) {
            GstVideoInfo info;
            if (gst_video_info_from_caps(&info, caps)) {
                int frame_area = GST_VIDEO_INFO_WIDTH(&info) * GST_VIDEO_INFO_HEIGHT(&info);
                motion_percentage = (total_area / frame_area) * 100.0;
            }
            gst_caps_unref(caps);
        }
    }
    
    // Callback'i çağır
    motion_callback_(regions, timestamp, motion_percentage);
    
    // Son alarm zamanını güncelle
    last_alert_time_ = now;
}