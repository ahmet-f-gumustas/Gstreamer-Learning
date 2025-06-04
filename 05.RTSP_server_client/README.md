# RTSP Low-Latency Camera Streaming Project

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-14-blue.svg" alt="C++ Version">
  <img src="https://img.shields.io/badge/GStreamer-1.16%2B-green.svg" alt="GStreamer Version">
  <img src="https://img.shields.io/badge/Target%20Latency-%3C250ms-orange.svg" alt="Target Latency">
</p>

## 📝 About The Project

This project is a low-latency RTSP video streaming server and client implementation developed using the GStreamer library. The goal is to achieve real-time video transmission with an end-to-end latency of less than 250ms.

### 🎯 Key Features

- ✅ **Ultra-low latency** (< 250ms)
- ✅ **H.264 video codec** support
- ✅ **UDP/RTSP protocol** usage
- ✅ **Automatic latency measurement** and reporting
- ✅ **Optimized buffer management**
- ✅ **Real-time FPS display**

## 🚀 Quick Start

### System Requirements

- **Operating System**: Linux (Ubuntu 20.04+ recommended)
- **Compiler**: GCC 7+ or Clang 6+
- **CMake**: 3.10 or higher
- **GStreamer**: 1.16 or higher
- **Hardware**: V4L2 compatible USB/built-in camera

### 📦 Installing Dependencies

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-rtsp-server-1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    gstreamer1.0-x \
    v4l-utils
```

#### Fedora/RHEL:
```bash
sudo dnf install -y \
    cmake \
    gcc-c++ \
    gstreamer1-devel \
    gstreamer1-plugins-base-devel \
    gstreamer1-rtsp-server-devel \
    gstreamer1-plugins-good \
    gstreamer1-plugins-bad-free \
    gstreamer1-plugins-ugly \
    v4l-utils
```

### 🔨 Building and Installation

```bash
# Clone the project
git clone <project-url>
cd rtsp-camera-streaming

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# (Optional) Install to system
sudo make install
```

## 📖 Usage Guide

### 1. Starting the RTSP Server

```bash
# Start with default settings
./rtsp_cam_server

# Output:
# === Low-Latency Configuration ===
# Encoder: x264enc with tune=zerolatency
# Speed Preset: ultrafast
# Profile: baseline (lowest complexity)
# Queue: max-size-buffers=1 (minimal buffering)
# Transport: UDP (lower latency than TCP)
# Target Latency: < 250ms
# ================================
# 
# RTSP server started at rtsp://127.0.0.1:8554/camera
# Low-latency RTSP camera server is running...
# Stream URL: rtsp://localhost:8554/camera
# Press Ctrl+C to stop
```

### 2. Running the RTSP Client

```bash
# Connect to default server
./rtsp_cam_client

# Connect with custom URL
./rtsp_cam_client rtsp://192.168.1.100:8554/camera

# Output:
# Starting RTSP client...
# Connecting to: rtsp://localhost:8554/camera
# RTSP client is running...
# Press Ctrl+C to stop
# Current latency: 185.43 ms ✓ (Target achieved!)
# Current latency: 192.21 ms ✓ (Target achieved!)
# ...
```

The client automatically runs for 30 seconds and generates the `latency_plot.md` report.

## 🔧 Configuration and Optimization

### Server-Side Optimizations

| Parameter | Value | Description |
|-----------|-------|-------------|
| `tune` | zerolatency | Optimizes x264enc for minimum latency |
| `speed-preset` | ultrafast | Fastest encoding preset |
| `key-int-max` | 15 | Maximum GOP size (lower = lower latency) |
| `intra-refresh` | true | Progressive intra refresh enabled |
| `profile` | baseline | Lowest complexity H.264 profile |
| `max-size-buffers` | 1 | Maximum 1 frame in queue |
| `sync` | false | Pipeline synchronization disabled |

### Client-Side Optimizations

| Parameter | Value | Description |
|-----------|-------|-------------|
| `latency` | 0 | Minimum latency setting |
| `buffer-mode` | 0 | No jitterbuffer usage |
| `protocols` | UDP | Low-latency transport |
| `do-rtcp` | TRUE | RTCP feedback enabled |
| `leaky` | downstream | Drop old frames |

## 🛠️ Advanced Features

### Custom Pipeline Definition

```cpp
// Using custom pipeline in server code
server.setCustomPipeline(
    "( v4l2src device=/dev/video1 ! "
    "video/x-raw,width=1920,height=1080,framerate=60/1 ! "
    "videoconvert ! "
    "x264enc tune=zerolatency speed-preset=superfast ! "
    "rtph264pay name=pay0 pt=96 )"
);
```

### Different Video Sources

```bash
# Using test pattern
"videotestsrc pattern=ball ! ..."

# Using IP camera
"rtspsrc location=rtsp://camera-ip/stream ! ..."

# Using video file
"filesrc location=video.mp4 ! decodebin ! ..."
```

## 🐛 Debugging

### 1. GStreamer Debug Logs

```bash
# General debug (Level 3)
GST_DEBUG=3 ./rtsp_cam_server

