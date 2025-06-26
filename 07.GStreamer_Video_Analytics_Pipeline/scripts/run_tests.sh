#!/bin/bash

# GStreamer Video Analytics Pipeline Test Script

# Renkleri tanımla
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Proje dizinleri
PROJECT_ROOT=$(dirname $(dirname $(readlink -f $0)))
BUILD_DIR="$PROJECT_ROOT/build"
EXECUTABLE="$BUILD_DIR/gstreamer_video_analytics"
TEST_VIDEO="$PROJECT_ROOT/assets/test_video.mp4"
OUTPUT_DIR="$PROJECT_ROOT/test_outputs"

# Test sonuçlarını sakla
PASSED_TESTS=0
FAILED_TESTS=0

echo -e "${BLUE}=== GStreamer Video Analytics Pipeline Test Suite ===${NC}"
echo "Proje dizini: $PROJECT_ROOT"
echo "Test çıktıları: $OUTPUT_DIR"
echo ""

# Çıktı dizinini oluştur
mkdir -p "$OUTPUT_DIR"

# Çalıştırılabilir dosyayı kontrol et
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}HATA: Çalıştırılabilir dosya bulunamadı!${NC}"
    echo "Önce './scripts/build.sh' ile projeyi derleyin."
    exit 1
fi

# Test video dosyasını kontrol et
if [ ! -f "$TEST_VIDEO" ]; then
    echo -e "${YELLOW}Test videosu bulunamadı, oluşturuluyor...${NC}"
    gst-launch-1.0 videotestsrc num-buffers=300 pattern=ball ! \
        video/x-raw,width=1280,height=720,framerate=30/1 ! \
        x264enc ! mp4mux ! \
        filesink location="$TEST_VIDEO" 2>/dev/null
fi

# Test fonksiyonu
run_test() {
    local test_name=$1
    local test_cmd=$2
    local expected_result=${3:-0}
    
    echo -ne "Test: $test_name ... "
    
    # Test logunu kaydet
    local log_file="$OUTPUT_DIR/${test_name// /_}.log"
    
    # Testi çalıştır (timeout ile)
    timeout 10s $test_cmd > "$log_file" 2>&1
    local result=$?
    
    # Timeout durumunu kontrol et
    if [ $result -eq 124 ]; then
        # Timeout oldu, bu bazı testler için normal
        if [ $expected_result -eq 124 ]; then
            echo -e "${GREEN}BAŞARILI${NC} (Timeout bekleniyor)"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}BAŞARISIZ${NC} (Beklenmeyen timeout)"
            ((FAILED_TESTS++))
        fi
    elif [ $result -eq $expected_result ]; then
        echo -e "${GREEN}BAŞARILI${NC}"
        ((PASSED_TESTS++))
    else
        echo -e "${RED}BAŞARISIZ${NC} (Kod: $result, Beklenen: $expected_result)"
        ((FAILED_TESTS++))
    fi
}

echo -e "${YELLOW}=== Temel İşlevsellik Testleri ===${NC}"

# Test 1: Yardım mesajı
run_test "Yardım Mesajı" "$EXECUTABLE --help" 1

# Test 2: Video dosyası oynatma
run_test "Video Dosyası Oynatma" "$EXECUTABLE -i $TEST_VIDEO" 124

# Test 3: Geçersiz dosya
run_test "Geçersiz Dosya" "$EXECUTABLE -i /tmp/nonexistent_video.mp4" 1

# Test 4: Video dosyasını kaydetme
run_test "Video Kayıt" "$EXECUTABLE -i $TEST_VIDEO -o $OUTPUT_DIR/test_output.mp4" 124

echo -e "\n${YELLOW}=== Hareket Algılama Testleri ===${NC}"

# Test 5: Hareket algılama
run_test "Hareket Algılama" "$EXECUTABLE -i $TEST_VIDEO --motion-detect" 124

# Test 6: Hareket algılama + kayıt
run_test "Hareket Algılama + Kayıt" "$EXECUTABLE -i $TEST_VIDEO --motion-detect --record $OUTPUT_DIR/motion_test.mp4" 124

echo -e "\n${YELLOW}=== Video İşleme Testleri ===${NC}"

# Test 7: Çözünürlük değiştirme
run_test "Çözünürlük Değiştirme" "$EXECUTABLE -i $TEST_VIDEO --width 640 --height 480" 124

# Test 8: FPS değiştirme
run_test "FPS Değiştirme" "$EXECUTABLE -i $TEST_VIDEO --fps 15" 124

# Test 9: Bitrate ayarı
run_test "Bitrate Ayarı" "$EXECUTABLE -i $TEST_VIDEO --bitrate 2000000 -o $OUTPUT_DIR/low_bitrate.mp4" 124

echo -e "\n${YELLOW}=== GPU Hızlandırma Testleri ===${NC}"

