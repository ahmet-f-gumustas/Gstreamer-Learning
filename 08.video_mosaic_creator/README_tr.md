# Video Mosaic Creator

GStreamer tabanlı, birden fazla video kaynağını tek bir mozaik görüntüde birleştiren profesyonel video işleme uygulaması.

## 🎯 Proje Özeti

Video Mosaic Creator, güvenlik sistemleri, canlı yayınlar, video konferans uygulamaları ve çoklu video izleme senaryoları için tasarlanmış güçlü bir video birleştirme aracıdır. GStreamer'ın compositor elementini kullanarak yüksek performanslı video işleme sağlar.

## ✨ Özellikler

### Temel Özellikler
- **Çoklu Video Kaynağı Desteği**
  - Web kameraları (V4L2)
  - RTSP akışları
  - Video dosyaları (MP4, AVI, MKV vb.)
  - HTTP/HTTPS video akışları
  - Test pattern'leri (geliştirme için)

- **Esnek Grid Layout Sistemi**
  - 2x2, 3x3, 4x4 ve özel grid boyutları
  - Dinamik hücre boyutlandırma
  - Özelleştirilebilir padding ve arka plan

- **Yüksek Performans**
  - Hardware accelerated video decoding
  - Düşük gecikme (low-latency) modu
  - CPU kullanımı optimizasyonu

### Gelişmiş Özellikler
- YAML tabanlı konfigürasyon sistemi
- Runtime'da kaynak ekleme/çıkarma
- Otomatik yeniden bağlanma (RTSP kaynakları için)
- Hata yönetimi ve logging

## 🛠️ Teknik Gereksinimler

### Sistem Gereksinimleri
- Ubuntu 20.04+ veya benzeri Linux dağıtımı
- GCC 9.0+ veya Clang 10.0+
- CMake 3.16+
- En az 4GB RAM (8GB önerilir)
- NVIDIA GPU (opsiyonel, hardware acceleration için)

### Bağımlılıklar
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
    gstreamer1.0-x \
    gstreamer1.0-alsa \
    gstreamer1.0-gl \
    gstreamer1.0-gtk3 \
    gstreamer1.0-qt5 \
    gstreamer1.0-pulseaudio

# OpenCV (opsiyonel, gelişmiş özellikler için)
sudo apt-get install -y libopencv-dev

# YAML konfigürasyon desteği
sudo apt-get install -y libyaml-cpp-dev

# Derleme araçları
sudo apt-get install -y build-essential pkg-config
```

## 📦 Kurulum

### 1. Projeyi Klonlayın
```bash
cd ~/git-projects/Gstreamer-Learning
mkdir 08.Video_Mosaic_Creator
cd 08.Video_Mosaic_Creator
# Proje dosyalarını buraya kopyalayın
```

### 2. Derleme
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 3. Debug Build (Geliştirme için)
```bash
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

## 🚀 Kullanım

### Temel Kullanım
```bash
# Varsayılan konfigürasyon ile çalıştırma
./video-mosaic-creator

# Özel konfigürasyon dosyası ile
./video-mosaic-creator config/my_mosaic.yaml
```

### Örnek Senaryolar

#### 1. Güvenlik Kamera Sistemi (4 Kamera)
```yaml
# config/security_cameras.yaml
output:
  width: 1920
  height: 1080
  
layout:
  grid_cols: 2
  grid_rows: 2
  cell_padding: 2
  background_color: "#1a1a1a"
  
sources:
  - name: "FrontDoor"
    uri: "rtsp://192.168.1.100:554/stream1"
    position: [0, 0]
  
  - name: "Backyard"
    uri: "rtsp://192.168.1.101:554/stream1"
    position: [1, 0]
  
  - name: "Garage"
    uri: "rtsp://192.168.1.102:554/stream1"
    position: [0, 1]
  
  - name: "Driveway"
    uri: "rtsp://192.168.1.103:554/stream1"
    position: [1, 1]
```

#### 2. Video Konferans (3x3 Grid)
```yaml
# config/video_conference.yaml
output:
  width: 1920
  height: 1080
  
layout:
  grid_cols: 3
  grid_rows: 3
  cell_padding: 5
  background_color: "#2d2d2d"
```

#### 3. Test ve Geliştirme
```bash
# Test pattern'leri ile hızlı test
./video-mosaic-creator

# Programatik olarak kaynak ekleme örneği kodda mevcuttur
```

### Komut Satırı Parametreleri
```bash
./video-mosaic-creator [options]
  -c, --config <file>     Konfigürasyon dosyası
  -v, --verbose          Detaylı log çıktısı
  -h, --help             Yardım mesajı
```

## 🏗️ Mimari

