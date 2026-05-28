#include "tracking/kalman_filter.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace edge;

static void test_constant_velocity_track() {
    KalmanFilter kf;
    kf.init({100, 100, 1.0f, 50.f});

    // 10 frame boyunca sağa doğru kayan kutu — Kalman bir sonraki konumu
    // ölçümden önce yaklaşık olarak tahmin etmeli.
    float dx = 5.f;
    for (int t = 1; t <= 20; ++t) {
        auto pred = kf.predict();
        kf.update({100.f + t * dx, 100.f, 1.0f, 50.f});

        if (t > 5) {
            // 5. frame sonrası tahmin hatası küçük olmalı
            float err = std::abs(pred[0] - (100.f + t * dx));
            assert(err < 5.0f);
        }
    }
}

static void test_aspect_stable_under_noise() {
    KalmanFilter kf;
    kf.init({200, 200, 0.5f, 100.f});

    // h sabit, ama gözlemlere küçük noise ekleyelim.
    for (int t = 0; t < 30; ++t) {
        kf.predict();
        float noise = (t % 2 ? -1.f : 1.f) * 0.5f;
        kf.update({200.f + noise, 200.f + noise, 0.5f, 100.f + noise});
    }
    auto m = kf.mean();
    assert(std::abs(m[2] - 0.5f) < 0.05f);  // aspect drift küçük
    assert(std::abs(m[3] - 100.f) < 5.0f);   // height drift küçük
}

int main() {
    test_constant_velocity_track();
    test_aspect_stable_under_noise();
    std::cout << "test_kalman: OK\n";
    return 0;
}
