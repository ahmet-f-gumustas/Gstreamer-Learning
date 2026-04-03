#!/bin/bash

# GStreamer Video Analytics Pipeline Build Script

# Define colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Project root directory
PROJECT_ROOT=$(dirname $(dirname $(readlink -f $0)))
BUILD_DIR="$PROJECT_ROOT/build"

echo -e "${GREEN}=== GStreamer Video Analytics Pipeline Build Script ===${NC}"
echo "Project directory: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"
echo ""

# Get build type (default: Release)
BUILD_TYPE=${1:-Release}
echo -e "${YELLOW}Build type: $BUILD_TYPE${NC}"

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Create required directories
mkdir -p "$PROJECT_ROOT/logs"
mkdir -p "$PROJECT_ROOT/recordings"
mkdir -p "$PROJECT_ROOT/assets"

cd "$BUILD_DIR"

# CMake configuration
echo -e "\n${YELLOW}Starting CMake configuration...${NC}"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

# Get CPU core count
CORES=$(nproc)
echo -e "\n${YELLOW}Starting compilation ($CORES cores will be used)...${NC}"

# Compile
make -j$CORES

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi

echo -e "\n${GREEN}Compilation completed successfully!${NC}"

# Create test video file (if not exists)
if [ ! -f "$PROJECT_ROOT/assets/test_video.mp4" ]; then
    echo -e "\n${YELLOW}Creating test video file...${NC}"
    gst-launch-1.0 videotestsrc num-buffers=300 ! \
        video/x-raw,width=1280,height=720,framerate=30/1 ! \
        x264enc ! mp4mux ! \
        filesink location="$PROJECT_ROOT/assets/test_video.mp4"
fi

# Run information
echo -e "\n${GREEN}=== Build Complete ===${NC}"
echo "Executable: $BUILD_DIR/gstreamer_video_analytics"
echo ""
echo "Example usage:"
echo "  # Run with test video"
echo "  $BUILD_DIR/gstreamer_video_analytics -i assets/test_video.mp4"
echo ""
echo "  # Webcam with motion detection"
echo "  $BUILD_DIR/gstreamer_video_analytics -i webcam --motion-detect"
echo ""
echo "  # Start RTSP stream"
echo "  $BUILD_DIR/gstreamer_video_analytics -i webcam -o rtsp://0.0.0.0:8554/live"
echo ""
