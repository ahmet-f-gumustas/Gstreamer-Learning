# DeepDetect Plugin Design Document

## Overview

DeepDetect is a high-performance GStreamer plugin that provides GPU-accelerated object detection using TensorRT YOLOv8 models. The plugin is designed for zero-copy operation on NVIDIA GPU memory to maximize throughput and minimize latency.

## Architecture

### Core Components

1. **GstDeepDetect**: Main element class inheriting from GstBaseTransform
2. **YoloEngine**: TensorRT inference wrapper
3. **NvOverlay**: GPU overlay composition utilities
4. **Metadata Pipeline**: JSON detection output stream

### Memory Flow

```
Input Buffer (GPU) → TensorRT Inference → Detection Results → Overlay Composition → Output Buffer (GPU)
                                                          ↓
                                                   JSON Metadata (CPU)
```

### State Machine

```
NULL → READY: Initialize CUDA context, load TensorRT engine
READY → PAUSED: Allocate GPU buffers, create execution context  
PAUSED → PLAYING: Start streaming thread
PLAYING → PAUSED: Flush pending operations
PAUSED → READY: Free GPU buffers
READY → NULL: Cleanup CUDA context, unload engine
```

## Zero-Copy Strategy

### GPU Memory Management

- Input frames must use `GstCudaMemory` allocator
- TensorRT inference operates directly on GPU pointers
- Overlay composition uses GPU-based rendering
- Only detection metadata is copied to host memory

### CUDA Stream Synchronization

- Each buffer carries its associated CUDA stream
- Inference operations are queued on the buffer's stream
- Synchronization only occurs for metadata extraction

## Performance Optimizations

### TensorRT Integration

- Dynamic batching support (batch size = 1)
- FP16 precision by default
- Optional INT8 quantization with calibration
- Persistent execution context per CUDA device

### Overlay Rendering

- Hardware-accelerated composition using GstVideoOverlayComposition
- Efficient bounding box rendering on GPU
- Configurable overlay colors and styles

## Error Handling

### Graceful Degradation

- Invalid engine paths result in element creation failure
- Inference errors are logged but don't stop the pipeline
- GPU memory allocation failures trigger fallback mechanisms

### Recovery Strategies

- Automatic retry on temporary CUDA errors
- Buffer skipping on inference timeout
- Engine reloading on persistent failures

## Testing Strategy

### Unit Tests

- Synthetic frame generation with known patterns
- Golden reference validation using deterministic inputs
- Property validation and edge case handling

### Integration Tests

- End-to-end pipeline validation
- Performance benchmarking
- Memory leak detection

### Continuous Integration

- Multi-platform testing (x86_64, aarch64)
- Multiple CUDA/TensorRT version combinations
- Automated performance regression detection

## Implementation Details

### Thread Safety

- All CUDA operations use stream-based async execution
- Thread-local CUDA contexts prevent race conditions
- Atomic operations for shared state management

### Memory Management

- RAII patterns for automatic resource cleanup
- Smart pointers for TensorRT object management
- GPU memory pooling for buffer reuse

### Error Propagation

- C++ exceptions converted to GStreamer error messages
- Graceful fallback on non-critical failures
- Detailed logging for debugging support

## Future Enhancements

### Planned Features

- Multi-class tracking integration
- Dynamic model switching
- Batch processing optimization
- Custom post-processing hooks

### Scalability

- Multi-GPU support
- Distributed inference
- Cloud deployment optimizations

## Plugin Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| engine-path | string | (required) | Path to TensorRT engine file |
| score-threshold | double | 0.25 | Detection confidence threshold |
| int8-calib-cache | string | NULL | INT8 calibration cache path |
| overlay-color | uint32 | 0xFF0000FF | RGBA overlay color |
| profile | boolean | false | Enable NVTX profiling |

## Pad Templates

### Sink Pad
- Capabilities: `video/x-raw(memory:NVMM), format=(string){ NV12, RGBA }`
- Size: `width=[1,8192], height=[1,8192]`
- Framerate: `[1/1,240/1]`

### Source Pad
- Capabilities: Same as sink pad
- Passthrough with overlay composition added

### Metadata Pad
- Capabilities: `application/json`
- Contains detection results per frame