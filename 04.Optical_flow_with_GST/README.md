# GStreamer Optical Flow Detector

This project performs real-time optical flow detection using **GStreamer** and **OpenCV**. It detects and visualizes motion vectors in video streams using the Lucas-Kanade algorithm.

## Features

- **Multiple Video Sources**: Webcam, video file, or test pattern
- **Real-Time Processing**: Motion detection with low latency
- **Smart Point Selection**: Automatic corner detection with goodFeaturesToTrack
- **Visual Feedback**: Motion vectors and tracked points
- **Modular Design**: Easily extensible code structure

## Requirements

### System Requirements
- Ubuntu 22.04+ or similar Linux distribution
- CMake 3.16+
- GCC 11+ (C++17 support)

### Libraries
```bash
# GStreamer libraries
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt-get install libgstreamer-plugins-good1.0-dev libgstreamer-plugins-bad1.0-dev
sudo apt-get install gstreamer1.0-plugins-good gstreamer1.0-plugins-bad
sudo apt-get install gstreamer1.0-libav gstreamer1.0-tools

# OpenCV
sudo apt-get install libopencv-dev

# V4L2 (webcam support)
sudo apt-get install v4l-utils

# Build tools
sudo apt-get install cmake build-essential pkg-config
```

## Installation

1. **Create Project Directory**
```bash
cd ~/git-projects/Gstreamer-Learning
mkdir 04.optical_flow
cd 04.optical_flow
```

2. **Create Project Structure**
```bash
mkdir -p include src build
```

3. **Place Files**
- `CMakeLists.txt` -> root directory
- `OpticalFlowDetector.hpp`, `Utils.hpp` -> `include/` directory
- `OpticalFlowDetector.cpp`, `Utils.cpp`, `main.cpp` -> `src/` directory

4. **Build**
```bash
cd build
cmake ..
make -j$(nproc)
```

## Usage

### Basic Usage
```bash
# Run with webcam (default)
./optical-flow-detector

# Run with test pattern
./optical-flow-detector --test

# Run with video file
./optical-flow-detector --file ../data/test_video.mp4
```

### Command Line Options
```bash
./optical-flow-detector [options]

Options:
  -h, --help          Show this help message
  -f, --file <path>   Use a video file
  -w, --webcam        Use webcam (default)
  -t, --test          Use test pattern
```

### Controls
- **'q' key**: Quit the program
- **Ctrl+C**: Terminate safely

## Algorithm Details

### What is Optical Flow?
Optical Flow is a computer vision technique that computes object motion vectors between consecutive frames in a video sequence.

### Method Used: Lucas-Kanade
- **Sparse Optical Flow**: Computes motion only at selected points
- **Pyramid Implementation**: Fast computation for multiple resolutions
- **Feature Tracking**: Automatic point selection with Good Features to Track

### Processing Steps
1. **Frame Capture**: Video stream via GStreamer
2. **Grayscale Conversion**: RGB to Grayscale
3. **Corner Detection**: Harris corner detection
4. **Optical Flow**: Lucas-Kanade algorithm
5. **Visualization**: Motion vectors and points

## Project Structure

```
04.optical_flow/
├── CMakeLists.txt              # Build configuration
├── README.md                   # Project documentation
├── include/
│   ├── OpticalFlowDetector.hpp # Main class header
│   └── Utils.hpp               # Helper functions
├── src/
│   ├── main.cpp                # Main program
│   ├── OpticalFlowDetector.cpp # Optical flow implementation
│   └── Utils.cpp               # Utility implementation
├── build/                      # Build files
└── test_videos/               # Test video files (optional)
```

## Technical Details

### GStreamer Pipeline
```
Webcam Mode:
v4l2src -> videoconvert -> appsink

Test Mode:
videotestsrc -> videoconvert -> appsink

File Mode:
filesrc -> qtdemux -> avdec_h264 -> videoconvert -> appsink
```

### OpenCV Processing Chain
1. **goodFeaturesToTrack()**: Detect corner points
2. **calcOpticalFlowPyrLK()**: Lucas-Kanade optical flow
3. **arrowedLine()**: Draw motion vectors
4. **circle()**: Display tracked points

### Parameters
```cpp
maxCorners = 100;        // Maximum number of points to track
qualityLevel = 0.01;     // Corner quality threshold
minDistance = 10.0;      // Minimum distance between points
```

## Performance Tips

### CPU Optimization
- Reduce frame size (640x480 recommended)
- Decrease maxCorners value
- Use ROI (Region of Interest)

### Memory Optimization
- Limit AppSink buffer count
- Minimize Mat clone() operations
- Avoid unnecessary format conversions

## Troubleshooting

### Webcam Not Found
```bash
# Check available video devices
ls /dev/video*
v4l2-ctl --list-devices

# Webcam test
gst-launch-1.0 v4l2src ! videoconvert ! xvimagesink
```

### Build Errors
```bash
# Check GStreamer packages
pkg-config --cflags --libs gstreamer-1.0

# Check OpenCV installation
pkg-config --modversion opencv4
```

### Permission Denied
```bash
# Check video device permissions
ls -l /dev/video0
sudo usermod -a -G video $USER
# Logout/login required
```

## Advanced Features

### Customization Options
- Different optical flow algorithms (Farneback, etc.)
- ROI-based processing
- Multi-object tracking
- Motion analysis and statistics

### Extension Ideas
- Object detection integration
- Motion-based alarm system
- Video stabilization
- Background subtraction

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is distributed under the MIT license. See the `LICENSE` file for details.

## Contact

For questions, please use GitHub issues.

---

**Happy Coding!**
