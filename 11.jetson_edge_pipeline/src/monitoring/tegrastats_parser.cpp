#include "monitoring/tegrastats_parser.h"

#include <iostream>
#include <sstream>
#include <regex>
#include <cstdio>
#include <cstdlib>

namespace edge {

TegrastatsParser::TegrastatsParser() = default;

TegrastatsParser::~TegrastatsParser() { stop(); }

bool TegrastatsParser::start(int interval_ms) {
    if (running_) return true;

    // Detect tegrastats; on dev PC this won't exist and we silently bail.
    if (::system("which tegrastats >/dev/null 2>&1") != 0) {
        std::cerr << "[tegrastats] not found — running off-target; "
                     "parser idle (Orin simulator should drive samples).\n";
        return false;
    }

    std::string cmd = "tegrastats --interval " + std::to_string(interval_ms);
    pipe_ = ::popen(cmd.c_str(), "r");
    if (!pipe_) return false;

    running_ = true;
    thread_  = std::thread(&TegrastatsParser::readerThread, this, interval_ms);
    return true;
}

void TegrastatsParser::stop() {
    if (!running_) return;
    running_ = false;
    if (pipe_)  { ::pclose(pipe_); pipe_ = nullptr; }
    if (thread_.joinable()) thread_.join();
}

void TegrastatsParser::readerThread(int /*interval_ms*/) {
    char buf[2048];
    while (running_ && pipe_ && std::fgets(buf, sizeof(buf), pipe_)) {
        std::string line(buf);
        TegraSample s = parseLine(line);
        if (s.valid) {
            std::lock_guard<std::mutex> lk(mtx_);
            last_ = s;
        }
    }
}

// Sample line (Orin Nano, JetPack 6.0):
// 06-15-2024 10:22:01 RAM 2451/7620MB (lfb 1067x4MB) SWAP 0/3810MB (cached 0MB)
// CPU [12%@1497,8%@1497,5%@1497,3%@1497,6%@1497,4%@1497]
// EMC_FREQ 0%@2133 GR3D_FREQ 25%@621
// CV0@40C CPU@40.5C Tdiode@N.NC SOC0@40C SOC1@39C SOC2@40C tj@40.5C
// VDD_IN 4720mW/4720mW VDD_CPU_GPU_CV 1480mW/1480mW VDD_SOC 1380mW/1380mW
TegraSample TegrastatsParser::parseLine(const std::string& line) const {
    TegraSample s;

    auto find_float = [&](const std::regex& re, float& dst) {
        std::smatch m;
        if (std::regex_search(line, m, re) && m.size() >= 2) {
            dst = std::stof(m[1].str());
            return true;
        }
        return false;
    };

    int ram_used = 0, ram_total = 0;
    {
        std::regex re_ram(R"(RAM\s+(\d+)/(\d+)MB)");
        std::smatch m;
        if (std::regex_search(line, m, re_ram) && m.size() == 3) {
            ram_used  = std::stoi(m[1]);
            ram_total = std::stoi(m[2]);
        }
    }
    s.ram_used_mb  = ram_used;
    s.ram_total_mb = ram_total;

    // Average CPU load across all cores
    {
        std::regex re_cpu(R"(CPU\s+\[([^\]]+)\])");
        std::smatch m;
        if (std::regex_search(line, m, re_cpu) && m.size() == 2) {
            std::string inner = m[1];
            std::stringstream ss(inner);
            std::string tok;
            float sum = 0; int n = 0;
            while (std::getline(ss, tok, ',')) {
                auto at = tok.find('%');
                if (at != std::string::npos) {
                    sum += std::stof(tok.substr(0, at));
                    ++n;
                }
            }
            if (n > 0) s.cpu_load_pct = sum / n;
        }
    }

    {
        std::regex re_gr3d(R"(GR3D_FREQ\s+(\d+)%@(\d+))");
        std::smatch m;
        if (std::regex_search(line, m, re_gr3d) && m.size() == 3) {
            s.gpu_load_pct = std::stof(m[1]);
            s.gpu_freq_mhz = std::stof(m[2]);
        }
    }

    {
        std::regex re_emc(R"(EMC_FREQ\s+\d+%@(\d+))");
        std::smatch m;
        if (std::regex_search(line, m, re_emc) && m.size() == 2)
            s.emc_freq_mhz = std::stof(m[1]);
    }

    find_float(std::regex(R"(SOC0@(\d+\.?\d*)C)"), s.soc_temp_c);
    find_float(std::regex(R"(CPU@(\d+\.?\d*)C)"),   s.cpu_temp_c);
    find_float(std::regex(R"(GPU@(\d+\.?\d*)C)"),   s.gpu_temp_c);
    find_float(std::regex(R"(tj@(\d+\.?\d*)C)"),    s.thermal_temp_c);

    find_float(std::regex(R"(VDD_IN\s+(\d+)mW)"),         s.power_total_mw);
    find_float(std::regex(R"(VDD_CPU_GPU_CV\s+(\d+)mW)"), s.power_gpu_mw);
    find_float(std::regex(R"(VDD_SOC\s+(\d+)mW)"),        s.power_cpu_mw);

    s.t     = std::chrono::steady_clock::now();
    s.valid = (ram_total > 0);    // sanity: if we parsed RAM, line was real
    return s;
}

TegraSample TegrastatsParser::lastSample() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return last_;
}

}  // namespace edge
