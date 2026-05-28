#ifndef JETSON_EDGE_CSI_CAMERA_H
#define JETSON_EDGE_CSI_CAMERA_H

#include "camera/i_camera.h"

namespace edge {

// CSI cameras on Jetson use the Argus stack via nvarguscamerasrc.
// Pipeline yields native NVMM buffers — zero-copy to TensorRT.
//
// node_ field is interpreted as sensor-id (0, 1, ...) for CSI.
class CsiCamera : public ICamera {
public:
    CsiCamera();
    explicit CsiCamera(int sensor_id, const CameraCaps& caps = {});

    bool        detect() override;
    std::string buildPipelineString() override;
    CameraType  getType() const override { return CameraType::CSI; }
    CameraInfo  getInfo() const override;

    void setSensorId(int id) { sensor_id_ = id; node_ = "sensor-id=" + std::to_string(id); }
    int  getSensorId() const { return sensor_id_; }

    // Jetson detection via /proc/device-tree/model
    static bool isJetsonHost();
    static std::vector<CameraInfo> enumerate();

private:
    int  sensor_id_     = 0;
    int  exposure_ns_   = 0;    // 0 => auto
    int  gain_          = 0;    // 0 => auto
    int  awb_mode_      = 1;    // 1 = auto
};

}  // namespace edge

#endif
