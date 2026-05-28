#include "camera/usb_camera.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace edge {

UsbCamera::UsbCamera() {
    node_ = "/dev/video0";
}

UsbCamera::UsbCamera(const std::string& node, const CameraCaps& caps) {
    node_ = node;
    caps_ = caps;
}

bool UsbCamera::detect() {
    struct stat st;
    return ::stat(node_.c_str(), &st) == 0 && S_ISCHR(st.st_mode);
}

std::string UsbCamera::buildPipelineString() {
    // v4l2src device=/dev/videoX ! video/x-raw,width=W,height=H,framerate=F/1
    //   ! videoconvert ! video/x-raw,format=NV12 ! appsink
    //
    // Note: when running on Jetson with USB camera, we still want NVMM
    // upload for inference. Caller can append nvvidconv ! 'video/x-raw(memory:NVMM),format=NV12'.
    std::ostringstream ss;
    ss << "v4l2src device=" << node_ << " io-mode=2 do-timestamp=true ! ";

    // Try to negotiate hardware-friendly format; videoconvert handles the rest.
    if (caps_.fmt == "MJPG") {
        ss << "image/jpeg,width=" << caps_.width
           << ",height=" << caps_.height
           << ",framerate=" << caps_.framerate << "/1 ! ";
        ss << "jpegdec ! ";
    } else {
        ss << "video/x-raw,width=" << caps_.width
           << ",height=" << caps_.height
           << ",framerate=" << caps_.framerate << "/1 ! ";
    }

    ss << "videoconvert ! video/x-raw,format=" << caps_.fmt;
    return ss.str();
}

CameraInfo UsbCamera::getInfo() const {
    CameraInfo info;
    info.type   = CameraType::USB;
    info.node   = node_;
    info.caps   = caps_;
    info.vendor = "UVC";

    // Try to read model from sysfs (works for most UVC devices)
    if (node_.rfind("/dev/video", 0) == 0) {
        std::string idx = node_.substr(10);
        fs::path name_path = "/sys/class/video4linux/video" + idx + "/name";
        std::ifstream f(name_path);
        if (f.is_open()) {
            std::getline(f, info.model);
        }
    }
    return info;
}

std::vector<CameraInfo> UsbCamera::enumerate() {
    std::vector<CameraInfo> out;
    for (int i = 0; i < 64; ++i) {
        std::string node = "/dev/video" + std::to_string(i);
        struct stat st;
        if (::stat(node.c_str(), &st) != 0) continue;
        if (!S_ISCHR(st.st_mode))           continue;

        CameraInfo info;
        info.type = CameraType::USB;
        info.node = node;
        info.vendor = "UVC";

        std::ifstream f("/sys/class/video4linux/video" + std::to_string(i) + "/name");
        if (f.is_open()) std::getline(f, info.model);

        out.push_back(info);
    }
    return out;
}

}  // namespace edge
