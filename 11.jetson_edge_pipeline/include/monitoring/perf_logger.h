#ifndef JETSON_EDGE_PERF_LOGGER_H
#define JETSON_EDGE_PERF_LOGGER_H

#include "monitoring/tegrastats_parser.h"

#include <string>
#include <fstream>
#include <chrono>
#include <vector>
#include <mutex>

namespace edge {

struct PerfFrame {
    int    frame_id      = 0;
    float  fps           = 0;
    float  inference_ms  = 0;
    float  preproc_ms    = 0;
    float  postproc_ms   = 0;
    float  tracking_ms   = 0;
    int    detections    = 0;
    int    active_tracks = 0;
    TegraSample tegra;
};

// Writes one CSV row per frame (or every N frames) plus a JSON summary
// at the end.  The format is intentionally flat so it loads into pandas
// or DuckDB without massaging.
class PerfLogger {
public:
    PerfLogger();
    ~PerfLogger();

    bool open(const std::string& csv_path, int row_every = 1);
    void close();

    void log(const PerfFrame& fr);

    // Aggregate stats for the JSON summary at end-of-run.
    struct Summary {
        int    total_frames     = 0;
        float  mean_fps         = 0;
        float  min_fps          = 1e9f;
        float  max_fps          = 0;
        float  mean_inf_ms      = 0;
        float  p95_inf_ms       = 0;
        float  mean_total_ms    = 0;
        float  mean_power_w     = 0;
        float  peak_power_w     = 0;
        float  peak_temp_c      = 0;
    };

    Summary summarize() const;
    void    writeSummary(const std::string& json_path) const;

private:
    std::ofstream file_;
    std::string   path_;
    int           row_every_ = 1;
    int           row_count_ = 0;

    mutable std::mutex   mtx_;
    std::vector<PerfFrame> history_;
};

}  // namespace edge

#endif