# GPU varlığını kontrol et
if nvidia-smi &>/dev/null; then
    # Test 10: GPU hızlandırma
    run_test "GPU Hızlandırma" "$EXECUTABLE -i $TEST_VIDEO --use-gpu" 124
    
    # Test 11: GPU + hareket algılama
    run_test "GPU + Hareket Algılama" "$EXECUTABLE -i $TEST_VIDEO --use-gpu --motion-detect" 124
else
    echo -e "${YELLOW}GPU bulunamadı, GPU testleri atlanıyor${NC}"
fi

echo -e "\n${YELLOW}=== RTSP Streaming Testleri ===${NC}"

# Test 12: RTSP server başlatma
run_test "RTSP Server" "$EXECUTABLE -i $TEST_VIDEO -o rtsp://0.0.0.0:8554/test" 124

# Test 13: Özel port ile RTSP
run_test "RTSP Özel Port" "$EXECUTABLE -i $TEST_VIDEO -o rtsp://0.0.0.0:8555/test --rtsp-port 8555" 124

echo -e "\n${YELLOW}=== Konfigürasyon Dosyası Testleri ===${NC}"

# Test config dosyası oluştur
cat > "$OUTPUT_DIR/test_config.yaml" << EOF
pipeline:
  input:
    type: "file"
    location: "$TEST_VIDEO"
  processing:
    motion_detection: true
    gpu_acceleration: false
  output:
    type: "display"
EOF

# Test 14: Config dosyası kullanımı
run_test "Config Dosyası" "$EXECUTABLE -c $OUTPUT_DIR/test_config.yaml" 124

echo -e "\n${YELLOW}=== Performans Testleri ===${NC}"

# Test 15: Büyük video işleme (varsa)
if [ -f "$PROJECT_ROOT/assets/large_video.mp4" ]; then
    run_test "Büyük Video İşleme" "$EXECUTABLE -i $PROJECT_ROOT/assets/large_video.mp4 --use-gpu" 124
else
    echo -e "${YELLOW}Büyük video dosyası bulunamadı, performans testi atlanıyor${NC}"
fi

echo -e "\n${YELLOW}=== Pipeline Kararlılık Testleri ===${NC}"

# Test 16: Çoklu başlatma/durdurma
for i in {1..3}; do
    run_test "Başlat/Durdur Test $i" "timeout 2s $EXECUTABLE -i $TEST_VIDEO" 124
done

echo -e "\n${YELLOW}=== GStreamer Element Testleri ===${NC}"

# GStreamer elementlerini kontrol et
check_gst_element() {
    local element=$1
    if gst-inspect-1.0 $element &>/dev/null; then
        echo -e "  $element: ${GREEN}✓${NC}"
    else
        echo -e "  $element: ${RED}✗${NC}"
    fi
}

echo "Gerekli GStreamer elementleri:"
check_gst_element "filesrc"
check_gst_element "decodebin"
check_gst_element "videoconvert"
check_gst_element "videoscale"
check_gst_element "x264enc"
check_gst_element "mp4mux"
check_gst_element "autovideosink"
check_gst_element "rtspserver"

# NVCODEC elementlerini kontrol et (opsiyonel)
echo -e "\nOpsiyonel GPU elementleri:"
check_gst_element "nvh264enc"
check_gst_element "nvh264dec"

echo -e "\n${BLUE}=== Test Özeti ===${NC}"
echo -e "Toplam testler: $((PASSED_TESTS + FAILED_TESTS))"
echo -e "Başarılı: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Başarısız: ${RED}$FAILED_TESTS${NC}"

# Başarı oranını hesapla
if [ $((PASSED_TESTS + FAILED_TESTS)) -gt 0 ]; then
    SUCCESS_RATE=$((PASSED_TESTS * 100 / (PASSED_TESTS + FAILED_TESTS)))
    echo -e "Başarı oranı: ${YELLOW}%$SUCCESS_RATE${NC}"
fi

echo -e "\nTest logları: $OUTPUT_DIR/"

# Memory leak testi (opsiyonel - valgrind gerektirir)
if command -v valgrind &> /dev/null; then
    echo -e "\n${YELLOW}=== Memory Leak Testi ===${NC}"
    echo "Valgrind ile memory leak kontrolü yapılıyor..."
    
    valgrind --leak-check=full --show-leak-kinds=all \
        timeout 5s $EXECUTABLE -i $TEST_VIDEO \
        > "$OUTPUT_DIR/valgrind.log" 2>&1
    
    if grep -q "definitely lost: 0 bytes" "$OUTPUT_DIR/valgrind.log"; then
        echo -e "Memory leak testi: ${GREEN}BAŞARILI${NC}"
    else
        echo -e "Memory leak testi: ${RED}BAŞARISIZ${NC}"
        echo "Detaylar için: $OUTPUT_DIR/valgrind.log"
    fi
fi

# Çıkış kodu
if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}Tüm testler başarılı!${NC}"
    exit 0
else
    echo -e "\n${RED}Bazı testler başarısız!${NC}"
    exit 1
fi