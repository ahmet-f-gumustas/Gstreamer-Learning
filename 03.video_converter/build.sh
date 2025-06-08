#!/bin/bash

# Simple build script for GStreamer Video Converter

echo "=== GStreamer Video Converter Build Script ==="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

# Check if dependencies are installed
check_deps() {
    echo -e "${YELLOW}Checking dependencies...${NC}"
    
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}CMake not found!${NC}"
        echo "Install with: sudo apt install cmake"
        exit 1
    fi
    
    if ! pkg-config --exists gstreamer-1.0; then
        echo -e "${RED}GStreamer not found!${NC}"
        echo "Install with: sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev"
        exit 1
    fi
    
    echo -e "${GREEN}Dependencies OK${NC}"
}

# Install dependencies
install_deps() {
    echo -e "${YELLOW}Installing dependencies...${NC}"
    sudo apt update
    sudo apt install -y cmake build-essential pkg-config
    sudo apt install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
    sudo apt install -y gstreamer1.0-plugins-base gstreamer1.0-plugins-good 
    sudo apt install -y gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly
    sudo apt install -y gstreamer1.0-libav gstreamer1.0-x264
    echo -e "${GREEN}Dependencies installed${NC}"
}

# Build project
build() {
    echo -e "${YELLOW}Building project...${NC}"
    
    # Create build directory
    mkdir -p build
    
    # Configure and build
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel $(nproc)
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Build successful!${NC}"
        echo "Executable: ./build/bin/video_converter"
        echo ""
        echo "Usage examples:"
        echo "  ./build/bin/video_converter input.mp4 output.mp4"
        echo "  ./build/bin/video_converter input.mov output.webm webm"
    else
        echo -e "${RED}Build failed!${NC}"
        exit 1
    fi
}

# Create test video
create_test() {
    if command -v ffmpeg &> /dev/null; then
        echo -e "${YELLOW}Creating test video...${NC}"
        mkdir -p test_videos
        ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 -c:v libx264 test_videos/test.mp4 -y
        echo -e "${GREEN}Test video created: test_videos/test.mp4${NC}"
    else
        echo -e "${YELLOW}ffmpeg not found, skipping test video creation${NC}"
    fi
}

# Test conversion
test_conversion() {
    if [ -f "test_videos/test.mp4" ] && [ -f "build/bin/video_converter" ]; then
        echo -e "${YELLOW}Testing conversion...${NC}"
        ./build/bin/video_converter test_videos/test.mp4 test_videos/output.mp4
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}Test conversion successful!${NC}"
        else
            echo -e "${RED}Test conversion failed!${NC}"
        fi
    else
        echo -e "${YELLOW}Skipping test (missing files)${NC}"
    fi
}

# Main script
case "$1" in
    "deps")
        install_deps
        ;;
    "check")
        check_deps
        ;;
    "test")
        check_deps
        build
        create_test
        test_conversion
        ;;
    "clean")
        echo -e "${YELLOW}Cleaning build files...${NC}"
        rm -rf build
        echo -e "${GREEN}Clean complete${NC}"
        ;;
    *)
        check_deps
        build
        ;;
esac