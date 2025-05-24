#ifndef OPTICAL_FLOW_DETECTOR_HPP
#define OPTICAL_FLOW_DETECTOR_HPP

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <memory>
#include <string>

class OpticalFlowDetector {
public:
    OpticalFlowDetector();
    ~OpticalFlowDetector();
    
    bool initialize(const std::string& source = "");
    void run();
    void stop();
    
private:
    // GStreamer elements
    GstElement* pipeline;
    GstElement* source;
    GstElement* convert;
    GstElement* appsink;
    GstBus* bus;
    
    // OpenCV variables
    cv::Mat prevFrame;
    cv::Mat currFrame;
    cv::Mat prevGray;
    cv::Mat currGray;
    std::vector<cv::Point2f> prevPoints;
    std::vector<cv::Point2f> currPoints;
    std::vector<uchar> status;
    std::vector<float> errors;
    
    // Configuration
    int maxCorners;
    double qualityLevel;
    double minDistance;
    bool isInitialized;
    bool firstFrame;
    
    // Private methods
    bool setupPipeline(const std::string& source);
    static GstFlowReturn onNewSample(GstAppSink* appsink, gpointer userData);
    void processFrame(const cv::Mat& frame);
    void detectCorners(const cv::Mat& grayFrame);
    void calculateOpticalFlow();
    void drawOpticalFlow(cv::Mat& frame);
    cv::Mat gstSampleToMat(GstSample* sample);
};

#endif // OPTICAL_FLOW_DETECTOR_HPP