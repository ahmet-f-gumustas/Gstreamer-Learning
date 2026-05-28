#ifndef JETSON_EDGE_ORIN_SIMULATOR_H
#define JETSON_EDGE_ORIN_SIMULATOR_H

#include <string>

namespace edge {

// Public Orin Nano (8 GB) headline numbers from NVIDIA whitepaper.
//   INT8: 40 TOPS  (sparse) / 20 TOPS (dense)
//   FP16: 10 TFLOPS
//   LPDDR5: 68 GB/s
//   Power modes: 7W, 15W
//
// We compare against the device we *are* running on (queried via NVML or
// hard-coded RTX 4090 numbers when NVML is unavailable) and synthesise an
// Orin-equivalent FPS.
struct OrinSimSpec {
    float int8_tops        = 40.0f;
    float fp16_tflops      = 10.0f;
    float mem_bandwidth_gb = 68.0f;
    float tdp_watts_15w    = 15.0f;
    float tdp_watts_7w     = 7.0f;
    int   model_year       = 2023;
    std::string codename   = "Orin Nano 8GB";
};

enum class PowerMode { P_7W, P_15W, MAXN };

struct DeviceSpec {
    std::string name      = "Unknown";
    float int8_tops       = 0;
    float fp16_tflops     = 0;
    float mem_bandwidth_gb = 0;
    float tdp_watts       = 0;
};

class OrinSimulator {
public:
    OrinSimulator();

    // Pick "currently active" hardware: NVML autodetect or override.
    void setHostDevice(const DeviceSpec& dev) { host_ = dev; }
    const DeviceSpec& hostDevice() const      { return host_; }

    void setPowerMode(PowerMode mode);
    PowerMode powerMode() const { return mode_; }

    // Mixed-workload scaling: most YOLOv8 work is compute-bound (~70%),
    // the rest is memory-bound.  Caller can tune ratio per network.
    void setComputeBoundFraction(float f) { compute_bound_ = f; }

    // Scale a measured-on-host FPS into the corresponding Orin Nano FPS.
    float estimateOrinFPS(float host_fps) const;

    // Scale a measured-on-host latency (ms) into Orin latency.
    float estimateOrinLatencyMs(float host_ms) const;

    // Estimate Orin power draw given a normalized utilization in [0,1].
    float estimateOrinPowerW(float utilization) const;

    // Generate plausible tegrastats sample when running off-target.
    // Caller usually feeds this into the perf logger as if it came from
    // a real Jetson, so the rest of the pipeline stays agnostic.
    void synthesizeSample(float host_fps, float host_util,
                          struct TegraSample& out) const;

    static DeviceSpec defaultHostFromCuda();   // try NVML, fallback to RTX 4090
    const OrinSimSpec& spec() const { return spec_; }

private:
    DeviceSpec  host_;
    OrinSimSpec spec_;
    PowerMode   mode_           = PowerMode::P_15W;
    float       compute_bound_  = 0.7f;
};

}  // namespace edge

#endif
