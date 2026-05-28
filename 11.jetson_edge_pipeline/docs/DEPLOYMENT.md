# Jetson'a Deployment

Bu doküman üç deployment yolunu açıklar:

1. **Direkt Jetson üzerinde native build** (en kolay, en yavaş)
2. **x86'da cross-compile + scp** (orta — bu projedeki default)
3. **Docker L4T container ile** (CI/CD için)

## 1. Native Build (Jetson Host'ta)

```bash
# Jetson'a SSH
ssh nvidia@<jetson-ip>

# Bağımlılıklar
sudo apt update
sudo apt install -y \
    build-essential cmake ninja-build pkg-config \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    libopencv-dev libyaml-cpp-dev libaravis-0.8-dev

# Repo'yu klonla
git clone <repo-url> ~/jetson_edge && cd ~/jetson_edge/11.jetson_edge_pipeline

# Build
mkdir build && cd build
cmake -DBUILD_TESTS=ON ..
make -j$(nproc)
ctest

# Model
cd ..
python3 scripts/build_int8_engine.py --model yolov8n.pt \
        --precision int8 --output models/yolov8n_int8.engine

# Çalıştır
./build/jetson_edge --config config/pipeline.yaml --power 15w
```

**Süre**: Orin Nano'da `cmake + make` yaklaşık **8-15 dakika**.  ccache
kullanırsanız tekrar build'lar 1-2 dakikaya iner.

## 2. Cross-Compile + Deploy (x86'dan)

```bash
# x86 host'ta
./scripts/cross_compile.sh
./scripts/deploy_to_jetson.sh 192.168.1.42 nvidia
```

Cross-compile QEMU + Docker buildx kullanır — Jetson hardware'i gerekmez.

**Tradeoff**: TensorRT engine GPU-specific olduğu için her zaman Jetson'da
yeniden build edilmelidir.  Script ONNX + calibration cache'i gönderir,
engine'i Jetson'da `build_int8_engine.py` ile üretirsiniz.

## 3. Docker L4T

```bash
# x86 host'ta image build (buildx + qemu)
docker buildx build --platform linux/arm64 \
    -f docker/Dockerfile.l4t -t edge:l4t .

# Image'i save et + Jetson'a kopyala
docker save edge:l4t | ssh nvidia@<jetson-ip> 'docker load'

# Jetson'da çalıştır
ssh nvidia@<jetson-ip>
docker run --rm --runtime nvidia \
    -v $(pwd):/work \
    -v /tmp/argus_socket:/tmp/argus_socket \
    --device /dev/video0 \
    --network host \
    edge:l4t /work/build/jetson_edge --config /work/config/pipeline.yaml
```

**Önemli**: Container içinden CSI kamera kullanmak için `--runtime nvidia` ve
Argus socket mount şart.  USB kamera için `--device /dev/videoX` yeter.

## systemd Servisi

`deploy_to_jetson.sh` aşağıdaki unit file'ı `/etc/systemd/system/jetson-edge.service`
altına yazar:

```ini
[Unit]
Description=Jetson Edge Pipeline
After=network.target nvargus-daemon.service

[Service]
Type=simple
User=nvidia
WorkingDirectory=/home/nvidia/jetson_edge
ExecStart=/home/nvidia/jetson_edge/bin/jetson_edge \
          --config /home/nvidia/jetson_edge/config/pipeline.yaml \
          --headless
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Etkinleştirmek için:
```bash
sudo systemctl enable jetson-edge
sudo systemctl start  jetson-edge
journalctl -u jetson-edge -f
```

## Üretim Önerileri

### Read-only rootfs
Boot süresini düşürür + SD kart aşınmasını yavaşlatır:
```bash
sudo systemctl mask systemd-update-utmp.service
# fstab'da `/` mount'unu `ro` yap
# /var/log için overlay tmpfs ekle
```

### Watchdog
Pipeline takılırsa sistemi reset etmek için:
```bash
sudo apt install watchdog
# /etc/watchdog.conf:
#   ping = 127.0.0.1
#   pidfile = /var/run/jetson-edge.pid
#   interval = 5
```

### Log rotation
`logs/perf.csv` ve `logs/perf_summary.json` üst üste yazar — uzun süreli
çalıştırma için logrotate yapılandırılmalı:

```
/home/nvidia/jetson_edge/logs/*.csv {
    daily
    rotate 7
    compress
    missingok
    notifempty
}
```

### OTA Update (basit yaklaşım)

```bash
# Yeni binary
scp build/jetson_edge nvidia@<jetson-ip>:/tmp/

# Atomik geçiş
ssh nvidia@<jetson-ip> "
    sudo systemctl stop jetson-edge
    mv /tmp/jetson_edge ~/jetson_edge/bin/
    sudo systemctl start jetson-edge
"
```

Daha gelişmiş çözüm için **Mender** veya **balena.io** kullanılabilir,
ama küçük dağıtımlarda script + systemd yeterli.

## Yaygın Sorunlar

### `nvargus-daemon` başlatılamadı
CSI kamera kullanılıyorsa ve container'dan çalışıyorsa Argus socket
mount edilmemiştir:
```bash
docker run … -v /tmp/argus_socket:/tmp/argus_socket …
```

### TensorRT engine yüklenmiyor
Hata mesajı: `ICudaEngine deserializeCudaEngine returned nullptr`.
→ Engine farklı bir GPU/TensorRT sürümünde build edildi.  Jetson'da
yeniden build et.

### Yüksek CPU kullanımı (x264 fallback)
Orin Nano'da NVENC yok → çıktıyı RTSP yapıyorsan x264 CPU encode'a düşer.
Çözüm: H.265 + `nvv4l2h265enc` veya MJPEG (CPU-friendly).

### `nvpmodel: cannot find available power model`
JetPack 6.0+ sürümünde Orin Nano için profile ID'leri:
- `0` → 15W
- `1` → 7W
- (MAXN ayrı profile değil, `jetson_clocks` ile lock uygulanmış 15W)

```bash
sudo nvpmodel -q
sudo nvpmodel -m 0
sudo jetson_clocks   # max freq lock
```
