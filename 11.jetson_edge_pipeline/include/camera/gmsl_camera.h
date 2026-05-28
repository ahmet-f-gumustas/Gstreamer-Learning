#ifndef JETSON_EDGE_GMSL_CAMERA_H
#define JETSON_EDGE_GMSL_CAMERA_H

#include "camera/i_camera.h"

namespace edge {

// GMSL cameras (Leopard Imaging, e-con, D3 Engineering) come behind a
// MAX9296/MAX96712 deserializer. After kernel modules load, each link
// is presented as a /dev/videoN node.  Some vendors expose Bayer data
// requiring nvarguscamerasrc + ISP; cheaper modules give YUV directly.
//
// Two modes supported:
//   * RAW_BAYER:  nvarguscamerasrc sensor-id=N  (needs Jetson ISP)
//   * YUV_DIRECT: v4l2src device=/dev/videoN
class GmslCamera : public ICamera {
public:
    enum class Variant { RAW_BAYER, YUV_DIRECT };

    GmslCamera();
    GmslCamera(const std::string& node, Variant v, const CameraCaps& caps = {});

    bool        detect() override;
    std::string buildPipelineString() override;
    CameraType  getType() const override { return CameraType::GMSL; }
    CameraInfo  getInfo() const override;

    void setVariant(Variant v)  { variant_ = v; }
    Variant getVariant() const  { return variant_; }

    // Check for known GMSL deserializer kernel modules (max9296, max96712)
    static bool detectDeserializer();
    static std::vector<CameraInfo> enumerate();

private:
    Variant variant_ = Variant::YUV_DIRECT;
};

}  // namespace edge

#endif
