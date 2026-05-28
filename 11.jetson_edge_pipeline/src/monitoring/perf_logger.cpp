#include "monitoring/perf_logger.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace edge {

PerfLogger::PerfLogger() = default;

PerfLogger::~PerfLogger() { close(); }

bool PerfLogger::open(const std::string& path, int row_every) {
    path_      = path;
    row_every_ = std::max(1, row_every);
    file_.open(path_);
    if (!file_.is_open()) return false;

    file_ << "frame_id,fps,inference_ms,preproc_ms,postproc_ms,tracking_ms,"
          << "detections,active_tracks,"
          << "cpu_pct,gpu_pct,gpu_mhz,"
          << "ram_used_mb,ram_total_mb,"
          << "soc_temp_c,gpu_temp_c,cpu_temp_c,thermal_temp_c,"
          << "power_total_mw,power_gpu_mw,power_cpu_mw\n";
    return true;
}

void PerfLogger::close() {
    if (file_.is_open()) file_.close();
}

void PerfLogger::log(const PerfFrame& fr) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        history_.push_back(fr);
    }

    if (!file_.is_open()) return;
    if (++row_count_ % row_every_ != 0) return;

    file_ << fr.frame_id << ','
          << std::fixed << std::setprecision(2) << fr.fps << ','
          << fr.inference_ms << ',' << fr.preproc_ms << ','
          << fr.postproc_ms  << ',' << fr.tracking_ms << ','
          << fr.detections   << ',' << fr.active_tracks << ','
          << fr.tegra.cpu_load_pct << ',' << fr.tegra.gpu_load_pct << ','
          << fr.tegra.gpu_freq_mhz << ','
          << fr.tegra.ram_used_mb  << ',' << fr.tegra.ram_total_mb << ','
          << fr.tegra.soc_temp_c   << ',' << fr.tegra.gpu_temp_c << ','
          << fr.tegra.cpu_temp_c   << ',' << fr.tegra.thermal_temp_c << ','
          << fr.tegra.power_total_mw << ',' << fr.tegra.power_gpu_mw << ','
          << fr.tegra.power_cpu_mw   << '\n';
}

PerfLogger::Summary PerfLogger::summarize() const {
    std::lock_guard<std::mutex> lk(mtx_);
    Summary s;
    if (history_.empty()) return s;

    s.total_frames = static_cast<int>(history_.size());
    std::vector<float> inf, totals;
    inf.reserve(history_.size());
    totals.reserve(history_.size());

    float sum_fps = 0, sum_inf = 0, sum_total = 0, sum_power = 0;
    for (auto& f : history_) {
        sum_fps   += f.fps;
        sum_inf   += f.inference_ms;
        float total = f.inference_ms + f.preproc_ms + f.postproc_ms + f.tracking_ms;
        sum_total += total;
        sum_power += f.tegra.power_total_mw / 1000.f;

        s.min_fps     = std::min(s.min_fps, f.fps);
        s.max_fps     = std::max(s.max_fps, f.fps);
        s.peak_power_w = std::max(s.peak_power_w, f.tegra.power_total_mw / 1000.f);
        s.peak_temp_c  = std::max(s.peak_temp_c,  f.tegra.thermal_temp_c);

        inf.push_back(f.inference_ms);
        totals.push_back(total);
    }

    s.mean_fps      = sum_fps   / history_.size();
    s.mean_inf_ms   = sum_inf   / history_.size();
    s.mean_total_ms = sum_total / history_.size();
    s.mean_power_w  = sum_power / history_.size();

    std::sort(inf.begin(), inf.end());
    size_t i95 = static_cast<size_t>(inf.size() * 0.95f);
    if (i95 >= inf.size()) i95 = inf.size() - 1;
    s.p95_inf_ms = inf[i95];
    return s;
}

void PerfLogger::writeSummary(const std::string& json_path) const {
    auto s = summarize();
    std::ofstream f(json_path);
    if (!f.is_open()) return;
    f << "{\n"
      << "  \"total_frames\":   " << s.total_frames     << ",\n"
      << "  \"mean_fps\":       " << std::fixed << std::setprecision(2) << s.mean_fps  << ",\n"
      << "  \"min_fps\":        " << s.min_fps         << ",\n"
      << "  \"max_fps\":        " << s.max_fps         << ",\n"
      << "  \"mean_inf_ms\":    " << s.mean_inf_ms     << ",\n"
      << "  \"p95_inf_ms\":     " << s.p95_inf_ms      << ",\n"
      << "  \"mean_total_ms\":  " << s.mean_total_ms   << ",\n"
      << "  \"mean_power_w\":   " << s.mean_power_w    << ",\n"
      << "  \"peak_power_w\":   " << s.peak_power_w    << ",\n"
      << "  \"peak_temp_c\":    " << s.peak_temp_c     << "\n"
      << "}\n";
}

}  // namespace edge
