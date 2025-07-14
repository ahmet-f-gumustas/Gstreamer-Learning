#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <gst/gst.h>
#include <vector>
#include <string>
#include <memory>

struct VideoInput {
    std::string name;
    std::string uri;
    GstElement* source;
    GstElement* decoder;
    GstElement* converter;
    GstElement* scale;
    GstElement* capsfilter;
    GstPad* src_pad;
    int grid_x;
    int grid_y;
};

class InputManager {
public:
    InputManager(GstElement* pipeline);
    ~InputManager();

    bool addInput(const std::string& uri, const std::string& name);
    bool removeInput(const std::string& name);
    std::vector<std::shared_ptr<VideoInput>>& getInputs() { return inputs_; }
    
    void setGridPosition(const std::string& name, int x, int y);
    bool connectToCompositor(GstElement* compositor);

private:
    GstElement* pipeline_;
    std::vector<std::shared_ptr<VideoInput>> inputs_;
    
    GstElement* createSourceElement(const std::string& uri);
    static void onPadAdded(GstElement* element, GstPad* pad, gpointer data);
};

#endif // INPUT_MANAGER_H