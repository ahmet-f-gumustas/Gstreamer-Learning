# GStreamer Video Analytics Pipeline

Bu proje, GStreamer kullanarak gerçek zamanlı video analizi yapan kapsamlı bir C++ uygulamasıdır. Video akışlarını işleme, hareket algılama, nesne takibi ve RTSP üzerinden yayın yapma özelliklerini içerir.

## Özellikler

- **Çoklu Kaynak Desteği**: Dosya, web kamerası, RTSP stream ve HTTP stream kaynaklarını destekler
- **Hareket Algılama**: OpenCV entegrasyonu ile gerçek zamanlı hareket tespiti
- **GPU Hızlandırma**: NVIDIA GPU'lar için NVENC/NVDEC codec desteği
- **RTSP Server**: İşlenmiş videoyu RTSP üzerinden yayınlama
- **Dinamik Pipeline**: Çalışma zamanında pipeline elemanlarını değiştirme
- **Performans İzleme**: FPS, CPU ve GPU kullanımını izleme
- **Kayıt Özelliği**: İşlenmiş videoyu farklı formatlarda kaydetme

## Gereksinimler

- Ubuntu 22.04 LTS
- GStreamer 1.20+ (tüm eklentiler dahil)
- CMake 3.22+
- GCC 11.4+
- OpenCV 4.x (opsiyonel, hareket algılama için)
- NVIDIA GPU ve sürücüleri (GPU hızlandırma için)

## Kurulum

### Bağımlılıkları Yükleme

```bash
# GStreamer ve geliştirme paketleri
sudo apt-get update
sudo apt-get install -y \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    gstreamer1.0-rtsp

# OpenCV (opsiyonel)
sudo apt-get install -y libopencv-dev

# YAML config parser
sudo apt-get install -y libyaml-cpp-dev
```

### Projeyi Derleme

```bash
# Proje dizinine gidin
cd GStreamerVideoAnalytics

# Build dizini oluşturun
mkdir -p build
cd build

# CMake ile yapılandırın
cmake ..

# Derleyin
make -j$(nproc)
```

## Kullanım

### Temel Kullanım

```bash
# Video dosyası işleme
./gstreamer_video_analytics -i /path/to/video.mp4

# Web kamerası kullanma
./gstreamer_video_analytics -i webcam

# RTSP stream işleme
./gstreamer_video_analytics -i rtsp://192.168.1.100:554/stream

# RTSP server başlatma
./gstreamer_video_analytics -i webcam -o rtsp://0.0.0.0:8554/live
```

### Gelişmiş Özellikler

```bash
# Hareket algılama aktif
./gstreamer_video_analytics -i webcam --motion-detect

# GPU hızlandırma kullan
./gstreamer_video_analytics -i video.mp4 --use-gpu

# Kayıt yapma
./gstreamer_video_analytics -i rtsp://camera.local --record output.mp4

# Özel konfigürasyon dosyası
./gstreamer_video_analytics --config config/custom_pipeline.yaml
```

## Konfigürasyon

`config/pipeline_config.yaml` dosyasını düzenleyerek pipeline ayarlarını özelleştirebilirsiniz:

```yaml
pipeline:
  input:
    type: "file"  # file, webcam, rtsp, http
    location: "assets/test_video.mp4"
  
  processing:
    motion_detection: true
    gpu_acceleration: true
    
  output:
    type: "display"  # display, file, rtsp
    location: "rtsp://0.0.0.0:8554/live"
```

## Mimari

Proje modüler bir yapıya sahiptir:

- **PipelineManager**: Ana GStreamer pipeline'ını yönetir
- **VideoProcessor**: Video işleme ve filtre uygulamaları
- **MotionDetector**: Hareket algılama algoritmaları
- **RTSPStreamer**: RTSP server yönetimi

## Performans

- 1080p@30fps video işleme (CPU)
- 4K@60fps video işleme (GPU ile)
- Düşük gecikme RTSP streaming (<100ms)
- Çoklu stream desteği (sistem kaynaklarına bağlı)

## Sorun Giderme

### GStreamer elementleri bulunamıyor
```bash
gst-inspect-1.0 | grep [element_adı]
```

### GPU hızlandırma çalışmıyor
- NVIDIA sürücülerini kontrol edin: `nvidia-smi`
- GStreamer NVCODEC eklentisini kontrol edin: `gst-inspect-1.0 nvcodec`

### RTSP bağlantı sorunları
- Firewall ayarlarını kontrol edin
- Port 8554'ün açık olduğundan emin olun

## Lisans

Bu proje MIT lisansı altında lisanslanmıştır.