### Pipeline Yapısı
```
┌─────────────────────────────────────────────────────────────┐
│                      Video Mosaic Pipeline                   │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────┐   ┌──────────┐   ┌───────────┐               │
│  │  Source  │──▶│ Decoder  │──▶│ Converter │──┐            │
│  └──────────┘   └──────────┘   └───────────┘  │            │
│                                                 ▼            │
│  ┌──────────┐   ┌──────────┐   ┌───────────┐  ┌────────┐   │
│  │  Source  │──▶│ Decoder  │──▶│ Converter │─▶│        │   │
│  └──────────┘   └──────────┘   └───────────┘  │ Compo- │   │
│                                                 │ sitor  │──▶│ Display
│  ┌──────────┐   ┌──────────┐   ┌───────────┐  │        │   │
│  │  Source  │──▶│ Decoder  │──▶│ Converter │─▶│        │   │
│  └──────────┘   └──────────┘   └───────────┘  └────────┘   │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Sınıf Diyagramı
```
VideoMosaic
    ├── InputManager
    │   └── VideoInput[]
    │       ├── source
    │       ├── decoder
    │       ├── converter
    │       └── scaler
    └── MosaicLayout
        └── LayoutConfig
```

## 🔧 Geliştirme

### Kod Yapısı
```
src/
├── main.cpp              # Ana program giriş noktası
├── video_mosaic.cpp      # Ana pipeline yönetimi
├── input_manager.cpp     # Video kaynak yönetimi
└── mosaic_layout.cpp     # Grid layout hesaplamaları

include/
├── video_mosaic.h        # VideoMosaic sınıfı
├── input_manager.h       # InputManager sınıfı
└── mosaic_layout.h       # MosaicLayout sınıfı
```

### Yeni Özellik Ekleme

#### 1. Yeni Video Kaynağı Tipi Ekleme
```cpp
// input_manager.cpp içinde createSourceElement() fonksiyonuna ekleyin
else if (uri.find("yeni_protokol://") == 0) {
    source = gst_element_factory_make("yeni_element", nullptr);
    // Konfigürasyon...
}
```

#### 2. Overlay Text Ekleme
```cpp
// Her video üzerine isim yazdırma
GstElement* textoverlay = gst_element_factory_make("textoverlay", nullptr);
g_object_set(textoverlay, 
    "text", input->name.c_str(),
    "valignment", 2,  // top
    "halignment", 0,  // left
    nullptr);
```

## 🎮 Gelişmiş Kullanım Örnekleri

### RTSP Sunucu Entegrasyonu
```cpp
// Mozaik çıktısını RTSP üzerinden yayınlama
GstElement* rtsp_sink = gst_element_factory_make("rtspclientsink", nullptr);
g_object_set(rtsp_sink, "location", "rtsp://localhost:8554/mosaic", nullptr);
```

### Kayıt Özelliği
```cpp
// Mozaik çıktısını dosyaya kaydetme
GstElement* filesink = gst_element_factory_make("filesink", nullptr);
g_object_set(filesink, "location", "output_mosaic.mp4", nullptr);
```

### Motion Detection
```cpp
// Her kaynağa motion detection ekleme
GstElement* motion = gst_element_factory_make("motioncells", nullptr);
g_object_set(motion, "sensitivity", 0.5, nullptr);
```

## 🐛 Sorun Giderme

### Yaygın Hatalar ve Çözümleri

1. **"Failed to create pipeline" hatası**
   ```bash
   # GStreamer kurulumunu kontrol edin
   gst-inspect-1.0 | grep compositor
   ```

2. **RTSP bağlantı sorunları**
   ```bash
   # RTSP kaynağını test edin
   gst-launch-1.0 rtspsrc location=rtsp://... ! fakesink
   ```

3. **Performans sorunları**
   - Hardware decoding'i etkinleştirin
   - Grid boyutunu azaltın
   - Video çözünürlüğünü düşürün

### Debug Logları
```bash
# Detaylı GStreamer logları
GST_DEBUG=3 ./video-mosaic-creator

# Sadece compositor logları
GST_DEBUG=compositor:5 ./video-mosaic-creator
```

## 📊 Performans İpuçları

1. **Hardware Acceleration**
   ```cpp
   // NVIDIA hardware decoder kullanımı
   decoder = gst_element_factory_make("nvv4l2decoder", nullptr);
   ```

2. **Buffer Optimizasyonu**
   ```cpp
   g_object_set(source, "buffer-size", 2048000, nullptr);
   ```

3. **Thread Kullanımı**
   ```cpp
   g_object_set(pipeline, "max-threads", 4, nullptr);
   ```

## 🤝 Katkıda Bulunma

1. Fork yapın
2. Feature branch oluşturun (`git checkout -b feature/amazing-feature`)
3. Değişikliklerinizi commit edin (`git commit -m 'Add amazing feature'`)
4. Branch'e push yapın (`git push origin feature/amazing-feature`)
5. Pull Request açın

## 📄 Lisans

Bu proje MIT lisansı altında lisanslanmıştır.

## 🔗 Faydalı Kaynaklar

- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [GStreamer Compositor Plugin](https://gstreamer.freedesktop.org/documentation/compositor/)
- [YAML-CPP Documentation](https://github.com/jbeder/yaml-cpp/wiki)

## 💡 Gelecek Geliştirmeler

- [ ] Web arayüzü ile kontrol
- [ ] Dinamik layout değiştirme
- [ ] Audio mixing desteği
- [ ] AI tabanlı sahne algılama
- [ ] Cloud streaming desteği
- [ ] Docker container desteği
- [ ] Prometheus metrics entegrasyonu