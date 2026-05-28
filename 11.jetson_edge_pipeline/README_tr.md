# 11 — Jetson Orin Nano Edge AI Pipeline

**[English](README.md) | [Türkçe](README_tr.md)**

**NVIDIA Jetson Orin Nano 8GB** için tasarlanmış, x86 (RTX-sınıfı GPU) üzerinde
geliştirilen ve donanım gelmeden önce her şeyi çalıştırmanıza, ölçmenize ve
ayarlamanıza olanak veren saydam bir simülasyon katmanına sahip uçtan uca
edge AI pipeline'ı.

## Öne Çıkan Özellikler

- Tek arayüzün arkasında **4 kamera backend'i**: USB / CSI / GMSL / GigE Vision
- **TensorRT** inference engine (FP32 / FP16 / INT8) — düzgün
  `IInt8EntropyCalibrator2` ile; DeepStream gerekmiyor
- **ByteTrack** çoklu nesne takipçisi — Kalman + Hungarian, saf C++
- **OrinSimulator**: x86 ölçümlerini, halka açık TOPS/bant genişliği oranlarını
  kullanarak Orin Nano 7W/15W/MAXN tahminlerine ölçekler
- Dev PC'de sessizce kapanan, Jetson'a girer girmez gerçek değer üreten
  **tegrastats parser**
- **CSV / JSON perf log** + karşılaştırma scripti (`compare_x86_vs_jetson.py`)
- x86 geliştirme ve aarch64 L4T deployment için **Docker imajları**, ek olarak
  cross-compile + `scp` deploy akışı

## Proje Yapısı

```
11.jetson_edge_pipeline/
├── include/
│   ├── camera/         # ICamera + 4 implementation + factory
│   ├── inference/      # TensorRT engine + INT8 calibrator
│   ├── tracking/       # ByteTrack, Kalman filter, Hungarian
│   ├── monitoring/     # tegrastats parser, Orin simulator, perf logger
│   └── edge_pipeline.h
├── src/                # include/ ile aynı yapı
├── config/             # pipeline.yaml, cameras.yaml, orin_nano_profiles.yaml
├── scripts/            # setup, build, benchmark, deploy, simulate
├── docker/             # Dockerfile.dev (x86) + Dockerfile.l4t (aarch64)
├── tests/              # 5 assert tabanlı unit test
├── models/             # ONNX, engine, INT8 calibration cache
└── docs/               # Setup, performance, camera, tracker, deployment notları
```

## Hızlı Başlangıç (x86 geliştirme makinesi)

```bash
# 1) Bağımlılıklar
./scripts/setup_dev_env.sh

# 2) Build
mkdir build && cd build
cmake -DBUILD_TESTS=ON ..
make -j$(nproc)
ctest

# 3) Model — YOLOv8n'i ONNX'e, sonra TensorRT engine'ine çevirir
python3 scripts/build_int8_engine.py \
    --model yolov8n.pt --precision fp16 \
    --output models/yolov8n_fp16.engine

# 4) Auto-detect kamerayla çalıştır
./build/jetson_edge --config config/pipeline.yaml

# 5) Headless benchmark — logs/perf.csv + perf_summary.json yazar
./build/jetson_edge --config config/pipeline.yaml --headless --benchmark

# 6) x86 ölçümlerini Orin Nano tahminine çevir
python3 scripts/simulate_orin_nano.py logs/perf.csv --host-gpu "RTX 4090"
```

## Kamera Backend'leri

| Backend | GStreamer element             | Tipik cihaz                   | Not                                                       |
|---------|------------------------------|-------------------------------|-----------------------------------------------------------|
| USB     | `v4l2src device=/dev/video0` | Logitech C920, Razer Kiyo     | Evrensel fallback, MJPG yolu destekli                     |
| CSI     | `nvarguscamerasrc sensor-id` | IMX219, IMX477, IMX708        | **Sadece Jetson'da**; NVMM zero-copy                      |
| GMSL    | `v4l2src` + `max96712` driver | Leopard LI-IMX390, D3 Eng.    | Deserializer kernel modülü yüklü olmalı                   |
| GigE    | `aravissrc camera-name=…`    | Basler ace, FLIR Blackfly S   | Aravis 0.8 dev; jumbo frame önerilir                      |

Tespit edilen tüm kameraları görmek için `./build/jetson_edge --list`.

## Orin Nano Simülasyonu

Simülatör, x86 ölçümlerini karışık bir compute/memory modeliyle ölçekler:

```
orin_fps = host_fps × (compute_bound × TOPS_ratio + (1 − compute_bound) × BW_ratio)
```

| | Orin Nano 7W | Orin Nano 15W | Orin Nano MAXN |
|--|--:|--:|--:|
| INT8 TOPS | 20 | 40 | 50 |
| FP16 TFLOPS | 5 | 10 | 12.5 |
| LPDDR5 BW (GB/s) | 68 | 68 | 68 |
| TDP (W) | 7 | 15 | ~20 |

Compute-bound katsayısı varsayılan 0.7 — YOLOv8 için iyi uyar; başka ağlar için
`OrinSimulator::setComputeBoundFraction()` ile ayarlayın.  Referans benchmark'lar
[config/orin_nano_profiles.yaml](config/orin_nano_profiles.yaml) içinde.

## Gerçek Jetson'a Deploy

```bash
# 1) QEMU altında cross-compile (veya Jetson'da yerel build)
./scripts/cross_compile.sh

# 2) Her şeyi gönder + systemd unit kur
./scripts/deploy_to_jetson.sh 192.168.1.42 nvidia

# 3) Jetson üzerinde — engine'i on-board GPU için yeniden build
ssh nvidia@192.168.1.42
cd ~/jetson_edge
python3 scripts/build_int8_engine.py --precision int8 \
        --output models/yolov8n_int8.engine
./bin/jetson_edge --config config/pipeline.yaml --power 15w
```

⚠️ TensorRT engine'leri GPU mimarisine özgüdür — **x86'da üretilen engine
Jetson'da çalışmaz** (ve tersi). Her zaman hedef üzerinde yeniden build edin.

## Dokümantasyon

- [`docs/ORIN_NANO_SPECS.md`](docs/ORIN_NANO_SPECS.md) — donanım referans kartı
- [`docs/CAMERA_TYPES.md`](docs/CAMERA_TYPES.md) — sensör backend seçimi
- [`docs/BYTETRACK_DESIGN.md`](docs/BYTETRACK_DESIGN.md) — tracker iç yapısı
- [`docs/SIMULATION_METHOD.md`](docs/SIMULATION_METHOD.md) — ölçekleme formülleri
- [`docs/DEPLOYMENT.md`](docs/DEPLOYMENT.md) — Docker, cross-compile, systemd

## Lisans

Apache 2.0 — repo'nun geri kalanıyla aynı.
