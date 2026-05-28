#ifndef JETSON_EDGE_TEGRASTATS_PARSER_H
#define JETSON_EDGE_TEGRASTATS_PARSER_H

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

namespace edge {

struct TegraSample {
    std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
    int   ram_used_mb     = 0;
    int   ram_total_mb    = 0;
    float cpu_load_pct    = 0;     // averaged across cores
    float gpu_load_pct    = 0;
    float gpu_freq_mhz    = 0;
    float emc_freq_mhz    = 0;     // memory controller
    float soc_temp_c      = 0;
    float gpu_temp_c      = 0;
    float cpu_temp_c      = 0;
    float thermal_temp_c  = 0;
    float power_total_mw  = 0;     // VDD_IN
    float power_gpu_mw    = 0;     // VDD_GPU_SOC
    float power_cpu_mw    = 0;     // VDD_CPU_CV
    bool  valid           = false;
};

// Spawns `tegrastats` and parses its line-oriented output.
//
// If tegrastats is not on the host (i.e. we're on a dev PC), the reader
// silently emits empty samples and `lastSample().valid` stays false.
// The Orin simulator is responsible for synthesising plausible values
// when running off-target.
class TegrastatsParser {
public:
    TegrastatsParser();
    ~TegrastatsParser();

    bool start(int interval_ms = 1000);
    void stop();

    TegraSample lastSample() const;
    bool        isRunning() const { return running_; }

private:
    void readerThread(int interval_ms);
    TegraSample parseLine(const std::string& line) const;

    std::atomic<bool>      running_{false};
    std::thread            thread_;
    mutable std::mutex     mtx_;
    TegraSample            last_;
    std::FILE*             pipe_ = nullptr;
};

}  // namespace edge

#endif
