#ifndef JETSON_EDGE_I_CAMERA_H
#define JETSON_EDGE_I_CAMERA_H

#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace edge {

enum class CameraType {
    UNKNOWN = 0,
    USB,        // UVC over v4l2src
    CSI,        // Jetson native CSI (nvarguscamerasrc)
    GMSL,       // GMSL deserializer (MAX9296/96712) exposed as v4l2
    GIGE        // GigE Vision via Aravis (aravissrc)
};

struct CameraCaps {
    int width        = 1920;
    int height       = 1080;
    int framerate    = 30;
    std::string fmt  = "NV12";   // NV12 | I420 | YUY2 | RGB | BAYER_RGGB8
    bool use_nvmm    = false;    // Jetson zero-copy memory
};

struct CameraInfo {
    CameraType  type   = CameraType::UNKNOWN;
    std::string node;            // /dev/video0, sensor-id=0, aravis name, ...
    std::string vendor;          // Logitech, Basler, Leopard, ...
    std::string model;
    CameraCaps  caps;
};

class ICamera {
public:
    virtual ~ICamera() = default;

    virtual bool       detect()                = 0;
    virtual std::string buildPipelineString()  = 0;
    virtual CameraType  getType() const        = 0;
    virtual CameraInfo  getInfo() const        = 0;

    void setCaps(const CameraCaps& c) { caps_ = c; }
    const CameraCaps& getCaps() const  { return caps_; }
    void setNode(const std::string& n) { node_ = n; }
    const std::string& getNode() const { return node_; }

protected:
    CameraCaps  caps_;
    std::string node_;
};

using CameraPtr = std::unique_ptr<ICamera>;

}  // namespace edge

#endif
