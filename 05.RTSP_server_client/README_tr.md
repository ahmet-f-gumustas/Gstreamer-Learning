# RTSP Düşük Gecikmeli Kamera Streaming Projesi

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-14-blue.svg" alt="C++ Version">
  <img src="https://img.shields.io/badge/GStreamer-1.16%2B-green.svg" alt="GStreamer Version">
  <img src="https://img.shields.io/badge/Target%20Latency-%3C250ms-orange.svg" alt="Target Latency">
</p>

## 📝 Proje Hakkında

Bu proje, GStreamer kütüphanesi kullanılarak geliştirilmiş düşük gecikmeli RTSP video streaming sunucusu ve istemcisi implementasyonudur. Hedef olarak 250ms'nin altında end-to-end gecikme süresi ile gerçek zamanlı video aktarımı sağlanmaktadır.

### 🎯 Temel Özellikler

- ✅ **Ultra düşük gecikme** (< 250ms)
- ✅ **H.264 video codec** desteği
- ✅ **UDP/RTSP protokol** kullanımı
- ✅ **Otomatik gecikme ölçümü** ve raporlama
- ✅ **Optimize edilmiş buffer yönetimi**
- ✅ **Gerçek zamanlı FPS gösterimi**

## 🚀 Hızlı Başlangıç

### Sistem Gereksinimleri

- **İşletim Sistemi**: Linux (Ubuntu 20.04+ önerilir)
- **Derleyici**: GCC 7+ veya Clang 6+
- **CMake**: 3.10 veya üzeri
- **GStreamer**: 1.16 veya üzeri
- **Donanım**: V4L2 uyumlu USB/dahili kamera

### 📦 Bağımlılıkların Kurulumu

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-rtsp-server-1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    gstreamer1.0-x \
    v4l-utils
```

#### Fedora/RHEL:
```bash
sudo dnf install -y \
    cmake \
    gcc-c++ \
    gstreamer1-devel \
    gstreamer1-plugins-base-devel \
    gstreamer1-rtsp-server-devel \
    gstreamer1-plugins-good \
    gstreamer1-plugins-bad-free \
    gstreamer1-plugins-ugly \
    v4l-utils
```

### 🔨 Derleme ve Kurulum

```bash
# Projeyi klonla
git clone <proje-url>
cd rtsp-camera-streaming

# Build dizini oluştur
mkdir build && cd build

# CMake ile yapılandır
cmake ..

# Derle
make -j$(nproc)

# (Opsiyonel) Sisteme kur
sudo make install
```

## 📖 Kullanım Kılavuzu

### 1. RTSP Sunucuyu Başlatma

```bash
# Varsayılan ayarlarla başlat
./rtsp_cam_server

# Çıktı:
# === Low-Latency Configuration ===
# Encoder: x264enc with tune=zerolatency
# Speed Preset: ultrafast
# Profile: baseline (lowest complexity)
# Queue: max-size-buffers=1 (minimal buffering)
# Transport: UDP (lower latency than TCP)
# Target Latency: < 250ms
# ================================
# 
# RTSP server started at rtsp://127.0.0.1:8554/camera
# Low-latency RTSP camera server is running...
# Stream URL: rtsp://localhost:8554/camera
# Press Ctrl+C to stop
```

### 2. RTSP İstemciyi Çalıştırma

```bash
# Varsayılan sunucuya bağlan
./rtsp_cam_client

# Özel URL ile bağlan
./rtsp_cam_client rtsp://192.168.1.100:8554/camera

