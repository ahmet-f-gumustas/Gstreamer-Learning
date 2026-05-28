#include "camera/csi_camera.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace edge {

CsiCamera::CsiCamera() {
    sensor_id_ = 0;
    node_ = "sensor-id=0";
    caps_.use_nvmm = true;
    caps_.fmt      = "NV12";
}

CsiCamera::CsiCamera(int sensor_id, const CameraCaps& caps) {
    sensor_id_ = sensor_id;
    node_ = "sensor-id=" + std::to_string(sensor_id);
    caps_ = caps;
    caps_.use_nvmm = true;          // CSI is always NVMM
}

bool CsiCamera::isJetsonHost() {
    std::ifstream f("/proc/device-tree/model");
    if (!f.is_open()) return false;
    std::string model;
    std::getline(f, model);
    return model.find("NVIDIA") != std::string::npos ||
           model.find("Jetson") != std::string::npos ||
           model.find("Orin")   != std::string::npos ||
           model.find("Xavier") != std::string::npos;
}

bool CsiCamera::detect() {
    if (!isJetsonHost()) return false;

    // On Jetson, CSI sensors register through tegra-camera and Argus.
    // /dev/video* still appears, but the canonical probe is via
    // /proc/device-tree/tegra-camera-platform — we simply check that
    // path and trust nvarguscamerasrc to fail at PLAYING if the
    // requested sensor-id is invalid.
    fs::path dt = "/proc/device-tree/tegra-camera-platform";
    return fs::exists(dt);
}

std::string CsiCamera::buildPipelineString() {
    // nvarguscamerasrc emits NVMM NV12.  We keep the buffer in NVMM and
    // let downstream (nvvidconv ! tensorrt) consume it zero-copy.
    std::ostringstream ss;
    ss << "nvarguscamerasrc sensor-id=" << sensor_id_;
    if (exposure_ns_ > 0)
        ss << " exposuretimerange=\"" << exposure_ns_ << " " << exposure_ns_ << "\"";
    if (gain_ > 0)
        ss << " gainrange=\"" << gain_ << " " << gain_ << "\"";
    ss << " awblock=false aelock=false ";

    // Argus output is always NVMM NV12 (or I420 for some sensors).
    ss << "! video/x-raw(memory:NVMM),width=" << caps_.width
       << ",height=" << caps_.height
       << ",framerate=" << caps_.framerate << "/1"
       << ",format=NV12 ";

    // Convert to plain raw so the rest of the pipeline (non-Jetson code) can
    // still consume it. On Jetson, replace this nvvidconv with the NVMM path
    // (handled by EdgePipeline).
    ss << "! nvvidconv ! video/x-raw,format=" << caps_.fmt;
    return ss.str();
}

CameraInfo CsiCamera::getInfo() const {
    CameraInfo info;
    info.type   = CameraType::CSI;
    info.node   = node_;
    info.caps   = caps_;
    info.vendor = "NVIDIA Argus";
    info.model  = "CSI sensor-id=" + std::to_string(sensor_id_);
    return info;
}

std::vector<CameraInfo> CsiCamera::enumerate() {
    std::vector<CameraInfo> out;
    if (!isJetsonHost()) return out;

    // Walk /proc/device-tree/tegra-camera-platform/modules/module*
    fs::path base = "/proc/device-tree/tegra-camera-platform";
    if (!fs::exists(base)) return out;

    int sensor_id = 0;
    for (auto& e : fs::directory_iterator(base)) {
        std::string name = e.path().filename().string();
        if (name.find("modules") == std::string::npos) continue;
        for (auto& m : fs::directory_iterator(e.path())) {
            if (m.path().filename().string().rfind("module", 0) == 0) {
                CameraInfo info;
                info.type   = CameraType::CSI;
                info.node   = "sensor-id=" + std::to_string(sensor_id);
                info.vendor = "NVIDIA Argus";
                info.model  = m.path().filename().string();
                info.caps.use_nvmm = true;
                out.push_back(info);
                ++sensor_id;
            }
        }
    }
    return out;
}

}  // namespace edge
