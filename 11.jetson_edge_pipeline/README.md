# 11 — Jetson Orin Nano Edge AI Pipeline

**[English](README.md) | [Türkçe](README_tr.md)**

End-to-end edge AI pipeline targeting **NVIDIA Jetson Orin Nano 8GB**, developed
on x86 (RTX-class GPU) with a transparent simulation layer that lets you run,
benchmark, and tune everything *before* the hardware lands on your desk.

## Highlights

- **4 camera backends** behind one abstraction: USB / CSI / GMSL / GigE Vision
- **TensorRT** inference engine (FP32 / FP16 / INT8) with proper
  `IInt8EntropyCalibrator2` — no DeepStream required
- **ByteTrack** multi-object tracker — Kalman + Hungarian, pure C++
- **OrinSimulator**: scales x86 measurements into Orin Nano 7W/15W/MAXN
  estimates using the public TOPS/bandwidth ratios
- **Tegrastats parser** that runs silently on a dev PC and yields real values
  the moment you boot it on a Jetson
- **CSV / JSON perf log** + comparison script (`compare_x86_vs_jetson.py`)
- **Docker images** for x86 dev and aarch64 L4T deployment, plus a
  cross-compile + `scp` deploy flow

## Project Layout

```
11.jetson_edge_pipeline/
├── include/
│   ├── camera/         # ICamera + 4 implementations + factory
│   ├── inference/      # TensorRT engine + INT8 calibrator
│   ├── tracking/       # ByteTrack, Kalman filter, Hungarian
│   ├── monitoring/     # tegrastats parser, Orin simulator, perf logger
│   └── edge_pipeline.h
├── src/                # mirror of include/
├── config/             # pipeline.yaml, cameras.yaml, orin_nano_profiles.yaml
├── scripts/            # setup, build, benchmark, deploy, simulate
├── docker/             # Dockerfile.dev (x86) + Dockerfile.l4t (aarch64)
├── tests/              # 5 assert-based unit tests
├── models/             # ONNX, engine, INT8 calibration cache
└── docs/               # Setup, performance, camera, tracker, deployment notes
```

## Quick Start (x86 dev machine)

```bash
# 1) Dependencies
./scripts/setup_dev_env.sh

# 2) Build
mkdir build && cd build
cmake -DBUILD_TESTS=ON ..
make -j$(nproc)
ctest

# 3) Get a model — exports YOLOv8n to ONNX, then to a TensorRT engine
python3 scripts/build_int8_engine.py \
    --model yolov8n.pt --precision fp16 \
    --output models/yolov8n_fp16.engine

# 4) Run with auto-detect camera
./build/jetson_edge --config config/pipeline.yaml

# 5) Headless benchmark — writes logs/perf.csv + perf_summary.json
./build/jetson_edge --config config/pipeline.yaml --headless --benchmark

# 6) Translate x86 numbers into Orin Nano estimates
python3 scripts/simulate_orin_nano.py logs/perf.csv --host-gpu "RTX 4090"
```

## Camera Backends

| Backend | GStreamer element             | Typical device                | Notes                                                  |
|---------|------------------------------|-------------------------------|--------------------------------------------------------|
| USB     | `v4l2src device=/dev/video0` | Logitech C920, Razer Kiyo     | Universal fallback, MJPG path supported                |
| CSI     | `nvarguscamerasrc sensor-id` | IMX219, IMX477, IMX708        | **Jetson only**; NVMM zero-copy                        |
| GMSL    | `v4l2src` after `max96712`   | Leopard LI-IMX390, D3 Engineering | Needs deserializer kernel module loaded             |
| GigE    | `aravissrc camera-name=…`    | Basler ace, FLIR Blackfly S   | Aravis 0.8 dev; jumbo frames recommended               |

Run `./build/jetson_edge --list` to enumerate everything detected on the host.

## Orin Nano Simulation

The simulator scales x86 measurements with a mixed compute/memory model:

```
orin_fps = host_fps × (compute_bound × TOPS_ratio + (1 − compute_bound) × BW_ratio)
```

| | Orin Nano 7W | Orin Nano 15W | Orin Nano MAXN |
|--|--:|--:|--:|
| INT8 TOPS | 20 | 40 | 50 |
| FP16 TFLOPS | 5 | 10 | 12.5 |
| LPDDR5 BW (GB/s) | 68 | 68 | 68 |
| TDP (W) | 7 | 15 | ~20 |

The mixed-workload coefficient defaults to 0.7 (compute-bound), which matches
YOLOv8 well; tune via `OrinSimulator::setComputeBoundFraction()` for other
networks.  Reference public benchmarks shipped in
[config/orin_nano_profiles.yaml](config/orin_nano_profiles.yaml).

## Deploying to a Real Jetson

```bash
# 1) Cross-compile under QEMU (or build natively on the Jetson)
./scripts/cross_compile.sh

# 2) Push everything + install a systemd unit
./scripts/deploy_to_jetson.sh 192.168.1.42 nvidia

# 3) On the Jetson — rebuild the engine for the on-board GPU
ssh nvidia@192.168.1.42
cd ~/jetson_edge
python3 scripts/build_int8_engine.py --precision int8 \
        --output models/yolov8n_int8.engine
./bin/jetson_edge --config config/pipeline.yaml --power 15w
```

TensorRT engines are GPU-architecture-specific — the engine produced on x86
**will not run on Jetson** and vice versa.  Always rebuild on the target.

## Docs

- [`docs/ORIN_NANO_SPECS.md`](docs/ORIN_NANO_SPECS.md) — hardware reference card
- [`docs/CAMERA_TYPES.md`](docs/CAMERA_TYPES.md) — choosing a sensor backend
- [`docs/BYTETRACK_DESIGN.md`](docs/BYTETRACK_DESIGN.md) — tracker walkthrough
- [`docs/SIMULATION_METHOD.md`](docs/SIMULATION_METHOD.md) — scaling formulas
- [`docs/DEPLOYMENT.md`](docs/DEPLOYMENT.md) — Docker, cross-compile, systemd

## License

Apache 2.0 — same as the rest of the repository.
