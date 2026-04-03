# GStreamer Optical Flow Detector

Bu proje, **GStreamer** ve **OpenCV** kullanarak gerçek zamanlı optik akış (optical flow) algılama yapar. Lucas-Kanade algoritması ile video akışındaki hareket vektörlerini tespit eder ve görselleştirir.

## Özellikler

- 🎥 **Çoklu Video Kaynağı**: Webcam, video dosyası veya test pattern
- 🔍 **Gerçek Zamanlı İşlem**: Düşük gecikme ile hareket algılama
- 🎯 **Akıllı Nokta Seçimi**: Goodfeaturestrotrack ile otomatik köşe tespiti
- 📊 **Görsel Feedback**: Hareket vektörleri ve izlenen noktalar
- 🛠️ **Modüler Tasarım**: Kolay genişletilebilir kod yapısı

## Gereksinimler

### Sistem Gereksinimleri
- Ubuntu 22.04+ veya benzer Linux dağıtımı
- CMake 3.16+
- GCC 11+ (C++17 desteği)

### Kütüphaneler
```bash
# GStreamer kütüphaneleri
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt-get install libgstreamer-plugins-good1.0-dev libgstreamer-plugins-bad1.0-dev
sudo apt-get install gstreamer1.0-plugins-good gstreamer1.0-plugins-bad
sudo apt-get install gstreamer1.0-libav gstreamer1.0-tools

# OpenCV
sudo apt-get install libopencv-dev

# V4L2 (webcam desteği)
sudo apt-get install v4l-utils

# Build araçları
sudo apt-get install cmake build-essential pkg-config
```

## Kurulum

1. **Proje Dizini Oluştur**
```bash
cd ~/git-projects/Gstreamer-Learning
mkdir 04.optical_flow
cd 04.optical_flow
```

2. **Proje Yapısını Oluştur**
```bash
mkdir -p include src build
```

3. **Dosyaları Yerleştir**
- `CMakeLists.txt` → ana dizine
- `OpticalFlowDetector.hpp`, `Utils.hpp` → `include/` dizinine
- `OpticalFlowDetector.cpp`, `Utils.cpp`, `main.cpp` → `src/` dizinine

4. **Build İşlemi**
```bash
cd build
cmake ..
make -j$(nproc)
```

## Kullanım

### Temel Kullanım
```bash
# Webcam ile çalıştır (varsayılan)
./optical-flow-detector

# Test pattern ile çalıştır
./optical-flow-detector --test

# Video dosyası ile çalıştır
./optical-flow-detector --file ../data/test_video.mp4
```

### Komut Satırı Seçenekleri
```bash
./optical-flow-detector [seçenekler]

Seçenekler:
  -h, --help          Yardım mesajını göster
  -f, --file <path>   Video dosyası kullan
  -w, --webcam        Webcam kullan (varsayılan)
  -t, --test          Test pattern kullan
```

### Kontroller
- **'q' tuşu**: Programdan çık
- **Ctrl+C**: Güvenli şekilde sonlandır

## Algoritma Detayları

### Optical Flow Nedir?
Optical Flow, video sekansındaki ardışık frameler arasında nesne hareket vektörlerini hesaplayan bir bilgisayar görü tekniğidir.

### Kullanılan Yöntem: Lucas-Kanade
- **Sparse Optical Flow**: Sadece seçili noktalarda hareket hesaplar
- **Pyramid Implementation**: Çoklu çözünürlük için hızlı hesaplama
- **Feature Tracking**: Good features to track ile otomatik nokta seçimi

### İşlem Adımları
1. **Frame Yakalama**: GStreamer ile video akışı
2. **Gri Tonlama**: RGB → Grayscale dönüşümü
3. **Köşe Tespiti**: Harris corner detection
4. **Optical Flow**: Lucas-Kanade algoritması
5. **Görselleştirme**: Hareket vektörleri ve noktalar

## Proje Yapısı

```
04.optical_flow/
├── CMakeLists.txt              # Build konfigürasyonu
├── README.md                   # Proje dokümantasyonu
├── include/
│   ├── OpticalFlowDetector.hpp # Ana sınıf header
│   └── Utils.hpp               # Yardımcı fonksiyonlar
├── src/
│   ├── main.cpp                # Ana program
│   ├── OpticalFlowDetector.cpp # Optical flow implementasyonu
│   └── Utils.cpp               # Utility implementasyonu
├── build/                      # Build dosyaları
└── test_videos/               # Test video dosyaları (isteğe bağlı)
```

## Teknik Detaylar

### GStreamer Pipeline
```
Webcam Modu:
v4l2src → videoconvert → appsink

Test Modu:
videotestsrc → videoconvert → appsink

Dosya Modu:
filesrc → qtdemux → avdec_h264 → videoconvert → appsink
```

### OpenCV İşlem Zinciri
1. **goodFeaturesToTrack()**: Köşe noktalarını tespit et
2. **calcOpticalFlowPyrLK()**: Lucas-Kanade optical flow
3. **arrowedLine()**: Hareket vektörlerini çiz
4. **circle()**: İzlenen noktaları göster

### Parametreler
```cpp
maxCorners = 100;        // Maksimum izlenecek nokta sayısı
qualityLevel = 0.01;     // Köşe kalite eşiği
minDistance = 10.0;      // Noktalar arası minimum mesafe
```

## Performans İpuçları

### CPU Optimizasyonu
- Frame boyutunu düşürün (640x480 önerilen)
- maxCorners değerini azaltın
- ROI (Region of Interest) kullanın

### Bellek Optimizasyonu
- AppSink buffer sayısını sınırlandırın
- Mat clone() işlemlerini minimize edin
- Gereksiz format dönüşümlerinden kaçının

## Sorun Giderme

### Webcam Bulunamıyor
```bash
# Mevcut video cihazları kontrol et
ls /dev/video*
v4l2-ctl --list-devices

# Webcam testi
gst-launch-1.0 v4l2src ! videoconvert ! xvimagesink
```

### Build Hataları
```bash
# GStreamer paketlerini kontrol et
pkg-config --cflags --libs gstreamer-1.0

# OpenCV kurulumunu kontrol et
pkg-config --modversion opencv4
```

### Permission Denied
```bash
# Video cihaz izinlerini kontrol et
ls -l /dev/video0
sudo usermod -a -G video $USER
# Logout/login gerekli
```

## Gelişmiş Özellikler

### Özelleştirme Seçenekleri
- Farklı optical flow algoritmaları (Farneback, etc.)
- ROI tabanlı işlem
- Çoklu nesne takibi
- Hareket analizi ve istatistikler

### Genişletme Fikirleri
- Nesne tespiti entegrasyonu
- Hareket tabanlı alarm sistemi
- Video stabilizasyonu
- Background subtraction

## Katkıda Bulunma

1. Fork yapın
2. Feature branch oluşturun (`git checkout -b feature/amazing-feature`)
3. Commit yapın (`git commit -m 'Add amazing feature'`)
4. Push yapın (`git push origin feature/amazing-feature`)
5. Pull Request açın

## Lisans

Bu proje MIT lisansı altında dağıtılmaktadır. Detaylar için `LICENSE` dosyasına bakın.

## İletişim

Sorularınız için GitHub issues kullanabilirsiniz.

---

**Happy Coding! 🚀**