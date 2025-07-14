#include "input_manager.h"
#include "mosaic_layout.h"
#include <iostream>
#include <algorithm>

InputManager::InputManager(GstElement* pipeline) 
    : pipeline_(pipeline) {
}

InputManager::~InputManager() {
    // Clean up all inputs
    for (auto& input : inputs_) {
        if (input->src_pad) {
            gst_object_unref(input->src_pad);
        }
    }
}

bool InputManager::addInput(const std::string& uri, const std::string& name) {
    // Check if input with same name already exists
    auto it = std::find_if(inputs_.begin(), inputs_.end(),
        [&name](const std::shared_ptr<VideoInput>& input) {
            return input->name == name;
        });
    
    if (it != inputs_.end()) {
        std::cerr << "Input with name '" << name << "' already exists" << std::endl;
        return false;
    }
    
    // Create new input
    auto input = std::make_shared<VideoInput>();
    input->name = name;
    input->uri = uri;
    
    // Create source element
    input->source = createSourceElement(uri);
    if (!input->source) {
        std::cerr << "Failed to create source element for: " << uri << std::endl;
        return false;
    }
    
    // Create decoder (for non-test sources)
    if (uri.find("test") == std::string::npos && uri.find("v4l2") == std::string::npos) {
        input->decoder = gst_element_factory_make("decodebin", 
            (name + "-decoder").c_str());
        if (!input->decoder) {
            std::cerr << "Failed to create decoder" << std::endl;
            gst_object_unref(input->source);
            return false;
        }
    }
    
    // Create video converter
    input->converter = gst_element_factory_make("videoconvert", 
        (name + "-converter").c_str());
    if (!input->converter) {
        std::cerr << "Failed to create video converter" << std::endl;
        gst_object_unref(input->source);
        if (input->decoder) gst_object_unref(input->decoder);
        return false;
    }
    
    // Create video scaler
    input->scale = gst_element_factory_make("videoscale", 
        (name + "-scale").c_str());
    if (!input->scale) {
        std::cerr << "Failed to create video scaler" << std::endl;
        gst_object_unref(input->source);
        if (input->decoder) gst_object_unref(input->decoder);
        gst_object_unref(input->converter);
        return false;
    }
    
    // Create capsfilter for scaling
    input->capsfilter = gst_element_factory_make("capsfilter", 
        (name + "-capsfilter").c_str());
    if (!input->capsfilter) {
        std::cerr << "Failed to create capsfilter" << std::endl;
        gst_object_unref(input->source);
        if (input->decoder) gst_object_unref(input->decoder);
        gst_object_unref(input->converter);
        gst_object_unref(input->scale);
        return false;
    }
    
    // Set caps for scaling (will be updated when connected to compositor)
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "width", G_TYPE_INT, 960,
        "height", G_TYPE_INT, 540,
        nullptr);
    g_object_set(input->capsfilter, "caps", caps, nullptr);
    gst_caps_unref(caps);
    
    // Add elements to pipeline
    if (input->decoder) {
        gst_bin_add_many(GST_BIN(pipeline_), input->source, input->decoder,
                         input->converter, input->scale, input->capsfilter, nullptr);
    } else {
        gst_bin_add_many(GST_BIN(pipeline_), input->source, input->converter,
                         input->scale, input->capsfilter, nullptr);
    }
    
    // Link elements
    if (input->decoder) {
        // Link source to decoder
        if (!gst_element_link(input->source, input->decoder)) {
            std::cerr << "Failed to link source to decoder" << std::endl;
            return false;
        }
        
        // Connect pad-added signal for decoder
        g_signal_connect(input->decoder, "pad-added", 
                        G_CALLBACK(onPadAdded), input.get());
    } else {
        // Direct link for test sources and cameras
        if (!gst_element_link_many(input->source, input->converter, 
                                   input->scale, input->capsfilter, nullptr)) {
            std::cerr << "Failed to link input elements" << std::endl;
            return false;
        }
    }
    
    // Link converter to scale to capsfilter (for decoder case)
    if (input->decoder) {
        if (!gst_element_link_many(input->converter, input->scale, 
                                   input->capsfilter, nullptr)) {
            std::cerr << "Failed to link converter chain" << std::endl;
            return false;
        }
    }
    
    // Get source pad from capsfilter
    input->src_pad = gst_element_get_static_pad(input->capsfilter, "src");
    
    // Add to inputs list
    inputs_.push_back(input);
    
    std::cout << "Added input: " << name << " (" << uri << ")" << std::endl;
    
    return true;
}

bool InputManager::removeInput(const std::string& name) {
    auto it = std::find_if(inputs_.begin(), inputs_.end(),
        [&name](const std::shared_ptr<VideoInput>& input) {
            return input->name == name;
        });
    
    if (it == inputs_.end()) {
        return false;
    }
    
    auto input = *it;
    
    // Set elements to NULL state
    gst_element_set_state(input->source, GST_STATE_NULL);
    if (input->decoder) gst_element_set_state(input->decoder, GST_STATE_NULL);
    gst_element_set_state(input->converter, GST_STATE_NULL);
    gst_element_set_state(input->scale, GST_STATE_NULL);
    gst_element_set_state(input->capsfilter, GST_STATE_NULL);
    
    // Remove from pipeline
    gst_bin_remove(GST_BIN(pipeline_), input->source);
    if (input->decoder) gst_bin_remove(GST_BIN(pipeline_), input->decoder);
    gst_bin_remove(GST_BIN(pipeline_), input->converter);
    gst_bin_remove(GST_BIN(pipeline_), input->scale);
    gst_bin_remove(GST_BIN(pipeline_), input->capsfilter);
    
    // Remove from list
    inputs_.erase(it);
    
    return true;
}

