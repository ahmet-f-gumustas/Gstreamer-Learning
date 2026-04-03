#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/calib3d.hpp>
#include <string>

// ─── Result structure ────────────────────────────────────────────────────────
struct DepthResult {
    cv::Mat disparity16;   // Raw disparity (16-bit, /16 = actual value)
    cv::Mat disparityF;    // float disparity (pixels)
    cv::Mat depthMap;      // Depth map (meters, float)
    cv::Mat colorDepth;    // Colorized image (BGR, 8-bit)
    float   minDepthM  = 0.f;
    float   maxDepthM  = 0.f;
    bool    valid      = false;
};

// ─── DepthEstimator ──────────────────────────────────────────────────────────
//
// Produces a depth map from a stereo image pair.
//
// Without calibration:
//   depth = (focalPx * baselineM) / disparity
//
// With calibration (after loadCalibration is called):
//   Images are rectified, and a 3D point cloud is produced via the Q matrix.
//
// Simulation mode:
//   If setSim(true) is called, the right image is produced as a horizontally
//   shifted version of the left image. Mimics a real stereo camera pair.
//
class DepthEstimator {
public:
    enum class Algorithm {
        BLOCK_MATCHING,    // cv::StereoBM  – fast, less accurate
        SEMI_GLOBAL_BM     // cv::StereoSGBM – slow, more accurate
    };

    DepthEstimator();

    // ── Parameters ───────────────────────────────────────────────────────────
    void setAlgorithm(Algorithm a) { algo_ = a; initMatchers(); }
    void setFocalLength(float f)   { focalPx_ = f; }
    void setBaseline(float b)      { baselineM_ = b; }
    void setMaxDepth(float d)      { maxDepthM_ = d; }

    // In simulation mode, generate right image from left image (shift pixels)
    void setSim(bool enable, int shift = 30) { simMode_ = enable; simShift_ = shift; }

    // ── Calibration ──────────────────────────────────────────────────────────
    // Load from calibration YAML file (OpenCV stereoCalibrate output)
    bool loadCalibration(const std::string& yamlPath);

    // ── Main function ────────────────────────────────────────────────────────
    DepthResult compute(const cv::Mat& left, const cv::Mat& right);

    // Returns depth in meters at pixel coordinate (-1 = invalid)
    float depthAt(const DepthResult& r, int x, int y) const;

private:
    Algorithm algo_     = Algorithm::BLOCK_MATCHING;
    float focalPx_      = 554.f;    // 640x480 @ ~60° FOV
    float baselineM_    = 0.06f;    // 6 cm
    float maxDepthM_    = 10.f;     // display upper limit

    bool simMode_       = false;
    int  simShift_      = 30;       // pixels

    bool calibrated_    = false;
    cv::Mat R1_, R2_, P1_, P2_, Q_;
    cv::Mat mapL1_, mapL2_, mapR1_, mapR2_;

    cv::Ptr<cv::StereoBM>   bm_;
    cv::Ptr<cv::StereoSGBM> sgbm_;

    void initMatchers();

    // Helper functions
    void        rectify(cv::Mat& left, cv::Mat& right) const;
    cv::Mat     computeDisparity(const cv::Mat& leftGray, const cv::Mat& rightGray);
    cv::Mat     disparityToDepth(const cv::Mat& disp16);
    cv::Mat     colorize(const cv::Mat& depthM);
};
