#!/bin/bash

# DeepDetect GStreamer Plugin Demo Launch Script

set -e

# Check if TensorRT engine exists
ENGINE_PATH="${1:-/opt/models/yolov8n.trt}"
if [ ! -f "$ENGINE_PATH" ]; then
    echo "Error: TensorRT engine not found at $ENGINE_PATH"
    echo "Usage: $0 [engine_path] [input_source]"
    echo "Please provide a valid YOLOv8 TensorRT engine file"
    exit 1
fi

INPUT_SOURCE="${2:-videotestsrc pattern=ball ! video/x-raw,width=1280,height=720,framerate=30/1}"

echo "Starting DeepDetect demo with:"
echo "  Engine: $ENGINE_PATH"
echo "  Source: $INPUT_SOURCE"
echo ""

# Main pipeline with metadata tee
gst-launch-1.0 -v \
    $INPUT_SOURCE ! \
    nvvidconv ! \
    'video/x-raw(memory:NVMM),format=NV12' ! \
    tee name=t ! \
    queue ! \
    deepdetect engine-path="$ENGINE_PATH" score-threshold=0.3 profile=true name=detector ! \
    nvoverlaysink \
    \
    detector.src_meta ! \
    queue ! \
    filesink location=detections.jsonl

echo ""
echo "Detection metadata will be saved to: detections.jsonl"
echo "Press Ctrl+C to stop"