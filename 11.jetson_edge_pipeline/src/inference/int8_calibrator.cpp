#include "inference/int8_calibrator.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <cuda_runtime_api.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

namespace edge {

Int8EntropyCalibrator2::Int8EntropyCalibrator2(int bs, int w, int h,
                                               const std::string& dir,
                                               const std::string& cache_file)
    : batch_size_(bs), input_w_(w), input_h_(h), cache_file_(cache_file) {

    if (fs::exists(dir)) {
        for (auto& e : fs::directory_iterator(dir)) {
            auto ext = e.path().extension().string();
            if (ext == ".jpg" || ext == ".png" || ext == ".jpeg")
                images_.push_back(e.path().string());
        }
        std::cout << "[INT8] Calibration set: " << images_.size()
                  << " images from " << dir << "\n";
    }

    cudaMalloc(&device_input_, batch_size_ * 3 * w * h * sizeof(float));
}

Int8EntropyCalibrator2::~Int8EntropyCalibrator2() {
    if (device_input_) cudaFree(device_input_);
}

bool Int8EntropyCalibrator2::getBatch(void* bindings[], const char* /*names*/[],
                                      int /*nb*/) noexcept {
    if (current_idx_ + batch_size_ > static_cast<int>(images_.size()))
        return false;

    std::vector<float> batch(batch_size_ * 3 * input_w_ * input_h_);
    int per_img = 3 * input_w_ * input_h_;

    for (int b = 0; b < batch_size_; ++b) {
        cv::Mat img = cv::imread(images_[current_idx_ + b]);
        if (img.empty()) continue;
        cv::Mat resized;
        cv::resize(img, resized, {input_w_, input_h_});
        cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
        resized.convertTo(resized, CV_32FC3, 1.0 / 255.0);

        std::vector<cv::Mat> chans(3);
        cv::split(resized, chans);
        for (int c = 0; c < 3; ++c) {
            std::memcpy(batch.data() + b * per_img + c * input_w_ * input_h_,
                        chans[c].data, input_w_ * input_h_ * sizeof(float));
        }
    }

    cudaMemcpy(device_input_, batch.data(), batch.size() * sizeof(float),
               cudaMemcpyHostToDevice);
    bindings[0] = device_input_;
    current_idx_ += batch_size_;

    if (current_idx_ % 50 == 0)
        std::cout << "[INT8] Calibrated " << current_idx_ << "/"
                  << images_.size() << "\n";
    return true;
}

const void* Int8EntropyCalibrator2::readCalibrationCache(size_t& length) noexcept {
    cache_.clear();
    std::ifstream f(cache_file_, std::ios::binary);
    if (!f.is_open()) { length = 0; return nullptr; }
    f.seekg(0, std::ios::end);
    cache_.resize(f.tellg());
    f.seekg(0);
    f.read(cache_.data(), cache_.size());
    length = cache_.size();
    std::cout << "[INT8] Loaded calibration cache (" << length << " bytes)\n";
    return cache_.data();
}

void Int8EntropyCalibrator2::writeCalibrationCache(const void* cache,
                                                   size_t length) noexcept {
    std::ofstream f(cache_file_, std::ios::binary);
    f.write(reinterpret_cast<const char*>(cache), length);
    std::cout << "[INT8] Wrote calibration cache (" << length << " bytes)\n";
}

}  // namespace edge
