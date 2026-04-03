# GStreamer Video Analytics Pipeline

This project is a comprehensive C++ application that performs real-time video analysis using GStreamer. It includes features for video stream processing, motion detection, object tracking, and streaming via RTSP.

## Features

- **Multi-Source Support**: Supports file, webcam, RTSP stream, and HTTP stream sources
- **Motion Detection**: Real-time motion detection with OpenCV integration
- **GPU Acceleration**: NVENC/NVDEC codec support for NVIDIA GPUs
- **RTSP Server**: Stream processed video over RTSP
- **Dynamic Pipeline**: Modify pipeline elements at runtime
- **Performance Monitoring**: Monitor FPS, CPU, and GPU usage
- **Recording**: Save processed video in various formats

## Requirements

- Ubuntu 22.04 LTS
- GStreamer 1.20+ (including all plugins)
- CMake 3.22+
- GCC 11.4+
- OpenCV 4.x (optional, for motion detection)
- NVIDIA GPU and drivers (for GPU acceleration)

## Installation

### Installing Dependencies

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
    gstreamer1.0-rtsp

# OpenCV (optional)
sudo apt-get install -y libopencv-dev

# YAML config parser
sudo apt-get install -y libyaml-cpp-dev
```

### Building the Project

```bash
# Navigate to the project directory
cd GStreamerVideoAnalytics

# Create the build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)
```

## Usage

### Basic Usage

```bash
# Process a video file
./gstreamer_video_analytics -i /path/to/video.mp4

# Use a webcam
./gstreamer_video_analytics -i webcam

# Process an RTSP stream
./gstreamer_video_analytics -i rtsp://192.168.1.100:554/stream

# Start an RTSP server
./gstreamer_video_analytics -i webcam -o rtsp://0.0.0.0:8554/live
```

### Advanced Features

```bash
# Enable motion detection
./gstreamer_video_analytics -i webcam --motion-detect

# Use GPU acceleration
./gstreamer_video_analytics -i video.mp4 --use-gpu

# Record video
./gstreamer_video_analytics -i rtsp://camera.local --record output.mp4

# Use a custom configuration file
./gstreamer_video_analytics --config config/custom_pipeline.yaml
```

## Configuration

You can customize pipeline settings by editing the `config/pipeline_config.yaml` file:

```yaml
pipeline:
  input:
    type: "file"  # file, webcam, rtsp, http
    location: "assets/test_video.mp4"
  
  processing:
    motion_detection: true
    gpu_acceleration: true
    
  output:
    type: "display"  # display, file, rtsp
    location: "rtsp://0.0.0.0:8554/live"
```

## Architecture

The project has a modular architecture:

- **PipelineManager**: Manages the main GStreamer pipeline
- **VideoProcessor**: Video processing and filter applications
- **MotionDetector**: Motion detection algorithms
- **RTSPStreamer**: RTSP server management

## Performance

- 1080p@30fps video processing (CPU)
- 4K@60fps video processing (with GPU)
- Low latency RTSP streaming (<100ms)
- Multi-stream support (depending on system resources)

## Troubleshooting

### GStreamer elements not found
```bash
gst-inspect-1.0 | grep [element_name]
```

### GPU acceleration not working
- Check NVIDIA drivers: `nvidia-smi`
- Check GStreamer NVCODEC plugin: `gst-inspect-1.0 nvcodec`

### RTSP connection issues
- Check firewall settings
- Make sure port 8554 is open

## License

This project is licensed under the MIT License.
