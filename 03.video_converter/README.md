# GStreamer Video Converter

Simple video converter with RTX 4070 and CUDA 12.4 support.

## Quick Start

```bash
# 1. Make the build script executable
chmod +x build.sh

# 2. Install dependencies
./build.sh deps

# 3. Build the project
./build.sh

# 4. Test
./build.sh test
```

## Usage

```bash
# Convert to MP4 (GPU accelerated)
./build/bin/video_converter input.mov output.mp4

# Convert to WebM
./build/bin/video_converter input.mp4 output.webm webm

# Convert to AVI
./build/bin/video_converter input.mkv output.avi avi
```

## Build Commands

```bash
./build.sh          # Normal build
./build.sh deps     # Install dependencies
./build.sh check    # System check
./build.sh test     # Build + test
./build.sh clean    # Clean
```

## Features

- **CUDA Support**: GPU acceleration with RTX 4070
- **Multiple Formats**: MP4, WebM, AVI
- **Simple Usage**: Convert with a single command
- **Modern C++17**: CMake build system

## System Requirements

- Ubuntu 22.04
- CMake 3.16+
- GCC 11.4+
- GStreamer 1.0+
- CUDA 12.4 (optional)

---

**Note**: If CUDA is not found, CPU encoder is used automatically.