# Module-specific debug
GST_DEBUG=rtspsrc:5,rtph264pay:4 ./rtsp_cam_client

# Debug for all RTSP modules
GST_DEBUG=rtsp*:5 ./rtsp_cam_server

# Write debug log to file
GST_DEBUG=4 ./rtsp_cam_client 2> client_debug.log
```

### 2. Pipeline Visualization

```bash
# Create pipeline graph
GST_DEBUG_DUMP_DOT_DIR=. ./rtsp_cam_client

# Convert DOT file to PNG
dot -Tpng *.dot -o pipeline.png

# If multiple DOT files exist
for dot in *.dot; do
    dot -Tpng "$dot" -o "${dot%.dot}.png"
done
```

### 3. Performance Analysis

```bash
# CPU profiling
perf record -g ./rtsp_cam_server
perf report

# Latency analysis
GST_DEBUG="GST_TRACER:7" GST_TRACERS="latency" ./rtsp_cam_client

# Buffer usage tracking
GST_DEBUG="queue:5" ./rtsp_cam_server
```

### 4. Network Analysis

```bash
# Monitor RTSP traffic
sudo tcpdump -i any -n port 8554 -w rtsp_traffic.pcap

# Analyze with Wireshark
wireshark rtsp_traffic.pcap

# Bandwidth usage
iftop -i eth0 -f "port 8554"
```

## 📊 Performance Metrics

### Expected Results

| Metric | Target | Typical Result |
|--------|--------|----------------|
| End-to-end latency | < 250ms | 150-200ms |
| FPS | 30 | 29-30 |
| CPU usage (server) | < 30% | 15-25% |
| CPU usage (client) | < 20% | 10-15% |
| Packet loss | < 0.1% | 0.01-0.05% |

### Sample Report Output

```markdown
# RTSP Low-Latency Streaming Report

## Configuration
- **Stream URL**: rtsp://localhost:8554/camera
- **Target Latency**: < 250ms
- **Transport Protocol**: UDP
- **Encoder Settings**: x264enc tune=zerolatency
- **Buffer Configuration**: max-size-buffers=1

## Latency Measurements

| Time (s) | Latency (ms) | Status |
|----------|--------------|--------|
| 0 | 185.43 | ✓ Pass |
| 1 | 192.21 | ✓ Pass |
| 2 | 178.95 | ✓ Pass |
...

## Summary Statistics
- **Average Latency**: 186.73 ms
- **Minimum Latency**: 165.32 ms  
- **Maximum Latency**: 215.87 ms
- **Target Achievement**: ✓ **SUCCESS** - Average latency below 250ms target

## Latency Graph (ASCII)
```
   250 |                                          
   225 |     █                                    
   200 |   █ █ █   █                             
   175 | █ █ █ █ █ █ █ █                         
   150 |                                          
       +------------------------------------------
        Time (seconds)
```
```

## 🎬 Creating Demo Video

### Simple Recording
```bash
# 30-second video recording
timeout 30 gst-launch-1.0 -e \
    rtspsrc location=rtsp://localhost:8554/camera latency=0 ! \
    rtph264depay ! h264parse ! mp4mux ! \
    filesink location=demo.mp4
```

### Advanced Recording (with Overlay)
```bash
# Recording with timestamp and FPS overlay
gst-launch-1.0 -e \
    rtspsrc location=rtsp://localhost:8554/camera latency=0 ! \
    rtph264depay ! h264parse ! avdec_h264 ! \
    clockoverlay time-format="%D %H:%M:%S" ! \
    fpsdisplaysink video-sink="x264enc ! mp4mux ! filesink location=demo_overlay.mp4"
```

## ❓ Common Issues

### 1. Camera Not Found
```bash
# List available cameras
ls -la /dev/video*
v4l2-ctl --list-devices

# Check camera capabilities
v4l2-ctl -d /dev/video0 --list-formats-ext
```

### 2. High Latency (> 250ms)
- Check CPU frequency management: `cpupower frequency-info`
- Verify network QoS settings
- Reduce video resolution (640x480)
- Lower framerate (15-20 FPS)

### 3. Connection Error
```bash
# Firewall check
sudo ufw status
sudo iptables -L

# Open port (UFW)
sudo ufw allow 8554/tcp
sudo ufw allow 8554/udp

# SELinux check (RHEL/Fedora)
sudo setenforce 0  # Temporarily disable
```

### 4. Video Quality Issues
```bash
# Increase bitrate
"x264enc tune=zerolatency bitrate=2000 ! ..."

# Change profile
"x264enc tune=zerolatency profile=main ! ..."
```

## 📚 Resources and Documentation

- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [GStreamer RTSP Server](https://gstreamer.freedesktop.org/documentation/gst-rtsp-server/)
- [x264 Encoding Guide](https://trac.ffmpeg.org/wiki/Encode/H.264)
- [V4L2 API Documentation](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2.html)

## 📄 License

This project is licensed under the [MIT License](LICENSE).

## 🤝 Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

---

<p align="center">
  <i>Low-latency video streaming with GStreamer and modern C++</i>
</p>