#!/bin/bash

# GStreamer Video Analytics Pipeline Test Script

# Define colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project directories
PROJECT_ROOT=$(dirname $(dirname $(readlink -f $0)))
BUILD_DIR="$PROJECT_ROOT/build"
EXECUTABLE="$BUILD_DIR/gstreamer_video_analytics"
TEST_VIDEO="$PROJECT_ROOT/assets/test_video.mp4"
OUTPUT_DIR="$PROJECT_ROOT/test_outputs"

# Store test results
PASSED_TESTS=0
FAILED_TESTS=0

echo -e "${BLUE}=== GStreamer Video Analytics Pipeline Test Suite ===${NC}"
echo "Project directory: $PROJECT_ROOT"
echo "Test outputs: $OUTPUT_DIR"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check executable
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}ERROR: Executable not found!${NC}"
    echo "First build the project with './scripts/build.sh'."
    exit 1
fi

# Check test video file
if [ ! -f "$TEST_VIDEO" ]; then
    echo -e "${YELLOW}Test video not found, creating...${NC}"
    gst-launch-1.0 videotestsrc num-buffers=300 pattern=ball ! \
        video/x-raw,width=1280,height=720,framerate=30/1 ! \
        x264enc ! mp4mux ! \
        filesink location="$TEST_VIDEO" 2>/dev/null
fi

# Test function
run_test() {
    local test_name=$1
    local test_cmd=$2
    local expected_result=${3:-0}

    echo -ne "Test: $test_name ... "

    # Save test log
    local log_file="$OUTPUT_DIR/${test_name// /_}.log"

    # Run test (with timeout)
    timeout 10s $test_cmd > "$log_file" 2>&1
    local result=$?

    # Check timeout status
    if [ $result -eq 124 ]; then
        # Timeout occurred, this is normal for some tests
        if [ $expected_result -eq 124 ]; then
            echo -e "${GREEN}PASSED${NC} (Timeout expected)"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}FAILED${NC} (Unexpected timeout)"
            ((FAILED_TESTS++))
        fi
    elif [ $result -eq $expected_result ]; then
        echo -e "${GREEN}PASSED${NC}"
        ((PASSED_TESTS++))
    else
        echo -e "${RED}FAILED${NC} (Code: $result, Expected: $expected_result)"
        ((FAILED_TESTS++))
    fi
}

echo -e "${YELLOW}=== Basic Functionality Tests ===${NC}"

# Test 1: Help message
run_test "Help Message" "$EXECUTABLE --help" 1

# Test 2: Video file playback
run_test "Video File Playback" "$EXECUTABLE -i $TEST_VIDEO" 124

# Test 3: Invalid file
run_test "Invalid File" "$EXECUTABLE -i /tmp/nonexistent_video.mp4" 1

# Test 4: Video file recording
run_test "Video Recording" "$EXECUTABLE -i $TEST_VIDEO -o $OUTPUT_DIR/test_output.mp4" 124

echo -e "\n${YELLOW}=== Motion Detection Tests ===${NC}"

# Test 5: Motion detection
run_test "Motion Detection" "$EXECUTABLE -i $TEST_VIDEO --motion-detect" 124

# Test 6: Motion detection + recording
run_test "Motion Detection + Recording" "$EXECUTABLE -i $TEST_VIDEO --motion-detect --record $OUTPUT_DIR/motion_test.mp4" 124

echo -e "\n${YELLOW}=== Video Processing Tests ===${NC}"

# Test 7: Resolution change
run_test "Resolution Change" "$EXECUTABLE -i $TEST_VIDEO --width 640 --height 480" 124

# Test 8: FPS change
run_test "FPS Change" "$EXECUTABLE -i $TEST_VIDEO --fps 15" 124

# Test 9: Bitrate setting
run_test "Bitrate Setting" "$EXECUTABLE -i $TEST_VIDEO --bitrate 2000000 -o $OUTPUT_DIR/low_bitrate.mp4" 124

echo -e "\n${YELLOW}=== GPU Acceleration Tests ===${NC}"

