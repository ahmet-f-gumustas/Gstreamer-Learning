#include "OpticalFlowDetector.hpp"
#include "Utils.hpp"
#include <gst/video/video.h>

OpticalFlowDetector::OpticalFlowDetector() 
    : pipeline(nullptr), source(nullptr), convert(nullptr), appsink(nullptr), bus(nullptr),
      maxCorners(100), qualityLevel(0.01), minDistance(10.0), 
      isInitialized(false), firstFrame(true) {
}

OpticalFlowDetector::~OpticalFlowDetector() {
    stop();
}

bool OpticalFlowDetector::initialize(const std::string& sourceInput) {
    if (!setupPipeline(sourceInput)) {
        Utils::logError("Pipeline kurulumu başarısız!");
        return false;
    }
    
    isInitialized = true;
    Utils::logInfo("OpticalFlowDetector başarıyla başlatıldı");
    return true;
}

bool OpticalFlowDetector::setupPipeline(const std::string& sourceInput) {
    // Pipeline oluştur
    pipeline = gst_pipeline_new("optical-flow-pipeline");
    if (!pipeline) {
        Utils::logError("Pipeline oluşturulamadı");
        return false;
    }
    
    // Source element seç
    std::string sourceStr = sourceInput.empty() ? Utils::detectVideoSource() : sourceInput;
    
    if (sourceStr == "webcam") {
        source = gst_element_factory_make("v4l2src", "source");
        g_object_set(G_OBJECT(source), "device", "/dev/video0", nullptr);
    } else if (sourceStr == "test") {
        source = gst_element_factory_make("videotestsrc", "source");
        g_object_set(G_OBJECT(source), "pattern", 18, nullptr); // Ball pattern
    } else {
        // Dosya kaynağı
        source = gst_element_factory_make("filesrc", "source");
        g_object_set(G_OBJECT(source), "location", sourceStr.c_str(), nullptr);
    }
    
    if (!source) {
        Utils::logError("Source element oluşturulamadı");
        return false;
    }
    
    // Convert element
    convert = gst_element_factory_make("videoconvert", "convert");
    if (!convert) {
        Utils::logError("VideoConvert element oluşturulamadı");
        return false;
    }
    
    // AppSink element
    appsink = gst_element_factory_make("appsink", "appsink");
    if (!appsink) {
        Utils::logError("AppSink element oluşturulamadı");
        return false;
    }
    
    // AppSink yapılandırması
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "BGR",
                                        "width", G_TYPE_INT, 640,
                                        "height", G_TYPE_INT, 480,
                                        nullptr);
    
    g_object_set(G_OBJECT(appsink),
                 "caps", caps,
                 "emit-signals", TRUE,
                 "sync", FALSE,
                 "max-buffers", 1,
                 "drop", TRUE,
                 nullptr);
    
    gst_caps_unref(caps);
    
    // Callback bağla
    g_signal_connect(appsink, "new-sample", G_CALLBACK(onNewSample), this);
    
    // Elementleri pipeline'a ekle
    gst_bin_add_many(GST_BIN(pipeline), source, convert, appsink, nullptr);
    
    // Dosya kaynağı için decoder ekle
    if (sourceStr != "webcam" && sourceStr != "test") {
        GstElement* demux = gst_element_factory_make("qtdemux", "demux");
        GstElement* decoder = gst_element_factory_make("avdec_h264", "decoder");
        
        if (demux && decoder) {
            gst_bin_add_many(GST_BIN(pipeline), demux, decoder, nullptr);
            
            if (!gst_element_link(source, demux) ||
                !gst_element_link(decoder, convert) ||
                !gst_element_link(convert, appsink)) {
                Utils::logError("Elementler bağlanamadı (dosya modu)");
                return false;
            }
            
            // Dynamic pad linking için demux callback
            g_signal_connect(demux, "pad-added", 
                           G_CALLBACK(+[](GstElement*, GstPad* pad, gpointer data) {
                               GstElement* decoder = static_cast<GstElement*>(data);
                               GstPad* sinkpad = gst_element_get_static_pad(decoder, "sink");
                               gst_pad_link(pad, sinkpad);
                               gst_object_unref(sinkpad);
                           }), decoder);
        }
    } else {
        // Webcam veya test source için direkt bağlantı
        if (!gst_element_link_many(source, convert, appsink, nullptr)) {
            Utils::logError("Elementler bağlanamadı");
            return false;
        }
    }
    
    // Bus al
    bus = gst_element_get_bus(pipeline);
    
    return true;
}

