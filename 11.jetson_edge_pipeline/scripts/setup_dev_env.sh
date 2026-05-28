#!/usr/bin/env bash
# Geliştirme ortamı kurucu (Ubuntu 22.04 + x86_64).
# Jetson host'ta ayrı bir script (deploy_to_jetson.sh) kullanılır.
set -euo pipefail

echo "=== Jetson Edge Pipeline — Dev Setup ==="

if [[ "$(lsb_release -is 2>/dev/null)" != "Ubuntu" ]]; then
    echo "Bu script Ubuntu için tasarlandı. Diğer dağıtımlarda manuel kurulum gerekir."
fi

# 1) GStreamer + multimedia bağımlılıkları
sudo apt update
sudo apt install -y \
    build-essential cmake ninja-build git pkg-config \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad  gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav        gstreamer1.0-tools \
    libopencv-dev libyaml-cpp-dev \
    v4l-utils

# 2) GigE Vision için Aravis
sudo apt install -y libaravis-0.8-dev gstreamer1.0-aravis libaravis-0.8-bin || true

# 3) NVIDIA / CUDA kontrolü
if ! command -v nvidia-smi &>/dev/null; then
    echo "[!] nvidia-smi bulunamadı. CUDA driver kurulmalı."
    echo "    https://developer.nvidia.com/cuda-downloads adresinden indir."
fi

if ! ldconfig -p | grep -q libnvinfer.so; then
    echo "[!] TensorRT bulunamadı."
    echo "    https://developer.nvidia.com/tensorrt-download adresinden DEB indir."
fi

# 4) Python araçları (ONNX export + INT8 calibration için)
python3 -m pip install --user --upgrade \
    numpy onnx onnxruntime-gpu opencv-python ultralytics pycuda

echo
echo "=== Setup tamam ==="
echo "Build için:"
echo "  mkdir build && cd build && cmake .. && make -j\$(nproc)"