void InputManager::setGridPosition(const std::string& name, int x, int y) {
    auto it = std::find_if(inputs_.begin(), inputs_.end(),
        [&name](const std::shared_ptr<VideoInput>& input) {
            return input->name == name;
        });
    
    if (it != inputs_.end()) {
        (*it)->grid_x = x;
        (*it)->grid_y = y;
    }
}

bool InputManager::connectToCompositor(GstElement* compositor) {
    for (auto& input : inputs_) {
        // Request a new sink pad from compositor
        GstPad* sink_pad = gst_element_get_request_pad(compositor, "sink_%u");
        if (!sink_pad) {
            std::cerr << "Failed to get sink pad from compositor" << std::endl;
            return false;
        }
        
        // Link the source pad to compositor sink pad
        if (gst_pad_link(input->src_pad, sink_pad) != GST_PAD_LINK_OK) {
            std::cerr << "Failed to link " << input->name << " to compositor" << std::endl;
            gst_object_unref(sink_pad);
            return false;
        }
        
        // Set position properties on the sink pad
        // These will be updated based on layout configuration
        g_object_set(sink_pad, 
            "xpos", input->grid_x * 960,
            "ypos", input->grid_y * 540,
            nullptr);
        
        gst_object_unref(sink_pad);
    }
    
    return true;
}

GstElement* InputManager::createSourceElement(const std::string& uri) {
    GstElement* source = nullptr;
    
    if (uri.find("rtsp://") == 0) {
        // RTSP source
        source = gst_element_factory_make("rtspsrc", nullptr);
        if (source) {
            g_object_set(source, "location", uri.c_str(), 
                        "latency", 0,
                        "buffer-mode", 0,
                        nullptr);
        }
    } else if (uri.find("http://") == 0 || uri.find("https://") == 0) {
        // HTTP source
        source = gst_element_factory_make("souphttpsrc", nullptr);
        if (source) {
            g_object_set(source, "location", uri.c_str(), nullptr);
        }
    } else if (uri.find("file://") == 0 || uri.find("/") == 0) {
        // File source
        source = gst_element_factory_make("filesrc", nullptr);
        if (source) {
            std::string location = uri;
            if (location.find("file://") == 0) {
                location = location.substr(7);
            }
            g_object_set(source, "location", location.c_str(), nullptr);
        }
    } else if (uri.find("v4l2src") == 0) {
        // V4L2 camera source
        source = gst_element_factory_make("v4l2src", nullptr);
        if (source) {
            // Extract device if specified
            size_t device_pos = uri.find("device=");
            if (device_pos != std::string::npos) {
                std::string device = uri.substr(device_pos + 7);
                g_object_set(source, "device", device.c_str(), nullptr);
            }
        }
    } else if (uri.find("videotestsrc") == 0) {
        // Test source
        source = gst_element_factory_make("videotestsrc", nullptr);
        if (source) {
            // Extract pattern if specified
            size_t pattern_pos = uri.find("pattern=");
            if (pattern_pos != std::string::npos) {
                std::string pattern_str = uri.substr(pattern_pos + 8);
                int pattern = 0;
                if (pattern_str == "smpte") pattern = 0;
                else if (pattern_str == "ball") pattern = 18;
                else if (pattern_str == "snow") pattern = 1;
                else if (pattern_str == "red") pattern = 4;
                else if (pattern_str == "green") pattern = 5;
                else if (pattern_str == "blue") pattern = 6;
                g_object_set(source, "pattern", pattern, nullptr);
            }
        }
    } else {
        // Try to create element by name
        source = gst_element_factory_make(uri.c_str(), nullptr);
    }
    
    return source;
}

void InputManager::onPadAdded(GstElement* element, GstPad* pad, gpointer data) {
    VideoInput* input = static_cast<VideoInput*>(data);
    
    // Check if this is a video pad
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }
    
    if (caps) {
        GstStructure* str = gst_caps_get_structure(caps, 0);
        const gchar* name = gst_structure_get_name(str);
        
        if (g_str_has_prefix(name, "video/")) {
            // Get sink pad of converter
            GstPad* sink = gst_element_get_static_pad(input->converter, "sink");
            
            // Link the pads
            if (gst_pad_link(pad, sink) == GST_PAD_LINK_OK) {
                std::cout << "Linked decoder to converter for: " << input->name << std::endl;
            } else {
                std::cerr << "Failed to link decoder to converter for: " << input->name << std::endl;
            }
            
            gst_object_unref(sink);
        }
        
        gst_caps_unref(caps);
    }
}