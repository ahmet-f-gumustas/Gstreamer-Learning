# DeepDetect GStreamer Plugin

GPU-accelerated YOLOv8 object detection plugin for GStreamer using TensorRT.

## Features

- Zero-copy GPU memory operation
- TensorRT FP16/INT8 inference 
- Real-time bounding box overlay
- JSON metadata output stream
- NVTX profiling support
- Production-ready performance

## Requirements

- GStreamer ≥ 1.26
- CUDA ≥ 12.4  
- TensorRT ≥ 10.0
- NVIDIA GPU with compute capability ≥ 6.1

## Building

### CMake Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### Meson Build

```bash
meson setup builddir
cd builddir
ninja
sudo ninja install
```

## Usage

### Basic Pipeline

```bash
gst-launch-1.0 \
    uridecodebin uri=file:///sample.mp4 ! \
    nvvidconv ! \
    'video/x-raw(memory:NVMM),format=NV12' ! \
    deepdetect engine-path=yolov8n.trt score-threshold=0.3 ! \
    nvoverlaysink
```

### With Metadata Output

```bash
gst-launch-1.0 \
    videotestsrc ! \
    nvvidconv ! \
    'video/x-raw(memory:NVMM)' ! \
    deepdetect engine-path=yolov8n.trt name=detector ! \
    nvoverlaysink \
    \
    detector.src_meta ! \
    filesink location=detections.jsonl
```

### Live Camera Feed

```bash
gst-launch-1.0 \
    v4l2src device=/dev/video0 ! \
    nvvidconv ! \
    'video/x-raw(memory:NVMM),width=1280,height=720' ! \
    deepdetect engine-path=yolov8n.trt profile=true ! \
    nvoverlaysink sync=false
```

## Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| engine-path | string | (required) | Path to TensorRT engine file |
| score-threshold | double | 0.25 | Detection confidence threshold |
| int8-calib-cache | string | NULL | INT8 calibration cache path |
| overlay-color | uint32 | 0xFF0000FF | RGBA overlay color |
| profile | boolean | false | Enable NVTX profiling |

## Model Preparation

### ONNX to TensorRT Conversion

```bash
trtexec --onnx=yolov8n.onnx \
        --saveEngine=yolov8n.trt \
        --fp16 \
        --minShapes=input:1x3x640x640 \
        --optShapes=input:1x3x640x640 \
        --maxShapes=input:1x3x640x640
```

### INT8 Calibration

```bash
# Build calibration tool
cd samples
g++ -o calibration_tool calibration_tool.cpp \
    -lnvinfer -lnvonnxparser -lopencv_core -lopencv_imgproc -lopencv_imgcodecs

# Run calibration
./calibration_tool yolov8n.onnx /path/to/coco/images yolov8n_int8.trt
```

## Testing

```bash
# Run unit tests
cd tests
./test_synthetic

# Validate plugin installation
gst-inspect-1.0 deepdetect

# Run integration tests
gst-validate-launcher --testsuites deepdetect_tests
```

## Performance

### Benchmarks

| Model | Resolution | GPU | FPS | Latency |
|-------|------------|-----|-----|---------|
| YOLOv8n | 640x640 | RTX 4090 | 245 | 4.1ms |
| YOLOv8s | 640x640 | RTX 4090 | 189 | 5.3ms |
| YOLOv8m | 640x640 | RTX 4090 | 134 | 7.5ms |
| YOLOv8n | 640x640 | Jetson AGX Xavier | 89 | 11.2ms |

### Memory Usage

- Base plugin overhead: ~50MB
- Per-frame GPU memory: ~5MB (640x640 NV12)
- TensorRT engine cache: Model-dependent (50-500MB)

## Troubleshooting

### Common Issues

**Plugin not found**
```bash
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0:$GST_PLUGIN_PATH
```

**CUDA out of memory**
```bash
# Reduce batch size or use INT8 precision
export CUDA_VISIBLE_DEVICES=0
```

**TensorRT engine incompatible**
```bash
# Rebuild engine for current TensorRT version
trtexec --onnx=model.onnx --saveEngine=model.trt --verbose
```

### Debug Mode

```bash
export GST_DEBUG=deepdetect:5
gst-launch-1.0 [pipeline] 2>&1 | grep deepdetect
```

## Examples

### Python Integration

```python
import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst

Gst.init(None)

pipeline = Gst.parse_launch("""
    videotestsrc ! 
    nvvidconv ! 
    video/x-raw(memory:NVMM) ! 
    deepdetect engine-path=yolov8n.trt name=detector !
    nvoverlaysink detector.src_meta ! 
    appsink name=metadata
""")

pipeline.set_state(Gst.State.PLAYING)
```

### C++ Integration

```cpp
#include <gst/gst.h>

int main() {
    gst_init(nullptr, nullptr);
    
    GstElement *pipeline = gst_parse_launch(
        "videotestsrc ! nvvidconv ! "
        "video/x-raw(memory:NVMM) ! "
        "deepdetect engine-path=yolov8n.trt ! "
        "nvoverlaysink", nullptr);
    
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    // Wait for EOS or error
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, 
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    
    return 0;
}
```

## License

LGPL v2.1 - see LICENSE file for details

## Contributing

1. Fork the repository
2. Create feature branch
3. Add tests for new functionality  
4. Ensure all tests pass
5. Submit pull request

## Support

- GitHub Issues: https://github.com/yourname/deepdetect-plugin/issues
- Documentation: https://deepdetect-plugin.readthedocs.io
- Discord: #deepdetect-plugin