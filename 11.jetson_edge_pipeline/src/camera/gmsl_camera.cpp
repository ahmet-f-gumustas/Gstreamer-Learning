#include "camera/gmsl_camera.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace edge {

GmslCamera::GmslCamera() {
    node_   = "/dev/video0";
    caps_.use_nvmm = true;
}

GmslCamera::GmslCamera(const std::string& node, Variant v, const CameraCaps& caps) {
    node_     = node;
    variant_  = v;
    caps_     = caps;
    caps_.use_nvmm = true;
}

bool GmslCamera::detectDeserializer() {
    // Probe /proc/modules for known GMSL deserializer kernel modules.
    std::ifstream f("/proc/modules");
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("max9296", 0)  == 0) return true;
        if (line.rfind("max96712", 0) == 0) return true;
        if (line.rfind("max9286", 0)  == 0) return true;
    }
    return false;
}

bool GmslCamera::detect() {
    // Two things must hold: deserializer module loaded AND v4l2 node present.
    if (!detectDeserializer()) return false;
    struct stat st;
    return ::stat(node_.c_str(), &st) == 0 && S_ISCHR(st.st_mode);
}

std::string GmslCamera::buildPipelineString() {
    std::ostringstream ss;

    if (variant_ == Variant::RAW_BAYER) {
        // Bayer GMSL: needs Jetson ISP via nvarguscamerasrc
        ss << "nvarguscamerasrc sensor-id=" << (node_.back() - '0')
           << " ! video/x-raw(memory:NVMM),width="
           << caps_.width << ",height=" << caps_.height
           << ",framerate=" << caps_.framerate << "/1,format=NV12 "
           << "! nvvidconv ! video/x-raw,format=" << caps_.fmt;
    } else {
        // YUV-direct GMSL: deserializer presents YUYV/UYVY via v4l2
        ss << "v4l2src device=" << node_ << " io-mode=4 ! "
           << "video/x-raw,width="   << caps_.width
           << ",height="             << caps_.height
           << ",framerate="          << caps_.framerate << "/1"
           << ",format=UYVY ! "
           << "videoconvert ! video/x-raw,format=" << caps_.fmt;
    }
    return ss.str();
}

CameraInfo GmslCamera::getInfo() const {
    CameraInfo info;
    info.type   = CameraType::GMSL;
    info.node   = node_;
    info.caps   = caps_;
    info.vendor = "GMSL";
    info.model  = (variant_ == Variant::RAW_BAYER) ? "Bayer (ISP)" : "YUV direct";
    return info;
}

std::vector<CameraInfo> GmslCamera::enumerate() {
    std::vector<CameraInfo> out;
    if (!detectDeserializer()) return out;

    // After deserializer init, each link appears as /dev/videoN.
    // Walk video nodes whose sysfs name contains "max9*" or "gmsl".
    for (int i = 0; i < 16; ++i) {
        std::string node = "/dev/video" + std::to_string(i);
        struct stat st;
        if (::stat(node.c_str(), &st) != 0) continue;

        std::ifstream f("/sys/class/video4linux/video" + std::to_string(i) + "/name");
        std::string name;
        if (f.is_open()) std::getline(f, name);

        bool is_gmsl = name.find("max9") != std::string::npos ||
                       name.find("gmsl") != std::string::npos ||
                       name.find("ar0231") != std::string::npos ||
                       name.find("imx390") != std::string::npos;
        if (!is_gmsl) continue;

        CameraInfo info;
        info.type   = CameraType::GMSL;
        info.node   = node;
        info.vendor = "GMSL";
        info.model  = name;
        info.caps.use_nvmm = true;
        out.push_back(info);
    }
    return out;
}

}  // namespace edge
