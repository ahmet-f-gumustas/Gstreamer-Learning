# GStreamer Learning - Multimedia Framework ile Uygulamalı Ogrenme

<div align="center">
  <img src="./data/Gstreamer-logo.png" alt="GStreamer Logo" width="400"/>
  &nbsp;&nbsp;&nbsp;
  <img src="./data/cpp-logo.png" alt="C++ Logo" width="100"/>
</div>

<div align="center">

**GStreamer multimedia framework ile C/C++ ve Python dillerinde temellerden ileri seviyeye projeler**

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![C](https://img.shields.io/badge/C-GStreamer%20API-green)
![Python](https://img.shields.io/badge/Python-3-yellow)
![GStreamer](https://img.shields.io/badge/GStreamer-1.0+-red)
![License](https://img.shields.io/badge/License-Apache%202.0-orange)

</div>

---

## Hakkinda

Bu repo, GStreamer multimedia framework'unu ogrenme surecinde gelistirdigim projeleri icerir. Basit Python/C orneklerinden baslayarak, stereo derinlik tahmini, GPU-hizlandirilmis nesne tespiti ve Jetson edge deployment'a kadar uzanan 12 proje bulunmaktadir.

> **Not:** [GSt_Note_.md](GSt_Note_.md) dosyasi GStreamer hakkinda **Turkce ogrenme notlari** olarak kullanilmaktadir. Pipeline, element, pad, bus gibi temel kavramlarin aciklamalarini icerir. Proje kaynak kodlari ise asagidaki klasorlerde ayri ayri degerlendirilmelidir.

---

## Projeler

| # | Proje | Aciklama | Dil | Temel Teknolojiler |
|---|-------|----------|-----|--------------------|
| **00** | [First Code](00.first_code/) | GStreamer baslangic - Python ile ilk pipeline | Python | PyGObject, GLib |
| **01** | [Basic Tutorial](01.basic_tutorial/) | 7 adet temel C tutorial (playback, bus, seek, state) | C | GStreamer C API |
| **02** | [Media Player](02.media_player/) | Interaktif komut satiri medya oynatici | C++ | Multithreading, GStreamer C++ wrapper |
| **03** | [Video Converter](03.video_converter/) | GPU destekli video format donusturucu | C++ | CUDA, NVENC, x264, VP8 |
| **04** | [Optical Flow](04.Optical_flow_with_GST/) | Gercek zamanli optik akis ile hareket tespiti | C++ | OpenCV, Lucas-Kanade, Harris Corner |
| **05** | [RTSP Server/Client](05.RTSP_server_client/) | Dusuk gecikmeli RTSP yayin sistemi (<250ms) | C++ | RTSP/RTP, x264 zerolatency |
| **06** | [DeepDetect Plugin](06.Gstreamer_DeepDetect_Plugin_Project/) | YOLOv8 + TensorRT nesne tespit eklentisi | C++ | TensorRT, CUDA, FP16/INT8 |
| **07** | [Video Analytics Pipeline](07.GStreamer_Video_Analytics_Pipeline/) | Moduler video analitik cercevesi | C++ | OpenCV, NVENC/NVDEC, YAML config |
| **08** | [Video Mosaic Creator](08.video_mosaic_creator/) | Coklu kaynak video mozaik birlestirici (2-16 kaynak) | C++ | GStreamer Compositor, YAML-CPP |
| **09** | [Video Frame Extractor](09.video_frame_extractor/) | Akilli kare cikarma araci (interval, keyframe, time-based) | C++ | OpenCV, appsink |
| **10** | [Stereo Depth Pipeline](10.stereo_depth_pipeline/) | Stereo goruntu ile derinlik tahmini ve engel tespiti | C++ | StereoBM/SGBM, OpenCV, V4L2 |
| **11** | [Jetson Edge Pipeline](11.jetson_edge_pipeline/) | Orin Nano edge AI (4 kamera + ByteTrack + INT8 + simulator) | C++ | TensorRT, ByteTrack, Aravis, QEMU cross-compile |

---

## Ogrenme Yol Haritasi

```
Temel                          Orta Seviye                         Ileri Seviye
──────                         ───────────                         ────────────
00. Python Init          ──>   03. Video Converter (GPU)     ──>  06. DeepDetect Plugin (TensorRT)
01. C Tutorials (x7)     ──>   04. Optical Flow (OpenCV)     ──>  07. Video Analytics Framework
02. Media Player (C++)   ──>   05. RTSP Streaming            ──>  08. Video Mosaic (Multi-source)
                               09. Frame Extractor           ──>  10. Stereo Depth (Robotik)
                                                              ──>  11. Jetson Edge (Production)
```

---

## Onkosuller

### Zorunlu
```bash
# GStreamer gelistirme kutuphaneleri
sudo apt install -y \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly \
  gstreamer1.0-tools

# CMake ve derleme araclari
sudo apt install -y cmake build-essential pkg-config
```

### Projeye Gore Ek Bagimliliklar

| Bagimlilik | Kullanildigi Projeler | Kurulum |
|------------|----------------------|---------|
| OpenCV | 04, 07, 09, 10 | `sudo apt install libopencv-dev` |
| YAML-CPP | 07, 08, 09 | `sudo apt install libyaml-cpp-dev` |
| GStreamer RTSP Server | 05, 07 | `sudo apt install libgstrtspserver-1.0-dev` |
| CUDA + TensorRT | 03, 06 | NVIDIA resmi kurulum kilavuzu |
| V4L2 | 04, 10 | `sudo apt install v4l-utils` |

---

## Hizli Baslangic

### Temel Tutorials (Proje 01)
```bash
cd 01.basic_tutorial
gcc basic-tutorial-1.c -o basic-tutorial-1 `pkg-config --cflags --libs gstreamer-1.0`
./basic-tutorial-1
```

### CMake Projelerini Derleme (Proje 02-10)
```bash
cd 02.media_player    # veya herhangi bir CMake projesi
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

---

## Proje Detaylari

### 00 - First Code (Python)
GStreamer'in Python ile baslatilmasi ve GLib MainLoop kurulumu. Framework'un calisma mantigini anlamak icin ilk adim.

### 01 - Basic Tutorial (C)
GStreamer'in resmi tutorial serisine dayanan 7 ornek:
- **Tutorial 1-2:** Pipeline olusturma, playbin ile medya oynatma
- **Tutorial 3-4:** Bus mesajlari, hata yonetimi, seek islemleri
- **Tutorial 5-7:** Caps negotiation, dinamik elementler, oynatma hizi kontrolu

### 02 - Media Player (C++)
Tam ozellikli interaktif medya oynatici:
- Play/Pause/Stop kontrolleri
- Ileri/geri sarma (+-10 saniye)
- Medya bilgisi gosterimi (sure, codec, bitrate)
- Ayri thread'de gercek zamanli pozisyon guncelleme

### 03 - Video Converter (C++)
NVIDIA GPU destekli video format donusturucu (MP4, WebM, AVI). GPU bulunamazsa otomatik olarak CPU encoder'a duser. CUDA 12.4 ve RTX serisi destekler.

### 04 - Optical Flow (C++)
Web kamerasi veya video dosyasindan gercek zamanli hareket tespiti:
- Harris corner detection ile ozellik noktasi bulma
- Lucas-Kanade optik akis algoritmasi
- Hareket vektorlerinin gorsellestirilmesi

### 05 - RTSP Server/Client (C++)
Dusuk gecikmeli (<250ms) RTSP yayin sistemi:
- **Server:** Kamera -> H.264 encode -> RTSP/RTP yayin
- **Client:** Yayin alma, decode, gecikme olcumu ve rapor
- `tune=zerolatency`, `speed-preset=ultrafast` optimizasyonlari

### 06 - DeepDetect Plugin (C++)
Uretim kalitesinde GStreamer eklentisi:
- YOLOv8 modeli ile gercek zamanli nesne tespiti
- TensorRT FP16/INT8 quantized inference
- Zero-copy GPU bellek islemleri
- JSON metadata cikisi
- RTX 4090'da YOLOv8n ile ~245 FPS

### 07 - Video Analytics Pipeline (C++)
Moduler video analitik cercevesi:
- Dosya, webcam, RTSP, HTTP giris kaynaklari
- OpenCV tabanli hareket tespiti
- NVENC/NVDEC GPU hizlandirma
- YAML tabanli pipeline yapilandirmasi
- Calisma zamaninda dinamik pipeline degisikligi

### 08 - Video Mosaic Creator (C++)
2-16 kaynagin tek ekranda birlestirildigi mozaik sistemi:
- Esnek grid yapilari (2x2, 3x3, 4x4, ozel)
- RTSP stream'ler icin otomatik yeniden baglanti
- YAML ile kaynak ve layout yapilandirmasi

### 09 - Video Frame Extractor (C++)
Video'dan akilli kare cikarma:
- **interval:** Her N kare
- **keyframe:** Sadece I-frame'ler
- **time_based:** Zaman aralikli (ornegin her 5 saniye)
- PNG/JPEG/BMP cikis formatlari
- Opsiyonel boyutlandirma ve zaman damgasi

### 10 - Stereo Depth Pipeline (C++)
Robotik uygulamalar icin stereo goruntu sistemi:
- Cift kamera veya simulasyon modu
- StereoBM/StereoSGBM ile disparite hesaplama
- Metrik derinlik haritasi (Z = focal x baseline / disparity)
- 3x4 grid engel tespiti (SAFE/CAUTION/DANGER)
- 4 panelli gercek zamanli goruntuleme
- ROS2 entegrasyonu ornegi

---

## Proje Yapisi

```
Gstreamer-Learning/
├── 00.first_code/                  # Python giris
├── 01.basic_tutorial/              # C temelleri (7 tutorial)
├── 02.media_player/                # C++ medya oynatici
│   ├── include/                    # Header dosyalari
│   ├── src/                        # Kaynak kodlar
│   └── CMakeLists.txt
├── 03.video_converter/             # GPU destekli donusturucu
├── 04.Optical_flow_with_GST/       # Optik akis
├── 05.RTSP_server_client/          # RTSP yayin
├── 06.Gstreamer_DeepDetect_Plugin_Project/  # YOLOv8 eklenti
│   ├── src/
│   ├── include/
│   ├── tests/
│   ├── scripts/
│   └── docs/
├── 07.GStreamer_Video_Analytics_Pipeline/    # Video analitik
├── 08.video_mosaic_creator/        # Mozaik birlestirici
├── 09.video_frame_extractor/       # Kare cikarici
├── 10.stereo_depth_pipeline/       # Stereo derinlik
├── data/                           # Logo ve gorseller
├── GSt_Note_.md                    # GStreamer Turkce ogrenme notlari
├── LICENSE                         # Apache 2.0
└── .gitignore
```

---

## Kullanilan Teknolojiler

- **Multimedia:** GStreamer 1.0, RTSP/RTP, H.264/VP8/MPEG-4
- **Bilgisayarli Goru:** OpenCV (optik akis, stereo matching, hareket tespiti)
- **GPU Hizlandirma:** NVIDIA CUDA 12.4, TensorRT 10.0, NVENC/NVDEC
- **Yapay Zeka:** YOLOv8 (nesne tespiti), FP16/INT8 quantization
- **Build:** CMake, pkg-config, Meson
- **Diller:** C++17, C, Python 3

---

## Lisans

Bu proje [Apache License 2.0](LICENSE) ile lisanslanmistir.
