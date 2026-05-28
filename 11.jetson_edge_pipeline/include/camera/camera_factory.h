#ifndef JETSON_EDGE_CAMERA_FACTORY_H
#define JETSON_EDGE_CAMERA_FACTORY_H

#include "camera/i_camera.h"

namespace edge {

class CameraFactory {
public:
    // Create a camera of the requested type.
    // type == "auto" => probe order: CSI, GMSL, GigE, USB.
    static CameraPtr create(const std::string& type,
                            const std::string& node    = "",
                            const CameraCaps&  caps    = {});

    static CameraPtr create(CameraType type,
                            const std::string& node    = "",
                            const CameraCaps&  caps    = {});

    // Enumerate every available camera across all backends.
    static std::vector<CameraInfo> enumerateAll();

    static CameraType parseType(const std::string& s);
    static std::string typeName(CameraType t);
};

}  // namespace edge

#endif
