#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp>
#include <string>

// ─── Sonuç yapısı ────────────────────────────────────────────────────────────
struct DepthResult {
    cv::Mat disparity16;   // Ham disparity (16-bit, /16 = gerçek değer)
    cv::Mat disparityF;    // float disparity (piksel)
    cv::Mat depthMap;      // Derinlik haritası (metre, float)
    cv::Mat colorDepth;    // Renklendirme görüntüsü (BGR, 8-bit)
    float   minDepthM  = 0.f;
    float   maxDepthM  = 0.f;
    bool    valid      = false;
};

// ─── DepthEstimator ──────────────────────────────────────────────────────────
//
// Stereo görüntü çiftinden derinlik haritası üretir.
//
// Kalibrasyon olmadan:
//   depth = (focalPx * baselineM) / disparity
//
// Kalibrasyon ile (loadCalibration çağrıldıktan sonra):
//   Görüntüler rektifiye edilir, Q matrisi ile 3B nokta bulutu üretilir.
//
// Simülasyon modu:
//   setSim(true) çağrılırsa sağ görüntü, sol görüntünün yatay kaydırılmış
//   hali olarak üretilir. Gerçek bir stereo kamera çiftini taklit eder.
//
class DepthEstimator {
public:
    enum class Algorithm {
        BLOCK_MATCHING,    // cv::StereoBM  – hızlı, daha az hassas
        SEMI_GLOBAL_BM     // cv::StereoSGBM – yavaş, daha hassas
    };

    DepthEstimator();

    // ── Parametreler ──────────────────────────────────────────────────────────
    void setAlgorithm(Algorithm a) { algo_ = a; initMatchers(); }
    void setFocalLength(float f)   { focalPx_ = f; }
    void setBaseline(float b)      { baselineM_ = b; }
    void setMaxDepth(float d)      { maxDepthM_ = d; }

    // Simülasyon modunda sağ görüntüyü sol görüntüden üret (shift piksel)
    void setSim(bool enable, int shift = 30) { simMode_ = enable; simShift_ = shift; }

    // ── Kalibrasyon ───────────────────────────────────────────────────────────
    // Kalibrasyon YAML dosyasından yükle (OpenCV stereoCalibrate çıktısı)
    bool loadCalibration(const std::string& yamlPath);

    // ── Ana fonksiyon ─────────────────────────────────────────────────────────
    DepthResult compute(const cv::Mat& left, const cv::Mat& right);

    // Piksel koordinatındaki metresi derinliği döner (-1 = geçersiz)
    float depthAt(const DepthResult& r, int x, int y) const;

private:
    Algorithm algo_     = Algorithm::BLOCK_MATCHING;
    float focalPx_      = 554.f;    // 640x480 @ ~60° FOV
    float baselineM_    = 0.06f;    // 6 cm
    float maxDepthM_    = 10.f;     // görüntüleme üst sınırı

    bool simMode_       = false;
    int  simShift_      = 30;       // piksel

    bool calibrated_    = false;
    cv::Mat R1_, R2_, P1_, P2_, Q_;
    cv::Mat mapL1_, mapL2_, mapR1_, mapR2_;

    cv::Ptr<cv::StereoBM>   bm_;
    cv::Ptr<cv::StereoSGBM> sgbm_;

    void initMatchers();

    // Yardımcı fonksiyonlar
    void        rectify(cv::Mat& left, cv::Mat& right) const;
    cv::Mat     computeDisparity(const cv::Mat& leftGray, const cv::Mat& rightGray);
    cv::Mat     disparityToDepth(const cv::Mat& disp16);
    cv::Mat     colorize(const cv::Mat& depthM);
};
