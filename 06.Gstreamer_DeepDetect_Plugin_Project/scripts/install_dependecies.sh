#!/bin/bash
set -e

echo "Installing DeepDetect plugin dependencies..."

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
else
    echo "Cannot detect OS version"
    exit 1
fi

# Function to install on Ubuntu/Debian
install_ubuntu() {
    echo "Installing on Ubuntu $VER"
    
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        ninja-build \
        pkg-config \
        git \
        wget \
        curl \
        vim \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        libgstreamer-plugins-bad1.0-dev \
        libgstreamer-plugins-good1.0-dev \
        gstreamer1.0-plugins-ugly \
        gstreamer1.0-libav \
        gstreamer1.0-tools \
        cpputest \
        clang-format \
        cppcheck

    # Install nlohmann/json
    if [ ! -f /usr/include/nlohmann/json.hpp ]; then
        echo "Installing nlohmann/json..."
        sudo mkdir -p /usr/include/nlohmann
        sudo wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp \
            -O /usr/include/nlohmann/json.hpp
    fi

    # Install CUDA if not present
    if ! command -v nvcc &> /dev/null; then
        echo "CUDA not found. Installing CUDA toolkit..."
        sudo apt-get install -y nvidia-cuda-toolkit
    else
        echo "CUDA already installed: $(nvcc --version | grep release)"
    fi

    # Check TensorRT
    if ! pkg-config --exists tensorrt; then
        echo "WARNING: TensorRT not found."
        echo "Please install TensorRT 10.0+ manually from:"
        echo "https://developer.nvidia.com/tensorrt"
        echo ""
        echo "For Ubuntu, you can also try:"
        echo "sudo apt-get install tensorrt-dev"
    else
        echo "TensorRT found: $(pkg-config --modversion tensorrt)"
    fi
}

# Function to install on CentOS/RHEL/Fedora
install_redhat() {
    echo "Installing on $OS $VER"
    
    if command -v dnf &> /dev/null; then
        PKG_MGR="dnf"
    else
        PKG_MGR="yum"
    fi
    
    sudo $PKG_MGR update -y
    sudo $PKG_MGR install -y \
        gcc gcc-c++ \
        cmake \
        ninja-build \
        pkg-config \
        git \
        wget \
        curl \
        vim \
        gstreamer1-devel \
        gstreamer1-plugins-base-devel \
        gstreamer1-plugins-bad-free-devel \
        gstreamer1-plugins-good \
        gstreamer1-plugins-ugly-free

    # Install nlohmann/json
    if [ ! -f /usr/include/nlohmann/json.hpp ]; then
        echo "Installing nlohmann/json..."
        sudo mkdir -p /usr/include/nlohmann
        sudo wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp \
            -O /usr/include/nlohmann/json.hpp
    fi
    
    echo "Please install CUDA and TensorRT manually for Red Hat based systems"
}

# Function to install on Jetson (Ubuntu-based)
install_jetson() {
    echo "Installing on NVIDIA Jetson"
    
    # Jetson specific packages
    sudo apt-get update
    sudo apt-get install -y \
        nvidia-jetpack \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        libgstreamer-plugins-bad1.0-dev \
        gstreamer1.0-plugins-good \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-libav \
        build-essential \
        cmake \
        pkg-config
    
    # Install nlohmann/json
    if [ ! -f /usr/include/nlohmann/json.hpp ]; then
        echo "Installing nlohmann/json..."
        sudo mkdir -p /usr/include/nlohmann
        sudo wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp \
            -O /usr/include/nlohmann/json.hpp
    fi
    
    echo "CUDA and TensorRT should be pre-installed on Jetson"
}

# Detect architecture
ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ] && [ -f /etc/nv_tegra_release ]; then
    install_jetson
elif [[ "$OS" == *"Ubuntu"* ]] || [[ "$OS" == *"Debian"* ]]; then
    install_ubuntu
elif [[ "$OS" == *"CentOS"* ]] || [[ "$OS" == *"Red Hat"* ]] || [[ "$OS" == *"Fedora"* ]]; then
    install_redhat
else
    echo "Unsupported OS: $OS"
    echo "Please install dependencies manually:"
    echo "- GStreamer 1.26+ development packages"
    echo "- CUDA 12.4+ toolkit"
    echo "- TensorRT 10.0+ development packages"
    echo "- CMake 3.20+"
    echo "- nlohmann/json library"
    exit 1
fi

echo ""
echo "Dependencies installation completed!"
echo ""
echo "Next steps:"
echo "1. Verify CUDA installation: nvcc --version"
echo "2. Verify TensorRT installation: pkg-config --modversion tensorrt"
echo "3. Build the plugin: mkdir build && cd build && cmake .. && make"
echo "4. Install the plugin: sudo make install"
echo ""

# Verify installations
echo "Verification:"
echo "============="

# Check GStreamer
if pkg-config --exists gstreamer-1.0; then
    echo "✓ GStreamer: $(pkg-config --modversion gstreamer-1.0)"
else
    echo "✗ GStreamer not found"
fi

# Check CUDA
if command -v nvcc &> /dev/null; then
    echo "✓ CUDA: $(nvcc --version | grep release | awk '{print $6}' | cut -c2-)"
else
    echo "✗ CUDA not found"
fi

# Check TensorRT
if pkg-config --exists tensorrt; then
    echo "✓ TensorRT: $(pkg-config --modversion tensorrt)"
else
    echo "⚠ TensorRT not found via pkg-config"
    if [ -f /usr/include/NvInfer.h ]; then
        echo "  (but headers found in /usr/include/)"
    fi
fi

# Check nlohmann/json
if [ -f /usr/include/nlohmann/json.hpp ]; then
    echo "✓ nlohmann/json: installed"
else
    echo "✗ nlohmann/json not found"
fi

echo ""
echo "Installation script completed!"