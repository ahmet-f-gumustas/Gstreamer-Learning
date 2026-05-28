#include "monitoring/orin_simulator.h"
#include "monitoring/tegrastats_parser.h"

#include <cassert>
#include <iostream>
#include <cmath>

using namespace edge;

static void test_scaling_monotonic() {
    OrinSimulator sim;
    DeviceSpec host;
    host.name = "RTX 4090";
    host.int8_tops = 1321; host.fp16_tflops = 165;
    host.mem_bandwidth_gb = 1008; host.tdp_watts = 450;
    sim.setHostDevice(host);

    sim.setPowerMode(PowerMode::P_7W);
    float fps_7w = sim.estimateOrinFPS(245.f);
    sim.setPowerMode(PowerMode::P_15W);
    float fps_15w = sim.estimateOrinFPS(245.f);
    sim.setPowerMode(PowerMode::MAXN);
    float fps_maxn = sim.estimateOrinFPS(245.f);

    assert(fps_7w < fps_15w);
    assert(fps_15w < fps_maxn);
    // Hepsi host'tan çok daha düşük olmalı
    assert(fps_15w < 50);   // mantıklı bir üst sınır
}

static void test_power_estimation_bounded() {
    OrinSimulator sim;
    sim.setPowerMode(PowerMode::P_15W);

    float idle   = sim.estimateOrinPowerW(0.0f);
    float full   = sim.estimateOrinPowerW(1.0f);
    float middle = sim.estimateOrinPowerW(0.5f);

    assert(idle < middle);
    assert(middle < full);
    assert(full <= 20.0f);   // 15W mode sentinel + margin
}

static void test_synth_sample_valid() {
    OrinSimulator sim;
    TegraSample s;
    sim.setPowerMode(PowerMode::P_15W);
    sim.synthesizeSample(30.f, 0.7f, s);

    assert(s.valid);
    assert(s.ram_total_mb > 0);
    assert(s.gpu_load_pct > 0 && s.gpu_load_pct <= 100);
    assert(s.power_total_mw > 0);
    assert(s.thermal_temp_c >= 30 && s.thermal_temp_c < 90);
}

int main() {
    test_scaling_monotonic();
    test_power_estimation_bounded();
    test_synth_sample_valid();
    std::cout << "test_orin_simulator: OK\n";
    return 0;
}
