#include "monitoring/orin_simulator.h"
#include "monitoring/tegrastats_parser.h"

#include <cuda_runtime.h>
#include <cmath>
#include <iostream>

namespace edge {

OrinSimulator::OrinSimulator() {
    host_ = defaultHostFromCuda();
}

DeviceSpec OrinSimulator::defaultHostFromCuda() {
    DeviceSpec d;
    int n = 0;
    cudaGetDeviceCount(&n);
    if (n <= 0) {
        // No CUDA — fall back to "Orin-like" so simulation degenerates to 1:1.
        d.name             = "CPU-only";
        d.int8_tops        = 40;
        d.fp16_tflops      = 10;
        d.mem_bandwidth_gb = 68;
        d.tdp_watts        = 15;
        return d;
    }

    cudaDeviceProp p{};
    cudaGetDeviceProperties(&p, 0);
    d.name = p.name;

    // Hard-coded headline numbers for the GPUs people commonly own.
    // For anything not in this list we approximate from SM count.
    std::string name = p.name;
    auto contains = [&](const char* s){ return name.find(s) != std::string::npos; };
    if (contains("RTX 4090")) {
        d.int8_tops = 1321; d.fp16_tflops = 165; d.mem_bandwidth_gb = 1008; d.tdp_watts = 450;
    } else if (contains("RTX 4080")) {
        d.int8_tops = 780;  d.fp16_tflops = 97;  d.mem_bandwidth_gb = 717;  d.tdp_watts = 320;
    } else if (contains("RTX 4070")) {
        d.int8_tops = 466;  d.fp16_tflops = 58;  d.mem_bandwidth_gb = 504;  d.tdp_watts = 200;
    } else if (contains("RTX 3090")) {
        d.int8_tops = 568;  d.fp16_tflops = 71;  d.mem_bandwidth_gb = 936;  d.tdp_watts = 350;
    } else if (contains("RTX 3080")) {
        d.int8_tops = 476;  d.fp16_tflops = 59;  d.mem_bandwidth_gb = 760;  d.tdp_watts = 320;
    } else if (contains("RTX 3070")) {
        d.int8_tops = 326;  d.fp16_tflops = 40;  d.mem_bandwidth_gb = 448;  d.tdp_watts = 220;
    } else if (contains("RTX 2080")) {
        d.int8_tops = 226;  d.fp16_tflops = 28;  d.mem_bandwidth_gb = 448;  d.tdp_watts = 250;
    } else if (contains("A100")) {
        d.int8_tops = 624;  d.fp16_tflops = 156; d.mem_bandwidth_gb = 1555; d.tdp_watts = 400;
    } else {
        // Generic approximation: each SM is roughly 0.5 INT8 TOPS @ 1.5 GHz
        d.int8_tops        = p.multiProcessorCount * 0.5f;
        d.fp16_tflops      = d.int8_tops / 8.f;
        d.mem_bandwidth_gb = p.memoryClockRate / 1000.f * (p.memoryBusWidth / 8.f) / 1024.f;
        d.tdp_watts        = 200;
    }
    return d;
}

void OrinSimulator::setPowerMode(PowerMode mode) {
    mode_ = mode;
    switch (mode) {
        case PowerMode::P_7W:  spec_.int8_tops = 20.0f; spec_.fp16_tflops = 5.0f;  break;
        case PowerMode::P_15W: spec_.int8_tops = 40.0f; spec_.fp16_tflops = 10.0f; break;
        case PowerMode::MAXN:  spec_.int8_tops = 50.0f; spec_.fp16_tflops = 12.5f; break;
    }
}

float OrinSimulator::estimateOrinFPS(float host_fps) const {
    if (host_.int8_tops <= 0 || host_.mem_bandwidth_gb <= 0) return host_fps;
    float compute_scale = spec_.int8_tops        / host_.int8_tops;
    float memory_scale  = spec_.mem_bandwidth_gb / host_.mem_bandwidth_gb;
    float blend         = compute_bound_ * compute_scale +
                          (1.f - compute_bound_) * memory_scale;
    return host_fps * blend;
}

float OrinSimulator::estimateOrinLatencyMs(float host_ms) const {
    float fps_h = (host_ms > 0) ? 1000.f / host_ms : 0;
    float fps_o = estimateOrinFPS(fps_h);
    return (fps_o > 0) ? 1000.f / fps_o : host_ms;
}

float OrinSimulator::estimateOrinPowerW(float utilization) const {
    float tdp = (mode_ == PowerMode::P_7W)  ? spec_.tdp_watts_7w  :
                (mode_ == PowerMode::P_15W) ? spec_.tdp_watts_15w :
                                              spec_.tdp_watts_15w * 1.5f;
    float idle = tdp * 0.25f;
    return idle + (tdp - idle) * std::clamp(utilization, 0.f, 1.f);
}

void OrinSimulator::synthesizeSample(float host_fps, float host_util,
                                     TegraSample& out) const {
    out.valid           = true;
    out.t               = std::chrono::steady_clock::now();
    out.ram_total_mb    = 7620;
    out.ram_used_mb     = 2400 + static_cast<int>(host_util * 1500);
    out.cpu_load_pct    = std::clamp(host_util * 70.f, 5.f, 95.f);
    out.gpu_load_pct    = std::clamp(host_util * 95.f, 3.f, 99.f);
    out.gpu_freq_mhz    = (mode_ == PowerMode::P_7W)  ? 408.f :
                          (mode_ == PowerMode::P_15W) ? 624.f : 918.f;
    out.emc_freq_mhz    = 2133.f;
    float power_w       = estimateOrinPowerW(host_util);
    out.power_total_mw  = power_w * 1000.f;
    out.power_gpu_mw    = power_w * 1000.f * 0.6f;
    out.power_cpu_mw    = power_w * 1000.f * 0.3f;
    // Thermal estimate: idle 38C + 1.5C per watt above idle baseline.
    float baseline_w    = (mode_ == PowerMode::P_7W) ? 3.f : 5.f;
    out.soc_temp_c      = 38.f + std::max(0.f, power_w - baseline_w) * 1.5f;
    out.gpu_temp_c      = out.soc_temp_c + 2.0f;
    out.cpu_temp_c      = out.soc_temp_c - 1.0f;
    out.thermal_temp_c  = std::max({out.soc_temp_c, out.gpu_temp_c, out.cpu_temp_c});
}

}  // namespace edge
