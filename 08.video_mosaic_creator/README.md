# Video Mosaic Creator

A professional video processing application based on GStreamer that combines multiple video sources into a single mosaic display.

## Project Overview

Video Mosaic Creator is a powerful video compositing tool designed for security systems, live broadcasts, video conferencing applications, and multi-video monitoring scenarios. It provides high-performance video processing using GStreamer's compositor element.

## Features

### Core Features
- **Multiple Video Source Support**
  - Web cameras (V4L2)
  - RTSP streams
  - Video files (MP4, AVI, MKV, etc.)
  - HTTP/HTTPS video streams
  - Test patterns (for development)

- **Flexible Grid Layout System**
  - 2x2, 3x3, 4x4, and custom grid sizes
  - Dynamic cell sizing
  - Customizable padding and background

- **High Performance**
  - Hardware accelerated video decoding
  - Low-latency mode
  - CPU usage optimization

### Advanced Features
- YAML-based configuration system
- Runtime source addition/removal
- Automatic reconnection (for RTSP sources)
- Error handling and logging

## Technical Requirements

### System Requirements
- Ubuntu 20.04+ or similar Linux distribution
- GCC 9.0+ or Clang 10.0+
- CMake 3.16+
- At least 4GB RAM (8GB recommended)
- NVIDIA GPU (optional, for hardware acceleration)

### Dependencies
```bash
# GStreamer and development packages
sudo apt-get update
sudo apt-get install -y \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    gstreamer1.0-x \
    gstreamer1.0-alsa \
    gstreamer1.0-gl \
    gstreamer1.0-gtk3 \
    gstreamer1.0-qt5 \
    gstreamer1.0-pulseaudio

# OpenCV (optional, for advanced features)
sudo apt-get install -y libopencv-dev

# YAML configuration support
sudo apt-get install -y libyaml-cpp-dev

# Build tools
sudo apt-get install -y build-essential pkg-config
```

## Installation

### 1. Clone the Project
```bash
cd ~/git-projects/Gstreamer-Learning
mkdir 08.Video_Mosaic_Creator
cd 08.Video_Mosaic_Creator
# Copy project files here
```

### 2. Build
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 3. Debug Build (for Development)
```bash
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

## Usage

### Basic Usage
```bash
# Run with default configuration
./video-mosaic-creator

# Run with a custom configuration file
./video-mosaic-creator config/my_mosaic.yaml
```

### Example Scenarios

#### 1. Security Camera System (4 Cameras)
```yaml
# config/security_cameras.yaml
output:
  width: 1920
  height: 1080
  
layout:
  grid_cols: 2
  grid_rows: 2
  cell_padding: 2
  background_color: "#1a1a1a"
  
sources:
  - name: "FrontDoor"
    uri: "rtsp://192.168.1.100:554/stream1"
    position: [0, 0]
  
  - name: "Backyard"
    uri: "rtsp://192.168.1.101:554/stream1"
    position: [1, 0]
  
  - name: "Garage"
    uri: "rtsp://192.168.1.102:554/stream1"
    position: [0, 1]
  
  - name: "Driveway"
    uri: "rtsp://192.168.1.103:554/stream1"
    position: [1, 1]
```

#### 2. Video Conference (3x3 Grid)
```yaml
# config/video_conference.yaml
output:
  width: 1920
  height: 1080
  
layout:
  grid_cols: 3
  grid_rows: 3
  cell_padding: 5
  background_color: "#2d2d2d"
```

#### 3. Testing and Development
```bash
# Quick test with test patterns
./video-mosaic-creator

# Programmatic source addition examples are available in the code
```

### Command Line Parameters
```bash
./video-mosaic-creator [options]
  -c, --config <file>     Configuration file
  -v, --verbose          Detailed log output
  -h, --help             Help message
