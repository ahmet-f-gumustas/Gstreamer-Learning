#ifndef JETSON_EDGE_INT8_CALIBRATOR_H
#define JETSON_EDGE_INT8_CALIBRATOR_H

#include <NvInfer.h>
#include <string>
#include <vector>

namespace edge {

// Entropy calibrator (v2) that feeds the network preprocessed images
// from a flat directory.  Used during INT8 engine build.
//
// Calibration cache is written/read from cache_file so a second build
// can skip the image loop entirely.
class Int8EntropyCalibrator2 : public nvinfer1::IInt8EntropyCalibrator2 {
public:
    Int8EntropyCalibrator2(int batch_size,
                           int input_w, int input_h,
                           const std::string& images_dir,
                           const std::string& cache_file);
    ~Int8EntropyCalibrator2() override;

    int  getBatchSize() const noexcept override { return batch_size_; }
    bool getBatch(void* bindings[], const char* names[], int nb) noexcept override;
    const void* readCalibrationCache(size_t& length) noexcept override;
    void writeCalibrationCache(const void* cache, size_t length) noexcept override;

private:
    int  batch_size_;
    int  input_w_, input_h_;
    int  current_idx_  = 0;
    std::vector<std::string> images_;
    std::string cache_file_;
    std::vector<char> cache_;
    void* device_input_ = nullptr;
};

}  // namespace edge

#endif
