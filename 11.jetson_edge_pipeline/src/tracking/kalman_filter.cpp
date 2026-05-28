#include "tracking/kalman_filter.h"
#include <cmath>

namespace edge {

KalmanFilter::KalmanFilter() {
    F_ = cv::Matx<float, 8, 8>::eye();
    for (int i = 0; i < 4; ++i) F_(i, i + 4) = 1.f;  // x' = x + v
    H_ = cv::Matx<float, 4, 8>::zeros();
    for (int i = 0; i < 4; ++i) H_(i, i) = 1.f;
    P_ = cv::Matx<float, 8, 8>::eye();
    Q_ = cv::Matx<float, 8, 8>::zeros();
    R_ = cv::Matx<float, 4, 4>::zeros();
    x_ = cv::Vec<float, 8>::all(0);
}

void KalmanFilter::init(const cv::Vec4f& xyah) {
    for (int i = 0; i < 4; ++i) {
        x_(i)     = xyah[i];
        x_(i + 4) = 0.f;
    }
    float h = xyah[3];
    float std_pos = 2 * std_weight_pos_ * h;
    float std_vel = 10 * std_weight_vel_ * h;

    P_ = cv::Matx<float, 8, 8>::zeros();
    for (int i = 0; i < 4; ++i) P_(i, i) = std_pos * std_pos;
    for (int i = 0; i < 4; ++i) P_(i + 4, i + 4) = std_vel * std_vel;
    // Aspect ratio has a tiny baseline — keeps it stable.
    P_(2, 2) = 1e-4f;
    P_(6, 6) = 1e-10f;
}

cv::Vec4f KalmanFilter::predict() {
    float h = x_(3);
    float std_pos = std_weight_pos_ * h;
    float std_vel = std_weight_vel_ * h;
    Q_ = cv::Matx<float, 8, 8>::zeros();
    for (int i = 0; i < 4; ++i) Q_(i, i)         = std_pos * std_pos;
    for (int i = 0; i < 4; ++i) Q_(i + 4, i + 4) = std_vel * std_vel;
    Q_(2, 2) = 1e-4f;
    Q_(6, 6) = 1e-10f;

    x_ = F_ * x_;
    P_ = F_ * P_ * F_.t() + Q_;

    return { x_(0), x_(1), x_(2), x_(3) };
}

void KalmanFilter::update(const cv::Vec4f& z) {
    float h = x_(3);
    float std_pos = std_weight_pos_ * h;
    R_ = cv::Matx<float, 4, 4>::zeros();
    for (int i = 0; i < 4; ++i) R_(i, i) = std_pos * std_pos;
    R_(2, 2) = 1e-2f;

    auto S = H_ * P_ * H_.t() + R_;
    auto K = P_ * H_.t() * S.inv();

    cv::Vec<float, 4> y = z - H_ * x_;
    x_ = x_ + K * y;
    P_ = (cv::Matx<float, 8, 8>::eye() - K * H_) * P_;
}

cv::Vec4f KalmanFilter::mean() const {
    return { x_(0), x_(1), x_(2), x_(3) };
}

}  // namespace edge
