#include "camera/camera_factory.h"
#include "camera/usb_camera.h"
#include "camera/csi_camera.h"
#include "camera/gmsl_camera.h"
#include "camera/gige_camera.h"

#include <algorithm>
#include <cctype>

namespace edge {

CameraType CameraFactory::parseType(const std::string& s) {
    std::string t = s;
    std::transform(t.begin(), t.end(), t.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    if (t == "usb")  return CameraType::USB;
    if (t == "csi")  return CameraType::CSI;
    if (t == "gmsl") return CameraType::GMSL;
    if (t == "gige" || t == "gigev" || t == "aravis") return CameraType::GIGE;
    return CameraType::UNKNOWN;
}

std::string CameraFactory::typeName(CameraType t) {
    switch (t) {
        case CameraType::USB:  return "USB";
        case CameraType::CSI:  return "CSI";
        case CameraType::GMSL: return "GMSL";
        case CameraType::GIGE: return "GigE";
        default:               return "Unknown";
    }
}

CameraPtr CameraFactory::create(CameraType type,
                                const std::string& node,
                                const CameraCaps&  caps) {
    switch (type) {
        case CameraType::USB:
            return std::make_unique<UsbCamera>(
                node.empty() ? "/dev/video0" : node, caps);

        case CameraType::CSI: {
            int id = node.empty() ? 0 : std::atoi(node.c_str());
            return std::make_unique<CsiCamera>(id, caps);
        }

        case CameraType::GMSL:
            return std::make_unique<GmslCamera>(
                node.empty() ? "/dev/video0" : node,
                GmslCamera::Variant::YUV_DIRECT, caps);

        case CameraType::GIGE:
            return std::make_unique<GigeCamera>(node, caps);

        default:
            return nullptr;
    }
}

CameraPtr CameraFactory::create(const std::string& type,
                                const std::string& node,
                                const CameraCaps&  caps) {
    if (type == "auto") {
        // Probe order favors highest-performance options first.
        // CSI is Jetson-native, GMSL is automotive, GigE is industrial,
        // USB is the universal fallback.
        for (auto t : { CameraType::CSI, CameraType::GMSL,
                        CameraType::GIGE, CameraType::USB }) {
            auto cam = create(t, node, caps);
            if (cam && cam->detect()) return cam;
        }
        // No probe succeeded — return USB anyway as a best-effort default.
        return create(CameraType::USB, node, caps);
    }
    return create(parseType(type), node, caps);
}

std::vector<CameraInfo> CameraFactory::enumerateAll() {
    std::vector<CameraInfo> out;
    for (auto& v : { CsiCamera::enumerate(),
                     GmslCamera::enumerate(),
                     GigeCamera::enumerate(),
                     UsbCamera::enumerate() }) {
        out.insert(out.end(), v.begin(), v.end());
    }
    return out;
}

}  // namespace edge