# Çıktı:
# Starting RTSP client...
# Connecting to: rtsp://localhost:8554/camera
# RTSP client is running...
# Press Ctrl+C to stop
# Current latency: 185.43 ms ✓ (Target achieved!)
# Current latency: 192.21 ms ✓ (Target achieved!)
# ...
```

İstemci otomatik olarak 30 saniye çalışır ve `latency_plot.md` raporunu oluşturur.

## 🔧 Yapılandırma ve Optimizasyon

### Sunucu Tarafı Optimizasyonları

| Parametre | Değer | Açıklama |
|-----------|-------|----------|
| `tune` | zerolatency | x264enc'i minimum gecikme için optimize eder |
| `speed-preset` | ultrafast | En hızlı encoding preset'i |
| `key-int-max` | 15 | Maksimum GOP boyutu (düşük = düşük gecikme) |
| `intra-refresh` | true | Progressive intra refresh aktif |
| `profile` | baseline | En düşük kompleksiteli H.264 profili |
| `max-size-buffers` | 1 | Queue'da maksimum 1 frame |
| `sync` | false | Pipeline senkronizasyonu kapalı |

### İstemci Tarafı Optimizasyonları

| Parametre | Değer | Açıklama |
|-----------|-------|----------|
| `latency` | 0 | Minimum gecikme ayarı |
| `buffer-mode` | 0 | Jitterbuffer kullanmama |
| `protocols` | UDP | Düşük gecikmeli transport |
| `do-rtcp` | TRUE | RTCP feedback aktif |
| `leaky` | downstream | Eski frame'leri düşür |

## 🛠️ Gelişmiş Özellikler

### Özel Pipeline Tanımlama

```cpp
// Sunucu kodunda özel pipeline kullanımı
server.setCustomPipeline(
    "( v4l2src device=/dev/video1 ! "
    "video/x-raw,width=1920,height=1080,framerate=60/1 ! "
    "videoconvert ! "
    "x264enc tune=zerolatency speed-preset=superfast ! "
    "rtph264pay name=pay0 pt=96 )"
);
```

### Farklı Video Kaynakları

```bash
# Test pattern kullanımı
"videotestsrc pattern=ball ! ..."

# IP kamera kullanımı  
"rtspsrc location=rtsp://camera-ip/stream ! ..."

# Video dosyası kullanımı
"filesrc location=video.mp4 ! decodebin ! ..."
```

## 🐛 Hata Ayıklama

### 1. GStreamer Debug Logları

```bash
# Genel debug (Level 3)
GST_DEBUG=3 ./rtsp_cam_server

# Modül bazlı debug
GST_DEBUG=rtspsrc:5,rtph264pay:4 ./rtsp_cam_client

# Tüm RTSP modülleri için debug
GST_DEBUG=rtsp*:5 ./rtsp_cam_server

# Debug log dosyasına yazma
GST_DEBUG=4 ./rtsp_cam_client 2> client_debug.log
```

### 2. Pipeline Görselleştirme

```bash
# Pipeline grafiği oluşturma
GST_DEBUG_DUMP_DOT_DIR=. ./rtsp_cam_client

# DOT dosyasını PNG'ye çevirme
dot -Tpng *.dot -o pipeline.png

# Birden fazla DOT dosyası varsa
for dot in *.dot; do
    dot -Tpng "$dot" -o "${dot%.dot}.png"
done
```

### 3. Performans Analizi

```bash
# CPU profiling
perf record -g ./rtsp_cam_server
perf report

# Gecikme analizi
GST_DEBUG="GST_TRACER:7" GST_TRACERS="latency" ./rtsp_cam_client

# Buffer kullanımı takibi
GST_DEBUG="queue:5" ./rtsp_cam_server
```

### 4. Network Analizi

```bash
# RTSP trafiğini izleme
sudo tcpdump -i any -n port 8554 -w rtsp_traffic.pcap

# Wireshark ile analiz
wireshark rtsp_traffic.pcap

# Bant genişliği kullanımı
iftop -i eth0 -f "port 8554"
```

## 📊 Performans Metrikleri

### Beklenen Sonuçlar

| Metrik | Hedef | Tipik Sonuç |
|--------|-------|-------------|
| End-to-end gecikme | < 250ms | 150-200ms |
| FPS | 30 | 29-30 |
| CPU kullanımı (sunucu) | < 30% | 15-25% |
| CPU kullanımı (istemci) | < 20% | 10-15% |
| Paket kaybı | < 0.1% | 0.01-0.05% |

### Örnek Rapor Çıktısı

```markdown
# RTSP Low-Latency Streaming Report

