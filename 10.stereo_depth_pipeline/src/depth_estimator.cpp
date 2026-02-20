#include "depth_estimator.h"
#include <iostream>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
DepthEstimator::DepthEstimator()
{
    initMatchers();
}

// ─── Matcher başlatma ────────────────────────────────────────────────────────
void DepthEstimator::initMatchers()
{
    // ── StereoBM ──────────────────────────────────────────────────────────────
    // numDisparities: 16'nın katı olmalı (ne kadar büyük → daha geniş derinlik aralığı)
    // blockSize: tek sayı, 5–51 arası (büyük → pürüzsüz ama kenarlar kayar)
    bm_ = cv::StereoBM::create(/*numDisparities=*/64, /*blockSize=*/15);
    bm_->setPreFilterCap(31);
    bm_->setMinDisparity(0);
    bm_->setTextureThreshold(10);
    bm_->setUniquenessRatio(15);
    bm_->setSpeckleWindowSize(100);
    bm_->setSpeckleRange(32);

    // ── StereoSGBM ────────────────────────────────────────────────────────────
    // P1, P2: düzlük cezaları; P2 > P1 tutulmalı
    int win = 5;
    sgbm_ = cv::StereoSGBM::create(
        /*minDisparity=*/0,
        /*numDisparities=*/64,
        /*blockSize=*/win,
        /*P1=*/8  * 3 * win * win,
        /*P2=*/32 * 3 * win * win,
        /*disp12MaxDiff=*/1,
        /*preFilterCap=*/63,
        /*uniquenessRatio=*/10,
        /*speckleWindowSize=*/100,
        /*speckleRange=*/32,
        /*mode=*/cv::StereoSGBM::MODE_SGBM_3WAY
    );
}

// ─── Kalibrasyon yükleme ─────────────────────────────────────────────────────
bool DepthEstimator::loadCalibration(const std::string& yamlPath)
{
    cv::FileStorage fs(yamlPath, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "[DepthEstimator] Kalibrasyon dosyası açılamadı: " << yamlPath << "\n";
        return false;
    }

    cv::Mat M1, D1, M2, D2, R, T;
    fs["M1"] >> M1;  fs["D1"] >> D1;
    fs["M2"] >> M2;  fs["D2"] >> D2;
    fs["R"]  >> R;   fs["T"]  >> T;

    if (M1.empty() || M2.empty()) {
        std::cerr << "[DepthEstimator] Kalibrasyon matrisleri eksik!\n";
        return false;
    }

    cv::Size imgSize(640, 480);
    cv::stereoRectify(M1, D1, M2, D2, imgSize, R, T,
                      R1_, R2_, P1_, P2_, Q_,
                      cv::CALIB_ZERO_DISPARITY, /*alpha=*/0, imgSize);

    cv::initUndistortRectifyMap(M1, D1, R1_, P1_, imgSize, CV_16SC2, mapL1_, mapL2_);
    cv::initUndistortRectifyMap(M2, D2, R2_, P2_, imgSize, CV_16SC2, mapR1_, mapR2_);

    // Focal length ve baseline'ı Q matrisinden çıkar
    focalPx_   = static_cast<float>(Q_.at<double>(2, 3));
    baselineM_ = static_cast<float>(-1.0 / Q_.at<double>(3, 2));

    calibrated_ = true;
    std::cout << "[DepthEstimator] Kalibrasyon yüklendi. f=" << focalPx_
              << " px, baseline=" << baselineM_ * 100.f << " cm\n";
    return true;
}