# Check GPU availability
if nvidia-smi &>/dev/null; then
    # Test 10: GPU acceleration
    run_test "GPU Acceleration" "$EXECUTABLE -i $TEST_VIDEO --use-gpu" 124

    # Test 11: GPU + motion detection
    run_test "GPU + Motion Detection" "$EXECUTABLE -i $TEST_VIDEO --use-gpu --motion-detect" 124
else
    echo -e "${YELLOW}GPU not found, skipping GPU tests${NC}"
fi

echo -e "\n${YELLOW}=== RTSP Streaming Tests ===${NC}"

# Test 12: RTSP server start
run_test "RTSP Server" "$EXECUTABLE -i $TEST_VIDEO -o rtsp://0.0.0.0:8554/test" 124

# Test 13: RTSP with custom port
run_test "RTSP Custom Port" "$EXECUTABLE -i $TEST_VIDEO -o rtsp://0.0.0.0:8555/test --rtsp-port 8555" 124

echo -e "\n${YELLOW}=== Configuration File Tests ===${NC}"

# Create test config file
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

# Test 14: Config file usage
run_test "Config File" "$EXECUTABLE -c $OUTPUT_DIR/test_config.yaml" 124

echo -e "\n${YELLOW}=== Performance Tests ===${NC}"

# Test 15: Large video processing (if available)
if [ -f "$PROJECT_ROOT/assets/large_video.mp4" ]; then
    run_test "Large Video Processing" "$EXECUTABLE -i $PROJECT_ROOT/assets/large_video.mp4 --use-gpu" 124
else
    echo -e "${YELLOW}Large video file not found, skipping performance test${NC}"
fi

echo -e "\n${YELLOW}=== Pipeline Stability Tests ===${NC}"

# Test 16: Multiple start/stop
for i in {1..3}; do
    run_test "Start/Stop Test $i" "timeout 2s $EXECUTABLE -i $TEST_VIDEO" 124
done

echo -e "\n${YELLOW}=== GStreamer Element Tests ===${NC}"

# Check GStreamer elements
check_gst_element() {
    local element=$1
    if gst-inspect-1.0 $element &>/dev/null; then
        echo -e "  $element: ${GREEN}✓${NC}"
    else
        echo -e "  $element: ${RED}✗${NC}"
    fi
}

echo "Required GStreamer elements:"
check_gst_element "filesrc"
check_gst_element "decodebin"
check_gst_element "videoconvert"
check_gst_element "videoscale"
check_gst_element "x264enc"
check_gst_element "mp4mux"
check_gst_element "autovideosink"
check_gst_element "rtspserver"

# Check NVCODEC elements (optional)
echo -e "\nOptional GPU elements:"
check_gst_element "nvh264enc"
check_gst_element "nvh264dec"

echo -e "\n${BLUE}=== Test Summary ===${NC}"
echo -e "Total tests: $((PASSED_TESTS + FAILED_TESTS))"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

# Calculate success rate
if [ $((PASSED_TESTS + FAILED_TESTS)) -gt 0 ]; then
    SUCCESS_RATE=$((PASSED_TESTS * 100 / (PASSED_TESTS + FAILED_TESTS)))
    echo -e "Success rate: ${YELLOW}${SUCCESS_RATE}%${NC}"
fi

echo -e "\nTest logs: $OUTPUT_DIR/"

# Memory leak test (optional - requires valgrind)
if command -v valgrind &> /dev/null; then
    echo -e "\n${YELLOW}=== Memory Leak Test ===${NC}"
    echo "Running memory leak check with Valgrind..."

    valgrind --leak-check=full --show-leak-kinds=all \
        timeout 5s $EXECUTABLE -i $TEST_VIDEO \
        > "$OUTPUT_DIR/valgrind.log" 2>&1

    if grep -q "definitely lost: 0 bytes" "$OUTPUT_DIR/valgrind.log"; then
        echo -e "Memory leak test: ${GREEN}PASSED${NC}"
    else
        echo -e "Memory leak test: ${RED}FAILED${NC}"
        echo "Details: $OUTPUT_DIR/valgrind.log"
    fi
fi

# Exit code
if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}Some tests failed!${NC}"
    exit 1
fi
