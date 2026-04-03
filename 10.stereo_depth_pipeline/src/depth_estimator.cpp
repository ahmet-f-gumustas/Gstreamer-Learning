#include "depth_estimator.h"
#include <iostream>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
DepthEstimator::DepthEstimator()
{
    initMatchers();
}

// ─── Matcher initialization ──────────────────────────────────────────────────
void DepthEstimator::initMatchers()
{
    // ── StereoBM ──────────────────────────────────────────────────────────────
    // numDisparities: must be a multiple of 16 (larger → wider depth range)
    // blockSize: odd number, 5–51 range (larger → smoother but edges shift)
    bm_ = cv::StereoBM::create(/*numDisparities=*/64, /*blockSize=*/15);
    bm_->setPreFilterCap(31);
    bm_->setMinDisparity(0);
    bm_->setTextureThreshold(10);
    bm_->setUniquenessRatio(15);
    bm_->setSpeckleWindowSize(100);
    bm_->setSpeckleRange(32);

    // ── StereoSGBM ────────────────────────────────────────────────────────────
    // P1, P2: smoothness penalties; P2 > P1 must hold
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

// ─── Calibration loading ─────────────────────────────────────────────────────
bool DepthEstimator::loadCalibration(const std::string& yamlPath)
{
    cv::FileStorage fs(yamlPath, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "[DepthEstimator] Could not open calibration file: " << yamlPath << "\n";
        return false;
    }

    cv::Mat M1, D1, M2, D2, R, T;
    fs["M1"] >> M1;  fs["D1"] >> D1;
    fs["M2"] >> M2;  fs["D2"] >> D2;
    fs["R"]  >> R;   fs["T"]  >> T;

    if (M1.empty() || M2.empty()) {
        std::cerr << "[DepthEstimator] Calibration matrices are missing!\n";
        return false;
    }

    cv::Size imgSize(640, 480);
    cv::stereoRectify(M1, D1, M2, D2, imgSize, R, T,
                      R1_, R2_, P1_, P2_, Q_,
                      cv::CALIB_ZERO_DISPARITY, /*alpha=*/0, imgSize);

    cv::initUndistortRectifyMap(M1, D1, R1_, P1_, imgSize, CV_16SC2, mapL1_, mapL2_);
    cv::initUndistortRectifyMap(M2, D2, R2_, P2_, imgSize, CV_16SC2, mapR1_, mapR2_);

    // Extract focal length and baseline from the Q matrix
    focalPx_   = static_cast<float>(Q_.at<double>(2, 3));
    baselineM_ = static_cast<float>(-1.0 / Q_.at<double>(3, 2));

    calibrated_ = true;
    std::cout << "[DepthEstimator] Calibration loaded. f=" << focalPx_
              << " px, baseline=" << baselineM_ * 100.f << " cm\n";
    return true;
}

// ─── Main computation ─────────────────────────────────────────────────────────
DepthResult DepthEstimator::compute(const cv::Mat& left, const cv::Mat& right)
{
    DepthResult result;
    if (left.empty() || right.empty()) return result;

    cv::Mat L = left.clone();
    cv::Mat R = right.clone();

    // In simulation mode, derive right image from left image
    if (simMode_) {
        // Shift right by simShift_ pixels: left pixel actually comes from further right
        cv::Mat M = (cv::Mat_<double>(2, 3) << 1, 0, -simShift_, 0, 1, 0);
        cv::warpAffine(L, R, M, L.size());
    }

    // Apply rectification if calibration is available
    if (calibrated_) {
        rectify(L, R);
    }

    // Convert to grayscale (StereoBM/SGBM requires grayscale input)
    cv::Mat grayL, grayR;
    cv::cvtColor(L, grayL, cv::COLOR_BGR2GRAY);
    cv::cvtColor(R, grayR, cv::COLOR_BGR2GRAY);

    // Compute disparity
    result.disparity16 = computeDisparity(grayL, grayR);

    // float disparity (actual pixel value)
    result.disparity16.convertTo(result.disparityF, CV_32F, 1.0 / 16.0);

    // Depth map (meters)
    result.depthMap = disparityToDepth(result.disparity16);

    // Statistics
    cv::Mat validMask = result.depthMap > 0.01f;
    if (cv::countNonZero(validMask) > 0) {
        double mn, mx;
        cv::minMaxLoc(result.depthMap, &mn, &mx, nullptr, nullptr, validMask);
        result.minDepthM = static_cast<float>(mn);
        result.maxDepthM = static_cast<float>(mx);
    }

    // Color image
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

// ─── Rectification ───────────────────────────────────────────────────────────
void DepthEstimator::rectify(cv::Mat& left, cv::Mat& right) const
{
    cv::Mat rL, rR;
    cv::remap(left,  rL, mapL1_, mapL2_, cv::INTER_LINEAR);
    cv::remap(right, rR, mapR1_, mapR2_, cv::INTER_LINEAR);
    left  = rL;
    right = rR;
}

// ─── Disparity computation ────────────────────────────────────────────────────
cv::Mat DepthEstimator::computeDisparity(const cv::Mat& grayL, const cv::Mat& grayR)
{
    cv::Mat disp;
    if (algo_ == Algorithm::BLOCK_MATCHING) {
        bm_->compute(grayL, grayR, disp);
    } else {
        sgbm_->compute(grayL, grayR, disp);
    }
    return disp;   // CV_16S, actual = disp / 16
}

// ─── Disparity → Depth (meters) ──────────────────────────────────────────────
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

// ─── Colorization (JET colormap) ─────────────────────────────────────────────
cv::Mat DepthEstimator::colorize(const cv::Mat& depthM)
{
    cv::Mat norm;
    // 0 → red (near), maxDepthM_ → blue (far) – JET is inverted
    cv::normalize(depthM, norm, 0, 255, cv::NORM_MINMAX, CV_8U);
    cv::bitwise_not(norm, norm);   // near = warm color

    cv::Mat colored;
    cv::applyColorMap(norm, colored, cv::COLORMAP_JET);

    // Make zero disparity pixels black
    cv::Mat mask = (depthM < 0.01f);
    colored.setTo(cv::Scalar(0, 0, 0), mask);

    return colored;
}
