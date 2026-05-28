#include "camera/camera_factory.h"
#include <cassert>
#include <iostream>

using namespace edge;

static void test_parse_type() {
    assert(CameraFactory::parseType("usb")   == CameraType::USB);
    assert(CameraFactory::parseType("CSI")   == CameraType::CSI);
    assert(CameraFactory::parseType("gmsl")  == CameraType::GMSL);
    assert(CameraFactory::parseType("gigev") == CameraType::GIGE);
    assert(CameraFactory::parseType("nope")  == CameraType::UNKNOWN);
}

static void test_create_each_type() {
    CameraCaps caps;
    caps.width = 640; caps.height = 480; caps.framerate = 30;
    caps.fmt = "BGR";

    auto usb  = CameraFactory::create("usb",  "/dev/video1", caps);
    auto csi  = CameraFactory::create("csi",  "0",            caps);
    auto gmsl = CameraFactory::create("gmsl", "/dev/video2", caps);
    auto gige = CameraFactory::create("gige", "TestCam",     caps);

    assert(usb  && usb->getType()  == CameraType::USB);
    assert(csi  && csi->getType()  == CameraType::CSI);
    assert(gmsl && gmsl->getType() == CameraType::GMSL);
    assert(gige && gige->getType() == CameraType::GIGE);
}

static void test_pipeline_strings_contain_node() {
    auto usb  = CameraFactory::create("usb",  "/dev/video3");
    auto gige = CameraFactory::create("gige", "Basler-1234");
    auto p1 = usb->buildPipelineString();
    auto p2 = gige->buildPipelineString();
    assert(p1.find("/dev/video3") != std::string::npos);
    assert(p2.find("Basler-1234") != std::string::npos);
    assert(p1.find("v4l2src")     != std::string::npos);
    assert(p2.find("aravissrc")   != std::string::npos);
}

static void test_csi_pipeline_uses_nvarguscamerasrc() {
    auto csi = CameraFactory::create("csi", "0");
    auto p = csi->buildPipelineString();
    assert(p.find("nvarguscamerasrc") != std::string::npos);
    assert(p.find("memory:NVMM")      != std::string::npos);
}

int main() {
    test_parse_type();
    test_create_each_type();
    test_pipeline_strings_contain_node();
    test_csi_pipeline_uses_nvarguscamerasrc();
    std::cout << "test_camera_factory: OK\n";
    return 0;
}
