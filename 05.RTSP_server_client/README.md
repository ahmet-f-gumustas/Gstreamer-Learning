# RTSP Düşük Gecikmeli Kamera Streaming Projesi

Bu proje, GStreamer kullanarak düşük gecikmeli (< 250ms) RTSP video streaming sunucusu ve istemcisi implementasyonunu içerir.

## 🎯 Proje Amacı

- Kamera görüntüsünü ağ üzerinden minimum gecikmeyle aktarmak
- H.264 low-latency profile kullanarak optimize edilmiş video streaming
- Buffer ve queue parametrelerini ayarlayarak 250ms'nin altında gecikme elde etmek

## 📋 Gereksinimler

- C++14 veya üzeri
- CMake 3.10+
- GStreamer 1.16+
- gst-rtsp-server kütüphanesi
- V4L2 uyumlu kamera (varsayılan: /dev/video0)

### Ubuntu/Debian için kurulum:
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
    v4l-utils
```

## 🔨 Derleme

```bash
mkdir build
cd build
cmake ..
make
```

## 🚀 Kullanım

### 1. RTSP Sunucuyu Başlatma

```bash
./rtsp_cam_server
```

Sunucu varsayılan olarak 8554 portunda başlar ve stream'i `rtsp://localhost:8554/camera` adresinde yayınlar.

### 2. RTSP İstemciyi Çalıştırma

```bash
./rtsp_cam_client [rtsp_url]
```

Varsayılan olarak `rtsp://localhost:8554/camera` adresine bağlanır.

İstemci 30 saniye boyunca çalışır ve gecikme ölçümlerini toplar, ardından `latency_plot.md` raporunu oluşturur.

## 🛠️ Kod Açıklaması

### rtsp_cam_server.cpp

**Ana Bileşenler:**

1. **RTSPCameraServer Sınıfı**: RTSP sunucusunu yöneten ana sınıf
   - `configureLowLatencyPipeline()`: Düşük gecikmeli GStreamer pipeline'ı oluşturur
   - `configureFactoryForLowLatency()`: RTSP factory'yi optimize eder

2. **Pipeline Yapısı:**
   ```
   v4l2src → videoconvert → x264enc → h264parse → rtph264pay → queue → udpsink
   ```

3. **Düşük Gecikme Optimizasyonları:**
   - `x264enc tune=zerolatency`: Encoder'ı minimum gecikme için ayarlar
   - `speed-preset=ultrafast`: En hızlı encoding
   - `queue max-size-buffers=1`: Minimum buffer kullanımı
   - UDP transport: TCP'ye göre daha düşük gecikme

### rtsp_cam_client.cpp

**Ana Bileşenler:**

1. **RTSPCameraClient Sınıfı**: RTSP stream'i alan ve görüntüleyen istemci
   - `measureLatency()`: Pipeline gecikmesini ölçer
   - `saveLatencyReport()`: Detaylı rapor oluşturur

2. **Pipeline Yapısı:**
   ```
   rtspsrc → decodebin → videoconvert → queue → fpsdisplaysink
   ```

3. **Gecikme Ölçümü:**
   - Her saniye GStreamer latency query kullanarak ölçüm
   - Sonuçları kaydetme ve analiz

## 📊 Çıktılar

### 📊 Örnek Çıktı (latency_plot.md)

```bash

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
```

### latency_plot.md
İstemci çalıştıktan sonra oluşturulan rapor şunları içerir:
- Konfigürasyon detayları
- Saniye bazında gecikme ölçümleri
- İstatistiksel özet (ortalama, min, max)
- ASCII gecikme grafiği

### Demo Video Oluşturma
```bash
# GStreamer ile kayıt
gst-launch-1.0 -e \
    rtspsrc location=rtsp://localhost:8554/camera latency=0 ! \
    rtph264depay ! h264parse ! mp4mux ! \
    filesink location=demo.mp4
```

## ⚙️ Ayar Parametreleri

### Sunucu Tarafı
- `key-int-max=15`: Keyframe aralığı (düşük değer = düşük gecikme)
- `intra-refresh=true`: Progressive intra refresh
- `buffer-mode=0`: Buffer kullanmama

### İstemci Tarafı
- `latency=0`: Minimum gecikme ayarı
- `protocols=UDP`: UDP transport kullanımı
- `sync=FALSE`: Senkronizasyonu kapatma

## 🎯 Performans Hedefi

- **Hedef**: < 250ms gecikme
- **Tipik Sonuçlar**: 100-200ms arası (yerel ağda)
- **Faktörler**: Ağ kalitesi, CPU performansı, kamera özellikleri

## 🔍 Gelişmiş Özellikler
### Özel Pipeline Kullanımı
Server sınıfında setCustomPipeline() metodu ile özel pipeline tanımlayabilirsiniz:

```bash
server.setCustomPipeline(
    "( videotestsrc ! video/x-raw,width=1280,height=720 ! "
    "x264enc tune=zerolatency ! rtph264pay name=pay0 pt=96 )"
);

```

### Farklı Kamera Kullanımı
Pipeline'da device parametresini değiştirin:

```bash
"v4l2src device=/dev/video1 ! ..."
```

## 🛡️ Hata Ayıklama
### GST_DEBUG kullanımı:

```bash
GST_DEBUG=3 ./rtsp_cam_server  # Genel debug
GST_DEBUG=rtspsrc:5 ./rtsp_cam_client  # RTSP-specific debug
```

### Gecikme analizi:

```bash
# Pipeline grafiği oluşturma
GST_DEBUG_DUMP_DOT_DIR=. ./rtsp_cam_client
dot -Tpng *.dot -o pipeline.png
```

## 🔧 Sorun Giderme

1. **Kamera bulunamıyor hatası**: 
   ```bash
   ls /dev/video*  # Mevcut kameraları listele
   ```

2. **Yüksek gecikme**:
   - CPU kullanımını kontrol edin
   - Ağ bant genişliğini kontrol edin
   - Video çözünürlüğünü düşürün

3. **Bağlantı hatası**:
   - Firewall ayarlarını kontrol edin
   - Port 8554'ün açık olduğundan emin olun