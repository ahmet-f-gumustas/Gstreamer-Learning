#ifndef JETSON_EDGE_KALMAN_FILTER_H
#define JETSON_EDGE_KALMAN_FILTER_H

#include <opencv2/core.hpp>

namespace edge {

// Constant-velocity model in (cx, cy, a, h, vcx, vcy, va, vh)
//   cx, cy : center coordinates of the box (px)
//   a      : aspect ratio  = w / h
//   h      : height (px)
//   v*     : per-frame velocities
//
// This matches the Kalman state used by ByteTrack and SORT.
class KalmanFilter {
public:
    KalmanFilter();

    void   init(const cv::Vec4f& xyah);          // (cx, cy, a, h)
    cv::Vec4f predict();                         // returns predicted (cx, cy, a, h)
    void   update(const cv::Vec4f& measurement);

    cv::Vec4f mean()       const;                // (cx, cy, a, h)
    const cv::Matx<float, 8, 8>& covariance() const { return P_; }

private:
    cv::Matx<float, 8, 8> F_;   // state transition
    cv::Matx<float, 4, 8> H_;   // measurement
    cv::Matx<float, 8, 8> Q_;   // process noise (depends on h)
    cv::Matx<float, 4, 4> R_;   // measurement noise
    cv::Matx<float, 8, 8> P_;   // estimate cov

    cv::Vec<float, 8> x_;       // state

    float std_weight_pos_ = 1.0f / 20.0f;
    float std_weight_vel_ = 1.0f / 160.0f;
};

}  // namespace edge

#endif
