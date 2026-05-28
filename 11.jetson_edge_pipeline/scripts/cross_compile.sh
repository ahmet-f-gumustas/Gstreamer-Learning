#!/usr/bin/env bash
# x86 host'tan aarch64 Jetson hedefine cross-compile.
# QEMU + Docker buildx kullanır — Jetson hardware gerekmez.
#
# Sonuç: build_aarch64/jetson_edge  →  scp ile Jetson'a kopyala.
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build_aarch64"

# 1) qemu-user-static yüklü mü?
if ! docker buildx inspect jetson-builder &>/dev/null; then
    echo "[setup] qemu-user-static + buildx kuruluyor…"
    sudo apt install -y qemu-user-static binfmt-support
    docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
    docker buildx create --name jetson-builder --platform linux/arm64 --use
fi

# 2) aarch64 L4T container'ı içinde build et
docker run --rm --platform linux/arm64 \
    -v "$PROJECT_DIR":/work \
    -w /work \
    nvcr.io/nvidia/l4t-jetpack:r36.3.0 \
    bash -c "
        apt update && apt install -y \
            build-essential cmake ninja-build pkg-config \
            libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
            libgstreamer-plugins-bad1.0-dev libopencv-dev libyaml-cpp-dev \
            libaravis-0.8-dev && \
        rm -rf build_aarch64 && \
        cmake -B build_aarch64 -G Ninja \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX=/opt/jetson_edge && \
        cmake --build build_aarch64 -j
    "

echo
echo "=== Cross-compile tamam ==="
echo "Binary: $BUILD_DIR/jetson_edge"
echo
echo "Jetson'a deploy etmek için:"
echo "  scripts/deploy_to_jetson.sh <jetson-ip> <user>"