// ─── Ana hesaplama ────────────────────────────────────────────────────────────
DepthResult DepthEstimator::compute(const cv::Mat& left, const cv::Mat& right)
{
    DepthResult result;
    if (left.empty() || right.empty()) return result;

    cv::Mat L = left.clone();
    cv::Mat R = right.clone();

    // Simülasyon modunda sağ görüntüyü sol görüntüden türet
    if (simMode_) {
        // Sağa simShift_ piksel kaydırma: sol piksel aslında daha sağdan geliyor
        cv::Mat M = (cv::Mat_<double>(2, 3) << 1, 0, -simShift_, 0, 1, 0);
        cv::warpAffine(L, R, M, L.size());
    }

    // Kalibrasyon varsa rektifikasyon uygula
    if (calibrated_) {
        rectify(L, R);
    }

    // Gri tonlamaya çevir (StereoBM/SGBM gri giriş ister)
    cv::Mat grayL, grayR;
    cv::cvtColor(L, grayL, cv::COLOR_BGR2GRAY);
    cv::cvtColor(R, grayR, cv::COLOR_BGR2GRAY);

    // Disparity hesapla
    result.disparity16 = computeDisparity(grayL, grayR);

    // float disparity (gerçek piksel değeri)
    result.disparity16.convertTo(result.disparityF, CV_32F, 1.0 / 16.0);

    // Derinlik haritası (metre)
    result.depthMap = disparityToDepth(result.disparity16);

    // İstatistik
    cv::Mat validMask = result.depthMap > 0.01f;
    if (cv::countNonZero(validMask) > 0) {
        double mn, mx;
        cv::minMaxLoc(result.depthMap, &mn, &mx, nullptr, nullptr, validMask);
        result.minDepthM = static_cast<float>(mn);
        result.maxDepthM = static_cast<float>(mx);
    }

    // Renk görüntüsü
    result.colorDepth = colorize(result.depthMap);
    result.valid = true;
    return result;
}

// ─── depthAt ─────────────────────────────────────────────────────────────────
float DepthEstimator::depthAt(const DepthResult& r, int x, int y) const
{
    if (!r.valid || r.depthMap.empty()) return -1.f;
    if (x < 0 || y < 0 || x >= r.depthMap.cols || y >= r.depthMap.rows) return -1.f;
    return r.depthMap.at<float>(y, x);
}

// ─── Rektifikasyon ───────────────────────────────────────────────────────────
void DepthEstimator::rectify(cv::Mat& left, cv::Mat& right) const
{
    cv::Mat rL, rR;
    cv::remap(left,  rL, mapL1_, mapL2_, cv::INTER_LINEAR);
    cv::remap(right, rR, mapR1_, mapR2_, cv::INTER_LINEAR);
    left  = rL;
    right = rR;
}

// ─── Disparity hesabı ────────────────────────────────────────────────────────
cv::Mat DepthEstimator::computeDisparity(const cv::Mat& grayL, const cv::Mat& grayR)
{
    cv::Mat disp;
    if (algo_ == Algorithm::BLOCK_MATCHING) {
        bm_->compute(grayL, grayR, disp);
    } else {
        sgbm_->compute(grayL, grayR, disp);
    }
    return disp;   // CV_16S, gerçek = disp / 16
}

// ─── Disparity → Derinlik (metre) ────────────────────────────────────────────
//   depth = (focalPx * baselineM) / disparity_px
cv::Mat DepthEstimator::disparityToDepth(const cv::Mat& disp16)
{
    cv::Mat depth(disp16.size(), CV_32F, cv::Scalar(0.f));

    for (int y = 0; y < disp16.rows; ++y) {
        const int16_t* row = disp16.ptr<int16_t>(y);
        float*         out = depth.ptr<float>(y);
        for (int x = 0; x < disp16.cols; ++x) {
            float d = static_cast<float>(row[x]) / 16.f;
            if (d > 0.f) {
                float z = (focalPx_ * baselineM_) / d;
                out[x] = (z < maxDepthM_) ? z : 0.f;
            }
        }
    }
    return depth;
}

// ─── Renklendirme (JET colormap) ─────────────────────────────────────────────
cv::Mat DepthEstimator::colorize(const cv::Mat& depthM)
{
    cv::Mat norm;
    // 0 → kırmızı (yakın), maxDepthM_ → mavi (uzak) – JET tersine çevriliyor
    cv::normalize(depthM, norm, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::bitwise_not(norm, norm);   // yakın = sıcak renk

    cv::Mat colored;
    cv::applyColorMap(norm, colored, cv::COLORMAP_JET);

    // Sıfır disparity piksellerini siyah yap
    cv::Mat mask = (depthM < 0.01f);
    colored.setTo(cv::Scalar(0, 0, 0), mask);

    return colored;
}
