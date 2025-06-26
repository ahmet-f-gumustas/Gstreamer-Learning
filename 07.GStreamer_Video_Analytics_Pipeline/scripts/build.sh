#!/bin/bash

# GStreamer Video Analytics Pipeline Build Script

# Renkleri tanımla
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Proje kök dizini
PROJECT_ROOT=$(dirname $(dirname $(readlink -f $0)))
BUILD_DIR="$PROJECT_ROOT/build"

echo -e "${GREEN}=== GStreamer Video Analytics Pipeline Build Script ===${NC}"
echo "Proje dizini: $PROJECT_ROOT"
echo "Build dizini: $BUILD_DIR"
echo ""

# Build tipini al (varsayılan: Release)
BUILD_TYPE=${1:-Release}
echo -e "${YELLOW}Build tipi: $BUILD_TYPE${NC}"

# Build dizinini oluştur
if [ ! -d "$BUILD_DIR" ]; then
    echo "Build dizini oluşturuluyor..."
    mkdir -p "$BUILD_DIR"
fi

# Gerekli dizinleri oluştur
mkdir -p "$PROJECT_ROOT/logs"
mkdir -p "$PROJECT_ROOT/recordings"
mkdir -p "$PROJECT_ROOT/assets"

cd "$BUILD_DIR"

# CMake yapılandırması
echo -e "\n${YELLOW}CMake yapılandırması başlatılıyor...${NC}"
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake yapılandırması başarısız!${NC}"
    exit 1
fi

# CPU çekirdek sayısını al
CORES=$(nproc)
echo -e "\n${YELLOW}Derleme başlatılıyor ($CORES çekirdek kullanılacak)...${NC}"

# Derleme
make -j$CORES

if [ $? -ne 0 ]; then
    echo -e "${RED}Derleme başarısız!${NC}"
    exit 1
fi

echo -e "\n${GREEN}Derleme başarıyla tamamlandı!${NC}"

# Test video dosyası oluştur (yoksa)
if [ ! -f "$PROJECT_ROOT/assets/test_video.mp4" ]; then
    echo -e "\n${YELLOW}Test video dosyası oluşturuluyor...${NC}"
    gst-launch-1.0 videotestsrc num-buffers=300 ! \
        video/x-raw,width=1280,height=720,framerate=30/1 ! \
        x264enc ! mp4mux ! \
        filesink location="$PROJECT_ROOT/assets/test_video.mp4"
fi

# Çalıştırma bilgisi
echo -e "\n${GREEN}=== Derleme Tamamlandı ===${NC}"
echo "Çalıştırılabilir dosya: $BUILD_DIR/gstreamer_video_analytics"
echo ""
echo "Örnek kullanımlar:"
echo "  # Test videosu ile çalıştır"
echo "  $BUILD_DIR/gstreamer_video_analytics -i assets/test_video.mp4"
echo ""
echo "  # Web kamerası ile hareket algılama"
echo "  $BUILD_DIR/gstreamer_video_analytics -i webcam --motion-detect"
echo ""
echo "  # RTSP yayını başlat"
echo "  $BUILD_DIR/gstreamer_video_analytics -i webcam -o rtsp://0.0.0.0:8554/live"
echo ""