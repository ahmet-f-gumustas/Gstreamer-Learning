# 10. Stereo Depth Pipeline

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg" alt="C++17"/>
  <img src="https://img.shields.io/badge/GStreamer-1.16%2B-green.svg" alt="GStreamer"/>
  <img src="https://img.shields.io/badge/OpenCV-4.x-red.svg" alt="OpenCV"/>
  <img src="https://img.shields.io/badge/Platform-Linux-lightgrey.svg" alt="Linux"/>
  <img src="https://img.shields.io/badge/Use%20Case-Robotics-orange.svg" alt="Robotics"/>
</p>

A real-time stereo vision system built on GStreamer and OpenCV. Captures synchronized frames from two camera sources, computes a metric depth map using stereo matching, and classifies obstacles by distance — suitable for robotic navigation and collision avoidance.

---

## Table of Contents

- [What It Does](#what-it-does)
- [Architecture](#architecture)
- [GStreamer Pipeline](#gstreamer-pipeline)
- [Features](#features)
- [Requirements](#requirements)
- [Build](#build)
- [Usage](#usage)
- [Stereo Vision Theory](#stereo-vision-theory)
- [Stereo Calibration](#stereo-calibration)
- [Display Layout](#display-layout)
- [Robotics Integration](#robotics-integration)
- [Project Structure](#project-structure)
- [GStreamer Concepts](#gstreamer-concepts)

---

## What It Does

Three sequential stages:

```
[1] GStreamer captures synchronized frames from two camera sources
        ↓
[2] OpenCV StereoBM / SGBM computes per-pixel disparity,
    then converts disparity → real-world depth (meters)
        ↓
[3] Depth map is divided into a grid of cells,
    each cell classified as SAFE / CAUTION / DANGER
```

The output is a 4-panel window: left camera, right camera, color-coded depth map, and obstacle overlay.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                          GStreamer Layer                              │
│                                                                       │
│   videotestsrc / v4l2src / filesrc                                    │
│           │                                                           │
│    [decode + scale + colorspace conversion]                           │
│           │                                                           │
│          tee ──────────────────────────────────────┐                 │
│           │                                        │                  │
│   queue → videoconvert → appsink(left)    queue → appsink(right)     │
└───────────┼────────────────────────────────────────┼─────────────────┘
            │  callback: onLeftSample                 │  callback: onRightSample
            │                                         │
            ▼                                         ▼
       cv::Mat (left)                           cv::Mat (right)
            │                                         │
            └──────────────────┬──────────────────────┘
                               │
                    ┌──────────▼──────────┐
                    │    DepthEstimator    │
                    │                     │
                    │  1. Grayscale conv   │
                    │  2. Rectification   │  ← if calibration loaded
                    │  3. StereoBM/SGBM   │
                    │  4. disp → meters   │
                    │  5. JET colorize    │
                    └──────────┬──────────┘
                               │
                  ┌────────────┴────────────┐
                  │                         │
       ┌──────────▼──────────┐   ┌──────────▼──────────┐
       │  depthMap (float)    │   │   ObstacleDetector   │
       │  (meters)            │   │                      │
       └──────────────────────┘   │  3×4 grid analysis   │
                  │               │  Blob detection       │
                  │               │  Danger level output  │
                  │               └──────────┬────────────┘
                  │                          │
                  └──────────────┬───────────┘
                                 │
                      ┌──────────▼──────────┐
                      │    4-Panel Window    │
                      │   Left  │  Right     │
                      │   ──────┼──────      │
                      │   Depth │  Obstacle  │
                      └─────────────────────┘
```

---

## GStreamer Pipeline

The core of the project lives in the pipeline strings inside `stereo_pipeline.cpp`.

### Simulation Mode

```
videotestsrc pattern=18 is-live=true
  ! video/x-raw,width=640,height=480,framerate=30/1
  ! tee name=t
    t. ! queue max-size-buffers=2 leaky=downstream
       ! videoconvert
       ! video/x-raw,format=BGR
       ! appsink name=left_sink  emit-signals=true sync=true max-buffers=1 drop=true
    t. ! queue max-size-buffers=2 leaky=downstream
       ! videoconvert
       ! video/x-raw,format=BGR
       ! appsink name=right_sink emit-signals=true sync=true max-buffers=1 drop=true
```

| Element | Role |
|---------|------|
| `videotestsrc pattern=18` | Generates a moving ball test pattern — no camera required |
| `tee` | Splits one stream into two branches (left and right) |
| `queue leaky=downstream` | Each branch runs on its own thread; drops old frames when full |
| `videoconvert` | Converts GStreamer internal format to `BGR` required by OpenCV |
| `appsink` | GStreamer → C++ bridge; fires `new-sample` signal for every frame |

### Dual Webcam Mode

```
v4l2src device=/dev/video0
  ! videoconvert ! videoscale
  ! video/x-raw,format=BGR,width=640,height=480
  ! appsink name=left_sink ...

v4l2src device=/dev/video2
  ! videoconvert ! videoscale
  ! video/x-raw,format=BGR,width=640,height=480
  ! appsink name=right_sink ...
```

Two independent `v4l2src` elements run in parallel inside the same pipeline. `sync=false` minimizes latency without waiting for the other source.

### Video File Mode

```
filesrc location="video.mp4"
  ! decodebin
  ! videoconvert ! videoscale
  ! video/x-raw,format=BGR,width=640,height=480
  ! tee name=t
    t. ! queue ! appsink name=left_sink  ...
    t. ! queue ! appsink name=right_sink ...
```

`decodebin` auto-selects the codec (H.264, H.265, VP9…). The `tee` distributes each decoded frame to both sinks.

### Frame Delivery — Callback Mechanism

```cpp
// GStreamer calls this from its own internal thread
GstFlowReturn StereoPipeline::onLeftSample(GstAppSink* sink, gpointer data)
{
    GstSample* s = gst_app_sink_pull_sample(sink);   // grab the frame

    // GstBuffer → cv::Mat (zero-copy wrap, then clone for ownership)
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);
    cv::Mat frame(h, w, CV_8UC3, map.data);
    cv::Mat result = frame.clone();
    gst_buffer_unmap(buffer, &map);

    // Push to thread-safe queue — main loop reads from here
    std::lock_guard<std::mutex> lk(self->leftMtx_);
    self->leftQ_.push({result, timestamp});
}
```

---

## Features

| Feature | Detail |
|---------|--------|
| Source modes | Simulation / Dual USB camera / Video file |
| Stereo algorithm | StereoBM (fast) or StereoSGBM (accurate, `--sgbm`) |
| Calibration | Optional YAML support — rectification + Q matrix |
| Depth range | 0.1 m – 10 m (configurable via `--base`, `--focal`) |
| Obstacle grid | 3 rows × 4 columns = 12 cells, each independently classified |
| Blob detection | Morphology + connectedComponents for bounding boxes |
| Danger levels | SAFE / CAUTION / DANGER (thresholds are configurable) |
| Output | Real-time display window + optional MJPEG video recording |
| Thread safety | Mutex-protected dual queue (GStreamer thread ↔ main thread) |

---

## Requirements

### System Packages

```bash
sudo apt update && sudo apt install -y \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-0 \
    libgstreamer-plugins-bad1.0-0 \
    gstreamer1.0-tools \
    libopencv-dev \
    cmake \
    build-essential
```

### Version Requirements

| Component | Minimum |
|-----------|---------|
| GStreamer | 1.16 |
| OpenCV | 4.x |
| CMake | 3.16 |
| GCC | 9+ (C++17) |

---

## Build

```bash
cd 10.stereo_depth_pipeline
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

On success, `build/stereo-depth-pipeline` is produced.

---

## Usage

### Quick Start — Simulation (no camera needed)

```bash
./stereo-depth-pipeline --sim
```

Displays a moving ball pattern. The right image is the left image shifted 30 pixels horizontally, creating artificial disparity that simulates a constant-depth plane.

### Dual USB Cameras

```bash
./stereo-depth-pipeline --webcam \
    --left  /dev/video0 \
    --right /dev/video2 \
    --focal 554 \
    --base  0.06
```

> List available cameras: `ls /dev/video*`

### From a Video File

```bash
./stereo-depth-pipeline --file sample.mp4 --sgbm
```

### With Calibration (most accurate)

```bash
./stereo-depth-pipeline --webcam \
    --calib stereo_calib.yaml \
    --danger 0.8 \
    --caution 2.5
```

### Save Output to Video

```bash
./stereo-depth-pipeline --sim --save output.avi
```

### All Parameters

```
Source:
  --sim               Simulation mode (default)
  --webcam            Dual USB camera mode
  --file   <path>     Video file mode

Camera:
  --left   <device>   Left camera  (default: /dev/video0)
  --right  <device>   Right camera (default: /dev/video2)
  --width  <px>       Frame width  (default: 640)
  --height <px>       Frame height (default: 480)

Depth:
  --sgbm              Use StereoSGBM (more accurate, slower)
  --focal  <px>       Focal length in pixels  (default: 554)
  --base   <m>        Baseline in meters       (default: 0.06)
  --calib  <yaml>     Stereo calibration file

Obstacle:
  --danger  <m>       Danger threshold  (default: 1.0 m)
  --caution <m>       Caution threshold (default: 3.0 m)

Other:
  --save   <path>     Record output as MJPEG video
  -h, --help          Show this message
```

---

## Stereo Vision Theory

### Core Principle

Two cameras observe the same scene from slightly different horizontal positions. The horizontal pixel offset of the same point between the left and right image is called **disparity**. A larger disparity means the object is closer.

```
    Left Camera          Right Camera
         |    baseline (B)    |
         |←──────────────────→|
          \                  /
           \                /
            \              /
             \            /
              [  Object  ]   ← depth Z
```

### Disparity → Depth Formula

```
        f × B
Z = ─────────────
      disparity
```

| Symbol | Description | Typical Value |
|--------|-------------|---------------|
| `Z` | Depth (meters) | computed |
| `f` | Focal length (pixels) | 554 px (640 px wide, ~60° FOV) |
| `B` | Baseline — distance between camera centers (meters) | 0.06 m |
| `disparity` | Pixel offset from stereo matching | StereoBM/SGBM output |

> **Note:** OpenCV's StereoBM and StereoSGBM return 16× fixed-point disparity values.
> Divide by 16.0 to get the real pixel disparity before applying the formula.

### Focal Length Estimation (without calibration)

```
           image_width / 2
f  ≈  ──────────────────────────
       tan(horizontal_FOV / 2)
```

Common values:

| Resolution | FOV | focal (px) |
|------------|-----|-----------|
| 640×480 | 60° | 554 |
| 640×480 | 90° | 320 |
| 1280×720 | 60° | 1109 |

### StereoBM vs StereoSGBM

| Property | StereoBM | StereoSGBM |
|----------|----------|------------|
| Speed | Very fast | Moderate |
| Accuracy | Lower | Higher |
| Texture requirement | High | Low |
| Best for | Real-time embedded | Accurate measurement |
| Flag | _(default)_ | `--sgbm` |

---

## Stereo Calibration

For accurate metric depth, camera calibration is essential. A YAML file produced by OpenCV's `stereoCalibrate` can be loaded directly.

### Calibration YAML Format

```yaml
M1: !!opencv-matrix      # Left camera intrinsic matrix (3×3)
  rows: 3
  cols: 3
  data: [ fx,  0, cx,
           0, fy, cy,
           0,  0,  1 ]

D1: !!opencv-matrix      # Left distortion coefficients (1×5)
  rows: 1
  cols: 5
  data: [ k1, k2, p1, p2, k3 ]

M2: !!opencv-matrix      # Right camera intrinsic matrix
D2: !!opencv-matrix      # Right distortion coefficients

R:  !!opencv-matrix      # Rotation matrix — right to left (3×3)
  rows: 3
  cols: 3
  data: [ ... ]

T:  !!opencv-matrix      # Translation vector (3×1, in meters)
  rows: 3
  cols: 1
  data: [ tx, ty, tz ]
```

When a calibration file is loaded:
- Rectification maps are computed via `stereoRectify`
- `focal` and `baseline` are **read automatically from the Q matrix** — no manual input needed
- Every frame is undistorted and rectified via `cv::remap` before stereo matching

---

## Display Layout

```
┌─────────────────────┬─────────────────────┐
│                     │                     │
│    Left Camera      │    Right Camera      │
│    (raw frame)      │    (raw frame)       │
│                     │                     │
├─────────────────────┼─────────────────────┤
│                     │  STATUS: SAFE        │
│    Depth            │  Closest: 2.45 m    │
│    Map              │                     │
│                     │  [  ][  ][  ][  ]   │
│  red   = near       │  [  ][  ][  ][  ]   │
│  blue  = far        │  [  ][  ][  ][  ]   │
│  black = invalid    │                     │
└─────────────────────┴─────────────────────┘
```

### Depth Map Color Scale

```
Red ──── Yellow ──── Green ──── Cyan ──── Blue
near                                      far
(0 m)                                  (10 m)
Black = invalid pixel (disparity = 0)
```

### Obstacle Grid Colors

```
┌────────┬────────┬────────┬────────┐
│  2.1 m │  4.5 m │  3.8 m │  5.2 m │   Green  = SAFE    (> 3.0 m)
│CAUTION │  SAFE  │  SAFE  │  SAFE  │   Orange = CAUTION (1.0 – 3.0 m)
├────────┼────────┼────────┼────────┤   Red    = DANGER  (< 1.0 m)
│  0.7 m │  1.2 m │  2.8 m │  4.1 m │
│ DANGER │CAUTION │CAUTION │  SAFE  │
├────────┼────────┼────────┼────────┤
│   ---  │   ---  │   ---  │   ---  │   --- = no valid depth
└────────┴────────┴────────┴────────┘
```

---

## Robotics Integration

The output of this project can be fed directly into downstream robotics systems.

### ROS2 Integration

```cpp
// Publish depth frame as a ROS2 sensor_msgs/Image topic
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>

auto msg = cv_bridge::CvImage(header, "bgr8", depth_r.colorDepth).toImageMsg();
depth_pub_->publish(*msg);
```

### Jetson (CSI Camera)

Replace `v4l2src` with `nvarguscamerasrc` for Jetson CSI cameras:

```bash
./stereo-depth-pipeline --webcam \
    --left  "nvarguscamerasrc sensor-id=0" \
    --right "nvarguscamerasrc sensor-id=1"
```

### Motor / Servo Control

```cpp
// Add to the main loop based on ObstacleResult
switch (obs.overallDanger) {
    case DangerLevel::DANGER:
        robot.setSpeed(0.0f);    // stop immediately
        robot.alarm(true);
        break;
    case DangerLevel::CAUTION:
        robot.setSpeed(0.3f);    // slow down
        break;
    case DangerLevel::SAFE:
        robot.setSpeed(1.0f);    // full speed
        break;
}
```

### Closest Obstacle Distance (serial output)

```cpp
// For Arduino / Raspberry Pi serial communication
std::string msg = "DIST:" + std::to_string(obs.closestM) + "\n";
serial.write(msg);
```

---

## Project Structure

```
10.stereo_depth_pipeline/
│
├── CMakeLists.txt                ← Build configuration
├── README.md
│
├── include/
│   ├── stereo_pipeline.h         ← GStreamer pipeline interface
│   ├── depth_estimator.h         ← Stereo matching + depth conversion
│   └── obstacle_detector.h       ← Grid analysis + obstacle classification
│
└── src/
    ├── main.cpp                  ← CLI args, main loop, window layout
    ├── stereo_pipeline.cpp       ← gst_parse_launch, appsink callbacks, queues
    ├── depth_estimator.cpp       ← StereoBM/SGBM, disparity→meters, JET color
    └── obstacle_detector.cpp     ← Grid cells, blob detection, overlay drawing
```

---

## GStreamer Concepts

Key GStreamer concepts used in this project and where to find them:

| Concept | Element / API | Location |
|---------|--------------|----------|
| Pipeline creation | `gst_parse_launch()` | `stereo_pipeline.cpp:64` |
| Stream splitting | `tee` | Simulation and file pipelines |
| Thread-safe buffer | `queue leaky=downstream` | Each tee branch |
| Format conversion | `videoconvert` | GStreamer → BGR for OpenCV |
| C++ bridge | `appsink` | Left and right frame reception |
| Signal system | `g_signal_connect` | Binding `new-sample` callback |
| Buffer memory access | `gst_buffer_map` | `GstBuffer → cv::Mat` conversion |
| Timestamp | `GST_BUFFER_PTS` | Frame synchronization |
| State management | `gst_element_set_state` | PLAYING / NULL transitions |
| Source selection | `v4l2src` / `videotestsrc` / `filesrc` | Chosen by mode flag |
