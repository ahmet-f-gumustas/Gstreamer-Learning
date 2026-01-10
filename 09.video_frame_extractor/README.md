# Video Frame Extractor

GStreamer ve OpenCV tabanlı, video dosyalarından veya akışlardan kare (frame) cikaran profesyonel bir uygulama.

## Proje Ozeti

Video Frame Extractor, video analizi, makine ogrenimi veri seti hazirlama, video ozet olusturma ve benzeri senaryolar icin tasarlanmis bir aractir. GStreamer'in appsink elementini kullanarak yuksek performansli frame cikarma islemi gerceklestirir.

## Ozellikler

### Temel Ozellikler
- **Coklu Video Kaynagi Destegi**
  - Video dosyalari (MP4, AVI, MKV, WebM vb.)
  - RTSP akislari (IP kameralar)
  - HTTP/HTTPS video akislari

- **Esnek Cikarma Modlari**
  - `interval`: Belirli frame araliklarinda cikarma (ornegin her 30 frame'de bir)
  - `keyframe`: Sadece anahtar kareleri (I-frame) cikarma
  - `all`: Tum frame'leri cikarma
  - `time_based`: Zaman tabanli cikarma (ornegin her 5 saniyede bir)

- **Cikti Formatlari**
  - PNG (kayipsiz)
  - JPEG (ayarlanabilir kalite)
  - BMP

### Gelismis Ozellikler
- YAML tabanli konfigurasyon sistemi
- Komut satiri parametreleri
- Frame boyutlandirma (resize)
- Zaman damgasi overlay
- Ozel frame callback fonksiyonu
- Ilerleme gostergesi

## Teknik Gereksinimler

### Sistem Gereksinimleri
- Ubuntu 20.04+ veya benzeri Linux dagitimi
- GCC 9.0+ veya Clang 10.0+
- CMake 3.16+

### Bagimliliklar
```bash
# GStreamer ve gelistirme paketleri
sudo apt-get update
sudo apt-get install -y \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-good1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools

# OpenCV
sudo apt-get install -y libopencv-dev

# YAML konfigurasyon destegi
sudo apt-get install -y libyaml-cpp-dev

# Derleme araclari
sudo apt-get install -y build-essential pkg-config cmake
```

## Kurulum

### 1. Derleme
```bash
cd 09.video_frame_extractor
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 2. Debug Build (Gelistirme icin)
```bash
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

## Kullanim

### Temel Kullanim
```bash
# Video dosyasindan her 30 frame'de bir cikarma
./video-frame-extractor -i video.mp4 -o ./frames -m interval -n 30

# RTSP akisindan her 5 saniyede bir cikarma
./video-frame-extractor -i rtsp://192.168.1.100:554/stream -m time_based -t 5.0

# Konfigurasyon dosyasi ile calistirma
./video-frame-extractor -c ../config/config.yaml
```

### Komut Satiri Parametreleri
```
Kullanim: video-frame-extractor [secenekler]

Secenekler:
  -i, --input <path>      Girdi video dosyasi veya URI (zorunlu)
  -o, --output <dir>      Cikti dizini (varsayilan: ./frames)
  -c, --config <file>     Konfigurasyon dosyasi (YAML)
  -m, --mode <mode>       Cikarma modu: interval, keyframe, all, time_based
  -n, --interval <num>    'interval' modu icin frame araligi (varsayilan: 30)
  -t, --time <seconds>    'time_based' modu icin zaman araligi (varsayilan: 1.0)
  -f, --format <fmt>      Cikti formati: png, jpeg, bmp (varsayilan: png)
  -q, --quality <num>     JPEG kalitesi 1-100 (varsayilan: 95)
  -x, --max <num>         Maksimum cikarilacak frame sayisi (varsayilan: sinirsiz)
  -p, --prefix <name>     Cikti dosya on eki (varsayilan: frame)
  -r, --resize <WxH>      Ciktiyi yeniden boyutlandir (ornegin: 640x480)
  --timestamp             Frame'lere zaman damgasi ekle
  -v, --verbose           Detayli cikti
  -h, --help              Bu yardim mesajini goster
```

### Ornek Senaryolar

#### 1. Makine Ogrenimi Veri Seti Hazirlama
```bash
# Her 1 saniyede bir 640x480 boyutunda JPEG olarak kaydet
./video-frame-extractor \
    -i training_video.mp4 \
    -o ./dataset/images \
    -m time_based \
    -t 1.0 \
    -f jpeg \
    -q 90 \
    -r 640x480
```

#### 2. Video Ozet Olusturma
```bash
# Ilk 100 anahtar kareyi cikar
./video-frame-extractor \
    -i documentary.mkv \
    -o ./thumbnails \
    -m keyframe \
    -x 100 \
    -f png
```

#### 3. Guvenlik Kamerasi Kaydi
```bash
# RTSP akisindan her 10 saniyede bir frame al
./video-frame-extractor \
    -i rtsp://camera:554/stream \
    -o ./security_frames \
    -m time_based \
    -t 10.0 \
    --timestamp
```

## Konfigurasyon Dosyasi

```yaml
# config.yaml ornegi
input:
  uri: "../data/sample_video.mp4"

output:
  directory: "./frames"
  prefix: "frame"
  format: "png"
  jpeg_quality: 95
  resize:
    enabled: true
    width: 640
    height: 480
  timestamp_overlay: false

extraction:
  mode: "interval"
  interval_frames: 30
  interval_seconds: 1.0
  max_frames: 0
```

## Mimari

### Pipeline Yapisi
```
+------------+     +----------+     +--------------+     +---------+
| Video      | --> | Decoder  | --> | Video        | --> | AppSink |
| Source     |     | (auto)   |     | Convert(BGR) |     |         |
+------------+     +----------+     +--------------+     +---------+
                                                              |
                                                              v
                                                    +------------------+
                                                    | Frame Processing |
                                                    | (OpenCV)         |
                                                    +------------------+
                                                              |
                                                              v
                                                    +------------------+
                                                    | File Output      |
                                                    | (PNG/JPEG/BMP)   |
                                                    +------------------+
```

### Kod Yapisi
```
09.video_frame_extractor/
├── CMakeLists.txt
├── README.md
├── config/
│   └── config.yaml
├── include/
│   └── frame_extractor.h
└── src/
    ├── main.cpp
    └── frame_extractor.cpp
```

## API Kullanimi

Projeyi kutuphane olarak kullanmak icin:

```cpp
#include "frame_extractor.h"

using namespace gst_frame_extractor;

int main() {
    ExtractorConfig config;
    config.input_uri = "video.mp4";
    config.output_dir = "./frames";
    config.mode = ExtractionMode::INTERVAL;
    config.interval_frames = 30;
    config.format = OutputFormat::PNG;

    FrameExtractor extractor;

    // Ozel frame isleme callback'i ekle
    extractor.setFrameCallback([](const cv::Mat& frame, int64_t pts, int frame_num) {
        // Yuz tanima, nesne algilama vb. islemler yapilabilir
    });

    if (!extractor.initialize(config)) {
        return 1;
    }

    extractor.start();
    extractor.waitForCompletion();

    return 0;
}
```

## Sorun Giderme

### Yaygin Hatalar

1. **"Failed to create pipeline" hatasi**
   ```bash
   # GStreamer kurulumunu kontrol edin
   gst-inspect-1.0 --version
   ```

2. **"No video decoder found" hatasi**
   ```bash
   # Gerekli codec'leri yukleyin
   sudo apt-get install gstreamer1.0-plugins-ugly gstreamer1.0-libav
   ```

3. **RTSP baglanti sorunlari**
   ```bash
   # RTSP kaynagini test edin
   gst-launch-1.0 rtspsrc location=rtsp://... ! fakesink
   ```

### Debug Loglari
```bash
# Detayli GStreamer loglari
GST_DEBUG=3 ./video-frame-extractor -i video.mp4

# Sadece appsink loglari
GST_DEBUG=appsink:5 ./video-frame-extractor -i video.mp4
```

## Performans Ipuclari

1. **JPEG kullanin**: PNG'ye gore daha hizli yazilir
2. **SSD kullanin**: Frame yazma hizi onemli oldugundan SSD tercih edin
3. **Uygun aralik secin**: Tum frame'leri cikarmayin, gerekli olani secin
4. **Boyut kucultun**: Resize ozelligi ile disk alani ve islem suresi azaltilabilir

## Lisans

Bu proje MIT lisansi altinda lisanslanmistir.

## Faydali Kaynaklar

- [GStreamer Documentation](https://gstreamer.freedesktop.org/documentation/)
- [GStreamer AppSink Plugin](https://gstreamer.freedesktop.org/documentation/app/appsink.html)
- [OpenCV Documentation](https://docs.opencv.org/)
- [YAML-CPP Documentation](https://github.com/jbeder/yaml-cpp/wiki)
