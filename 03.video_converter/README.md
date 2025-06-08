# GStreamer Video Converter

RTX 4070 ve CUDA 12.4 desteği ile basit video dönüştürücü.

## Hızlı Başlangıç

```bash
# 1. Build script'i çalıştırılabilir yapın
chmod +x build.sh

# 2. Bağımlılıkları yükleyin
./build.sh deps

# 3. Projeyi derleyin
./build.sh

# 4. Test edin
./build.sh test
```

## Kullanım

```bash
# MP4'e dönüştür (GPU hızlandırmalı)
./build/bin/video_converter input.mov output.mp4

# WebM'e dönüştür
./build/bin/video_converter input.mp4 output.webm webm

# AVI'ye dönüştür
./build/bin/video_converter input.mkv output.avi avi
```

## Build Komutları

```bash
./build.sh          # Normal build
./build.sh deps     # Bağımlılık kurulumu
./build.sh check    # Sistem kontrolü
./build.sh test     # Build + test
./build.sh clean    # Temizlik
```

## Özellikler

- **CUDA Desteği**: RTX 4070 ile GPU hızlandırma
- **Çoklu Format**: MP4, WebM, AVI
- **Basit Kullanım**: Tek komutla dönüştürme
- **Modern C++17**: CMake build sistemi

## Sistem Gereksinimleri

- Ubuntu 22.04
- CMake 3.16+
- GCC 11.4+
- GStreamer 1.0+
- CUDA 12.4 (isteğe bağlı)

---

**Not**: CUDA bulunamazsa otomatik olarak CPU encoder kullanılır.