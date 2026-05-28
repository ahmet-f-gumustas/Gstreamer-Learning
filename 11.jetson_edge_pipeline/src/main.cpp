#include "edge_pipeline.h"
#include "camera/camera_factory.h"

#include <yaml-cpp/yaml.h>
#include <gst/gst.h>
#include <opencv2/core.hpp>

#include <iostream>
#include <csignal>
#include <atomic>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

std::atomic<bool> g_stop{false};
edge::EdgePipeline* g_pipeline = nullptr;

void onSignal(int) {
    g_stop = true;
    if (g_pipeline) g_pipeline->stop();
}

void printUsage(const char* p) {
    std::cout <<
"Usage: " << p << " [OPTIONS]\n\n"
"Options:\n"
"  --config <yaml>     Pipeline config (default: config/pipeline.yaml)\n"
"  --camera <type>     usb | csi | gmsl | gige | auto\n"
"  --node <node>       /dev/video0 | sensor-id=0 | <Aravis name>\n"
"  --engine <path>     TensorRT engine file\n"
"  --onnx   <path>     ONNX (used if engine missing)\n"
"  --precision <p>     fp32 | fp16 | int8\n"
"  --width  <px>       Frame width  (default 1920)\n"
"  --height <px>       Frame height (default 1080)\n"
"  --fps    <hz>       Camera fps   (default 30)\n"
"  --power  <mode>     7w | 15w | maxn   (Orin simulation)\n"
"  --record <path>     Save annotated video\n"
"  --headless          Disable OpenCV display\n"
"  --list              Enumerate cameras and exit\n"
"  --benchmark         Run 1000-frame benchmark then exit\n"
"  -h, --help\n";
}

edge::Precision parsePrecision(const std::string& s) {
    if (s == "int8") return edge::Precision::INT8;
    if (s == "fp32") return edge::Precision::FP32;
    return edge::Precision::FP16;
}

edge::PowerMode parsePower(const std::string& s) {
    if (s == "7w" || s == "7W") return edge::PowerMode::P_7W;
    if (s == "maxn" || s == "MAXN") return edge::PowerMode::MAXN;
    return edge::PowerMode::P_15W;
}

bool loadYaml(const std::string& path, edge::PipelineConfig& cfg) {
    if (!fs::exists(path)) return false;
    try {
        YAML::Node y = YAML::LoadFile(path);
        if (y["camera"]) {
            cfg.camera_type = y["camera"]["type"].as<std::string>("auto");
            cfg.camera_node = y["camera"]["node"].as<std::string>("");
            if (y["camera"]["width"])
                cfg.caps.width = y["camera"]["width"].as<int>();
            if (y["camera"]["height"])
                cfg.caps.height = y["camera"]["height"].as<int>();
            if (y["camera"]["framerate"])
                cfg.caps.framerate = y["camera"]["framerate"].as<int>();
            if (y["camera"]["format"])
                cfg.caps.fmt = y["camera"]["format"].as<std::string>();
        }
        if (y["model"]) {
            cfg.engine_path = y["model"]["engine"].as<std::string>(cfg.engine_path);
            cfg.onnx_path   = y["model"]["onnx"].as<std::string>(cfg.onnx_path);
            cfg.calib_dir   = y["model"]["calib_dir"].as<std::string>(cfg.calib_dir);
            cfg.precision   = parsePrecision(y["model"]["precision"].as<std::string>("fp16"));
        }
        if (y["tracker"]) {
            cfg.tracker.track_high_thresh =
                y["tracker"]["track_high_thresh"].as<float>(0.6f);
            cfg.tracker.track_low_thresh =
                y["tracker"]["track_low_thresh"].as<float>(0.1f);
            cfg.tracker.new_track_thresh =
                y["tracker"]["new_track_thresh"].as<float>(0.7f);
            cfg.tracker.match_thresh =
                y["tracker"]["match_thresh"].as<float>(0.8f);
            cfg.tracker.track_buffer =
                y["tracker"]["track_buffer"].as<int>(30);
        }
        if (y["output"]) {
            cfg.enable_display = y["output"]["display"].as<bool>(true);
            cfg.output_video   = y["output"]["video"].as<std::string>("");
            cfg.perf_csv       = y["output"]["perf_csv"].as<std::string>("perf.csv");
            cfg.perf_json      = y["output"]["perf_json"].as<std::string>("perf_summary.json");
        }
        if (y["jetson"]) {
            cfg.orin_mode = parsePower(y["jetson"]["power_mode"].as<std::string>("15w"));
            cfg.simulate_jetson = y["jetson"]["simulate"].as<bool>(true);
        }
    } catch (const std::exception& e) {
        std::cerr << "[main] YAML parse error: " << e.what() << "\n";
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    gst_init(&argc, &argv);
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    edge::PipelineConfig cfg;
    std::string yaml_path = "config/pipeline.yaml";
    bool list_mode = false, bench_mode = false;
    int  bench_frames = 1000;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() { return (i + 1 < argc) ? argv[++i] : ""; };

        if (a == "-h" || a == "--help") { printUsage(argv[0]); return 0; }
        else if (a == "--config")    yaml_path = next();
        else if (a == "--camera")    cfg.camera_type = next();
        else if (a == "--node")      cfg.camera_node = next();
        else if (a == "--engine")    cfg.engine_path = next();
        else if (a == "--onnx")      cfg.onnx_path   = next();
        else if (a == "--precision") cfg.precision   = parsePrecision(next());
        else if (a == "--width")     cfg.caps.width  = std::stoi(next());
        else if (a == "--height")    cfg.caps.height = std::stoi(next());
        else if (a == "--fps")       cfg.caps.framerate = std::stoi(next());
        else if (a == "--power")     cfg.orin_mode = parsePower(next());
        else if (a == "--record")    cfg.output_video = next();
        else if (a == "--headless")  cfg.enable_display = false;
        else if (a == "--list")      list_mode  = true;
        else if (a == "--benchmark") bench_mode = true;
        else std::cerr << "[main] Unknown arg: " << a << "\n";
    }

    if (list_mode) {
        std::cout << "Detected cameras:\n";
        for (auto& info : edge::CameraFactory::enumerateAll()) {
            std::cout << "  [" << edge::CameraFactory::typeName(info.type) << "] "
                      << info.node << "  " << info.vendor << " " << info.model << "\n";
        }
        gst_deinit();
        return 0;
    }

    // YAML overrides defaults; CLI overrides YAML.
    loadYaml(yaml_path, cfg);

    g_pipeline = new edge::EdgePipeline();
    if (!g_pipeline->initialize(cfg)) {
        delete g_pipeline;
        gst_deinit();
        return 1;
    }

    if (bench_mode) {
        std::cout << "[main] Benchmark mode: " << bench_frames << " frames\n";
        int target = bench_frames;
        g_pipeline->setCallback({
            [&](int fid, const std::vector<edge::Detection>&) {
                if (fid >= target) { g_stop = true; g_pipeline->stop(); }
            }
        });
    }

    g_pipeline->run();
    g_pipeline->stop();
    delete g_pipeline;
    gst_deinit();

    std::cout << "[main] Done. See " << cfg.perf_csv << " and " << cfg.perf_json << "\n";
    return 0;
}
