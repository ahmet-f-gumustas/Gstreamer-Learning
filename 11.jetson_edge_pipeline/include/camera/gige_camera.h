#ifndef JETSON_EDGE_GIGE_CAMERA_H
#define JETSON_EDGE_GIGE_CAMERA_H

#include "camera/i_camera.h"

namespace edge {

// GigE Vision cameras (Basler ace, FLIR Blackfly S, Allied Vision Mako).
// Uses the open-source Aravis library (aravissrc).  The node_ field is
// interpreted as the Aravis camera name (e.g. "Basler-acA1920-40uc").
//
// Empty node_ means: pick the first available camera.
class GigeCamera : public ICamera {
public:
    GigeCamera();
    explicit GigeCamera(const std::string& name, const CameraCaps& caps = {});

    bool        detect() override;
    std::string buildPipelineString() override;
    CameraType  getType() const override { return CameraType::GIGE; }
    CameraInfo  getInfo() const override;

    void setPacketSize(int sz)     { packet_size_ = sz; }
    void setAutoExposure(bool en)  { auto_exposure_ = en; }
    void setExposureUs(int us)     { exposure_us_ = us; }

    static std::vector<CameraInfo> enumerate();

private:
    int  packet_size_   = 1500;   // jumbo => 8192
    bool auto_exposure_ = true;
    int  exposure_us_   = 10000;
};

}  // namespace edge

#endif