## Configuration
- **Stream URL**: rtsp://localhost:8554/camera
- **Target Latency**: < 250ms
- **Transport Protocol**: UDP
- **Encoder Settings**: x264enc tune=zerolatency
- **Buffer Configuration**: max-size-buffers=1

## Latency Measurements

| Time (s) | Latency (ms) | Status |
|----------|--------------|--------|
| 0 | 185.43 | ✓ Pass |
| 1 | 192.21 | ✓ Pass |
| 2 | 178.95 | ✓ Pass |
...

## Summary Statistics
- **Average Latency**: 186.73 ms
- **Minimum Latency**: 165.32 ms  
- **Maximum Latency**: 215.87 ms
- **Target Achievement**: ✓ **SUCCESS** - Average latency below 250ms target

## Latency Graph (ASCII)
```
   250 |                                          
   225 |     █                                    
   200 |   █ █ █   █                             
   175 | █ █ █ █ █ █ █ █                         
   150 |                                          
       +------------------------------------------
        Time (seconds)
```
```

## 🎬 Demo Video Oluşturma

### Basit Kayıt
```bash
# 30 saniyelik video kaydı
timeout 30 gst-launch-1.0 -e \
    rtspsrc location=rtsp://localhost:8554/camera latency=0 ! \
    rtph264depay ! h264parse ! mp4mux ! \
    filesink location=demo.mp4
```

### Gelişmiş Kayıt (Overlay ile)
```bash
# Zaman damgası ve FPS overlay'i ile kayıt
gst-launch-1.0 -e \
    rtspsrc location=rtsp://localhost:8554/camera latency=0 ! \
    rtph264depay ! h264parse ! avdec_h264 ! \
    clockoverlay time-format="%D %H:%M:%S" ! \
    fpsdisplaysink video-sink="x264enc ! mp4mux ! filesink location=demo_overlay.mp4"
```

## ❓ Sık Karşılaşılan Sorunlar

### 1. Kamera Bulunamıyor
```bash
# Mevcut kameraları listele
ls -la /dev/video*
v4l2-ctl --list-devices

# Kamera yeteneklerini kontrol et
v4l2-ctl -d /dev/video0 --list-formats-ext
```

### 2. Yüksek Gecikme (> 250ms)
- CPU frekans yönetimini kontrol edin: `cpupower frequency-info`
- Network QoS ayarlarını kontrol edin
- Video çözünürlüğünü düşürün (640x480)
- Framerate'i azaltın (15-20 FPS)

### 3. Bağlantı Hatası
```bash
# Firewall kontrolü
sudo ufw status
sudo iptables -L

# Port açma (UFW)
sudo ufw allow 8554/tcp
sudo ufw allow 8554/udp

# SELinux kontrolü (RHEL/Fedora)
sudo setenforce 0  # Geçici olarak devre dışı bırak
```

### 4. Video Kalitesi Sorunları
```bash
# Bitrate artırma
"x264enc tune=zerolatency bitrate=2000 ! ..."

# Profil değiştirme
"x264enc tune=zerolatency profile=main ! ..."
```

## 📚 Kaynaklar ve Dokümantasyon

- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [GStreamer RTSP Server](https://gstreamer.freedesktop.org/documentation/gst-rtsp-server/)
- [x264 Encoding Guide](https://trac.ffmpeg.org/wiki/Encode/H.264)
- [V4L2 API Documentation](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2.html)

## 📄 Lisans

Bu proje [MIT Lisansı](LICENSE) altında lisanslanmıştır.

## 🤝 Katkıda Bulunma

Pull request'ler kabul edilir. Büyük değişiklikler için lütfen önce neyi değiştirmek istediğinizi tartışmak üzere bir issue açın.

---

<p align="center">
  <i>GStreamer ve modern C++ ile düşük gecikmeli video streaming</i>
</p>