void OpticalFlowDetector::run() {
    if (!isInitialized) {
        Utils::logError("Detector başlatılmamış!");
        return;
    }
    
    // Pipeline'ı başlat
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        Utils::logError("Pipeline başlatılamadı!");
        return;
    }
    
    Utils::logInfo("Optical Flow detection başladı. 'q' tuşuna basarak çıkabilirsiniz.");
    
    // Ana döngü
    GstMessage* msg;
    bool terminate = false;
    
    while (!terminate) {
        msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND,
                                        static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED | 
                                                                   GST_MESSAGE_ERROR | 
                                                                   GST_MESSAGE_EOS));
        
        if (msg != nullptr) {
            Utils::printGstMessage(msg);
            
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR:
                case GST_MESSAGE_EOS:
                    terminate = true;
                    break;
                default:
                    break;
            }
            gst_message_unref(msg);
        }
        
        // OpenCV window kontrol
        if (cv::waitKey(1) == 'q') {
            terminate = true;
        }
    }
}

void OpticalFlowDetector::stop() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    
    if (bus) {
        gst_object_unref(bus);
        bus = nullptr;
    }
    
    cv::destroyAllWindows();
    Utils::logInfo("OpticalFlowDetector durduruldu");
}

GstFlowReturn OpticalFlowDetector::onNewSample(GstAppSink* appsink, gpointer userData) {
    OpticalFlowDetector* detector = static_cast<OpticalFlowDetector*>(userData);
    
    GstSample* sample = gst_app_sink_pull_sample(appsink);
    if (sample) {
        cv::Mat frame = detector->gstSampleToMat(sample);
        if (!frame.empty()) {
            detector->processFrame(frame);
        }
        gst_sample_unref(sample);
    }
    
    return GST_FLOW_OK;
}

cv::Mat OpticalFlowDetector::gstSampleToMat(GstSample* sample) {
    GstCaps* caps = gst_sample_get_caps(sample);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    
    GstStructure* structure = gst_caps_get_structure(caps, 0);
    int width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);
    
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    
    cv::Mat frame(height, width, CV_8UC3, map.data);
    cv::Mat result = frame.clone();
    
    gst_buffer_unmap(buffer, &map);
    
    return result;
}

void OpticalFlowDetector::processFrame(const cv::Mat& frame) {
    currFrame = frame.clone();
    cv::cvtColor(currFrame, currGray, cv::COLOR_BGR2GRAY);
    
    if (firstFrame) {
        prevGray = currGray.clone();
        detectCorners(prevGray);
        firstFrame = false;
        return;
    }
    
    if (!prevPoints.empty()) {
        calculateOpticalFlow();
        drawOpticalFlow(currFrame);
    }
    
    // Her 30 frame'de bir yeni corner detect et
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 30 == 0 || prevPoints.size() < 10) {
        detectCorners(currGray);
        frameCount = 0;
    }
    
    // Sonraki frame için hazırla
    prevGray = currGray.clone();
    prevPoints = currPoints;
    
    // Sonucu göster
    cv::imshow("Optical Flow Detection", currFrame);
}

void OpticalFlowDetector::detectCorners(const cv::Mat& grayFrame) {
    std::vector<cv::Point2f> corners;
    
    cv::goodFeaturesToTrack(grayFrame, corners, maxCorners, qualityLevel, minDistance);
    
    if (!corners.empty()) {
        prevPoints = corners;
        Utils::logInfo("Yeni köşeler tespit edildi: " + std::to_string(corners.size()));
    }
}

void OpticalFlowDetector::calculateOpticalFlow() {
    if (prevPoints.empty()) return;
    
    currPoints.clear();
    status.clear();
    errors.clear();
    
    cv::calcOpticalFlowPyrLK(prevGray, currGray, prevPoints, currPoints, status, errors);
    
    // Geçerli noktaları filtrele
    std::vector<cv::Point2f> goodNew, goodOld;
    for (size_t i = 0; i < currPoints.size(); i++) {
        if (status[i] == 1) {
            goodNew.push_back(currPoints[i]);
            goodOld.push_back(prevPoints[i]);
        }
    }
    
    currPoints = goodNew;
    prevPoints = goodOld;
}

void OpticalFlowDetector::drawOpticalFlow(cv::Mat& frame) {
    // Hareket vektörlerini çiz
    for (size_t i = 0; i < currPoints.size(); i++) {
        cv::Point2f movement = currPoints[i] - prevPoints[i];
        float magnitude = cv::norm(movement);
        
        if (magnitude > 1.0) { // Minimum hareket eşiği
            // Hareket vektörü
            cv::arrowedLine(frame, prevPoints[i], currPoints[i], 
                           cv::Scalar(0, 255, 0), 2, 8, 0, 0.3);
            
            // Nokta
            cv::circle(frame, currPoints[i], 3, cv::Scalar(0, 0, 255), -1);
        }
    }
    
    // Bilgi metni
    std::string info = "Tracked Points: " + std::to_string(currPoints.size());
    cv::putText(frame, info, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, 
               cv::Scalar(255, 255, 255), 2);
}