```

## Architecture

### Pipeline Structure
```
+-------------------------------------------------------------+
|                      Video Mosaic Pipeline                   |
+-------------------------------------------------------------+
|                                                              |
|  +----------+   +----------+   +-----------+                 |
|  |  Source  |--->| Decoder  |--->| Converter |---+            |
|  +----------+   +----------+   +-----------+   |            |
|                                                 v            |
|  +----------+   +----------+   +-----------+  +--------+    |
|  |  Source  |--->| Decoder  |--->| Converter |->|        |    |
|  +----------+   +----------+   +-----------+  | Compo- |    |
|                                                | sitor  |--->| Display
|  +----------+   +----------+   +-----------+  |        |    |
|  |  Source  |--->| Decoder  |--->| Converter |->|        |    |
|  +----------+   +----------+   +-----------+  +--------+    |
|                                                              |
+-------------------------------------------------------------+
```

### Class Diagram
```
VideoMosaic
    +-- InputManager
    |   +-- VideoInput[]
    |       +-- source
    |       +-- decoder
    |       +-- converter
    |       +-- scaler
    +-- MosaicLayout
        +-- LayoutConfig
```

## Development

### Code Structure
```
src/
+-- main.cpp              # Main program entry point
+-- video_mosaic.cpp      # Main pipeline management
+-- input_manager.cpp     # Video source management
+-- mosaic_layout.cpp     # Grid layout calculations

include/
+-- video_mosaic.h        # VideoMosaic class
+-- input_manager.h       # InputManager class
+-- mosaic_layout.h       # MosaicLayout class
```

### Adding New Features

#### 1. Adding a New Video Source Type
```cpp
// Add to the createSourceElement() function in input_manager.cpp
else if (uri.find("new_protocol://") == 0) {
    source = gst_element_factory_make("new_element", nullptr);
    // Configuration...
}
```

#### 2. Adding Overlay Text
```cpp
// Display name on each video
GstElement* textoverlay = gst_element_factory_make("textoverlay", nullptr);
g_object_set(textoverlay, 
    "text", input->name.c_str(),
    "valignment", 2,  // top
    "halignment", 0,  // left
    nullptr);
```

## Advanced Usage Examples

### RTSP Server Integration
```cpp
// Stream the mosaic output over RTSP
GstElement* rtsp_sink = gst_element_factory_make("rtspclientsink", nullptr);
g_object_set(rtsp_sink, "location", "rtsp://localhost:8554/mosaic", nullptr);
```

### Recording Feature
```cpp
// Save the mosaic output to a file
GstElement* filesink = gst_element_factory_make("filesink", nullptr);
g_object_set(filesink, "location", "output_mosaic.mp4", nullptr);
```

### Motion Detection
```cpp
// Add motion detection to each source
GstElement* motion = gst_element_factory_make("motioncells", nullptr);
g_object_set(motion, "sensitivity", 0.5, nullptr);
```

## Troubleshooting

### Common Errors and Solutions

1. **"Failed to create pipeline" error**
   ```bash
   # Check GStreamer installation
   gst-inspect-1.0 | grep compositor
   ```

2. **RTSP connection issues**
   ```bash
   # Test the RTSP source
   gst-launch-1.0 rtspsrc location=rtsp://... ! fakesink
   ```

3. **Performance issues**
   - Enable hardware decoding
   - Reduce grid size
   - Lower video resolution

### Debug Logs
```bash
# Detailed GStreamer logs
GST_DEBUG=3 ./video-mosaic-creator

# Compositor logs only
GST_DEBUG=compositor:5 ./video-mosaic-creator
```

## Performance Tips

1. **Hardware Acceleration**
   ```cpp
   // Use NVIDIA hardware decoder
   decoder = gst_element_factory_make("nvv4l2decoder", nullptr);
   ```

2. **Buffer Optimization**
   ```cpp
   g_object_set(source, "buffer-size", 2048000, nullptr);
   ```

3. **Thread Usage**
   ```cpp
   g_object_set(pipeline, "max-threads", 4, nullptr);
   ```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License.

## Useful Resources

- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [GStreamer Compositor Plugin](https://gstreamer.freedesktop.org/documentation/compositor/)
- [YAML-CPP Documentation](https://github.com/jbeder/yaml-cpp/wiki)

## Future Improvements

- [ ] Web interface for control
- [ ] Dynamic layout switching
- [ ] Audio mixing support
- [ ] AI-based scene detection
- [ ] Cloud streaming support
- [ ] Docker container support
- [ ] Prometheus metrics integration
