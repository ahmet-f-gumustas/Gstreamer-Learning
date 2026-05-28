# Jetson Orin Nano 8GB — Referans Kart

Bu sayfa, simulator ve deployment notları için kullandığımız sayıların
kaynağıdır.  NVIDIA Orin Nano Developer Kit datasheet'inden (Rev 1.4, 2024)
ve JetPack 6.0 release notes'tan derlenmiştir.

## SoC

| Özellik | Değer |
|---|---|
| SoC kodadı | Orin (T234) |
| CPU | 6× ARM Cortex-A78AE @ 1.5 GHz |
| GPU | NVIDIA Ampere, 1024 CUDA core + 32 Tensor core |
| AI performansı | 40 TOPS (INT8 sparse) / 20 TOPS (INT8 dense) |
| FP16 performansı | 10 TFLOPS |
| Bellek | 8 GB LPDDR5 @ 102 GB/s teorik, ~68 GB/s sürdürülebilir |
| Depolama | microSD veya NVMe (m.2 key M) |
| Video encode | 1× 1080p30 (H.264) / 1× 1080p60 (H.265 *via SW*) |
| Video decode | 1-2× 4K30 (H.265) |
| ISP | 1.5 Gpix/s, 8 MP @ 30 fps |
| Camera | 4-lane MIPI CSI-2 × 4 (D-PHY 2.1) |

> **Not:** Orin Nano'da NVENC **yok** — H.264 encode tamamen SW (x264) ile yapılır.
> Bu yüzden 1080p60 H.264 hedeflerini düşük tutun veya H.265'e geçin.

## Güç Profilleri (nvpmodel)

```bash
# Mevcut profilleri listele
sudo nvpmodel -q --verbose

# Profili değiştir
sudo nvpmodel -m 0       # 15W (default)
sudo nvpmodel -m 1       # 7W
sudo nvpmodel -m 2       # MAXN (orin nano-de 7W üst sınırı kaldırılmış mod)

# Klokları kilitle (benchmark sırasında kararlılık için)
sudo jetson_clocks
```

| Mod | GPU clock | CPU clock | TDP | INT8 TOPS |
|---|---:|---:|---:|---:|
| 7W   | 408 MHz | 1.19 GHz | 7 W  | 20 |
| 15W  | 624 MHz | 1.51 GHz | 15 W | 40 |
| MAXN | 918 MHz | 1.70 GHz | ~20 W | 50 |

## Yazılım Stack'i

- **JetPack 6.0** (L4T r36.x)
- **CUDA 12.2**, **TensorRT 8.6**, **cuDNN 8.9**
- **GStreamer 1.20** + NVIDIA HW plugins (`nvarguscamerasrc`, `nvvidconv`,
  `nvv4l2decoder`, `nvv4l2h264enc`, `nvjpegdec`, `nvjpegenc`)
- **OpenCV 4.8** (apt'tan; CUDA backend için kaynaktan build gerekir)

## Mimari Notları

### NVMM (NVIDIA Multimedia Memory)

CSI kamera + decoder + encoder + inference arasında **zero-copy**:

```
nvarguscamerasrc ! video/x-raw(memory:NVMM),format=NV12 ! …
```

`(memory:NVMM)` kaldırılırsa pipeline çalışmaya devam eder ama her geçişte
host↔device kopyası yapılır.  Orin Nano'da 1080p NV12 frame ~3 MB, 30 fps'de
saniyede 180 MB kopya = ölçülebilir gecikme + güç.

### DLA (Deep Learning Accelerator)

Orin Nano'da **DLA yok** — Orin NX ve AGX Orin'de var.  Bu projedeki
`--use-dla` opsiyonu sadece Orin NX/AGX'de etkili olur, Nano'da hata vermez
ama sessizce GPU'ya düşer.

## Çevre / I/O

- Tipik fanless çalışma sıcaklığı: **45-65 °C** (1080p YOLOv8n @ 15W)
- Thermal throttling eşiği: **97 °C** (tj)
- Power input: USB-C 5V/3A (15W için yeterli) veya barrel jack 7-20V (MAXN için)
- Önerilen: Cooling fan + heatsink, jumbo USB-C PD adaptör

## Referans Benchmark Verileri

`config/orin_nano_profiles.yaml` içindeki `reference_benchmarks` bloğu
NVIDIA'nın 2024 Q1 blog post'larından alınmıştır.  Simulator çıktısı
bu değerlere ±%15 civarında olmalı — daha fazla sapma varsa
`compute_bound_fraction` ayarını gözden geçir.
