#ifndef JETSON_EDGE_USB_CAMERA_H
#define JETSON_EDGE_USB_CAMERA_H

#include "camera/i_camera.h"

namespace edge {

class UsbCamera : public ICamera {
public:
    UsbCamera();
    explicit UsbCamera(const std::string& node, const CameraCaps& caps = {});

    bool        detect() override;
    std::string buildPipelineString() override;
    CameraType  getType() const override { return CameraType::USB; }
    CameraInfo  getInfo() const override;

    // Probe /dev/video* nodes
    static std::vector<CameraInfo> enumerate();
};

}  // namespace edge

#endif
