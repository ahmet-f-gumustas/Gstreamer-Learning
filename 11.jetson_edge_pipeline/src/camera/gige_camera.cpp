#include "camera/gige_camera.h"

#include <sstream>
#include <cstdlib>
#include <array>
#include <memory>
#include <stdio.h>

namespace edge {

GigeCamera::GigeCamera() = default;

GigeCamera::GigeCamera(const std::string& name, const CameraCaps& caps) {
    node_ = name;
    caps_ = caps;
}

bool GigeCamera::detect() {
    // arv-tool-0.8 is the canonical Aravis enumeration utility.
    // If it returns non-empty, we have at least one GigE Vision camera.
    std::array<char, 256> buf{};
    std::string out;
    FILE* p = ::popen("arv-tool-0.8 2>/dev/null | head -n 4", "r");
    if (!p) return false;
    while (fgets(buf.data(), buf.size(), p) != nullptr) out += buf.data();
    ::pclose(p);

    if (out.empty()) return false;
    if (node_.empty()) return true;             // first available
    return out.find(node_) != std::string::npos;
}

std::string GigeCamera::buildPipelineString() {
    std::ostringstream ss;
    ss << "aravissrc";
    if (!node_.empty())
        ss << " camera-name=\"" << node_ << "\"";

    ss << " packet-size=" << packet_size_;
    ss << " auto-exposure=" << (auto_exposure_ ? "true" : "false");
    if (!auto_exposure_)
        ss << " exposure-time=" << exposure_us_;

    // Aravis output is typically Bayer or RGB depending on the sensor.
    // bayer2rgb / videoconvert normalizes to the requested format.
    ss << " ! video/x-bayer,format=rggb,width=" << caps_.width
       << ",height=" << caps_.height
       << ",framerate=" << caps_.framerate << "/1 "
       << "! bayer2rgb ! videoconvert ! video/x-raw,format=" << caps_.fmt;
    return ss.str();
}

CameraInfo GigeCamera::getInfo() const {
    CameraInfo info;
    info.type   = CameraType::GIGE;
    info.node   = node_;
    info.caps   = caps_;
    info.vendor = "GigE Vision (Aravis)";
    info.model  = node_.empty() ? "(first available)" : node_;
    return info;
}

std::vector<CameraInfo> GigeCamera::enumerate() {
    std::vector<CameraInfo> out;

    std::array<char, 512> buf{};
    FILE* p = ::popen("arv-tool-0.8 2>/dev/null", "r");
    if (!p) return out;

    while (fgets(buf.data(), buf.size(), p) != nullptr) {
        std::string line(buf.data());
        if (line.empty()) continue;
        // arv-tool prints: "Basler-acA1920-40uc-12345 (192.168.1.10)"
        auto sp = line.find(' ');
        std::string name = (sp == std::string::npos) ? line : line.substr(0, sp);
        if (name.empty() || name == "\n") continue;
        if (name.back() == '\n') name.pop_back();

        CameraInfo info;
        info.type   = CameraType::GIGE;
        info.node   = name;
        info.vendor = "GigE Vision";
        info.model  = name;
        out.push_back(info);
    }
    ::pclose(p);
    return out;
}

}  // namespace edge
