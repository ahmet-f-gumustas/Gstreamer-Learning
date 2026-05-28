# Orin Nano Simülasyon Metodu

x86 host'ta yapılan ölçümleri Orin Nano performansına ölçeklemek için
basit ama makul doğrulukta bir model kullanıyoruz.  Bu doküman formüllerin
arkasındaki mantığı ve sınırlamalarını açıklar.

## Temel Model

Her inference iş yükü, baskın olarak iki kaynaktan birinde sınırlıdır:

1. **Compute-bound**: Tensor Core / CUDA çekirdek hesaplama gücü → **TOPS**
2. **Memory-bound**: ağırlıkları + activation'ları okumak → **GB/s**

Karışık iş yükleri (YOLOv8 buna iyi bir örnek) ikisinin lineer kombinasyonu
olarak ifade edilebilir:

```
orin_fps = host_fps × (α × TOPS_ratio + (1 − α) × BW_ratio)

TOPS_ratio = orin_int8_tops / host_int8_tops
BW_ratio   = orin_mem_bw_gb / host_mem_bw_gb
α          = compute_bound_fraction  ∈ [0, 1]
```

α ≈ 0.7 YOLOv8 için ampirik olarak iyi uyar (Ultralytics ve NVIDIA blog
post'larındaki referans değerlere bakarak ayarlandı).

## Kalibrasyon Verisi

`scripts/simulate_orin_nano.py` aşağıdaki halka açık hardware sabitlerini
kullanır:

| GPU | INT8 TOPS | FP16 TFLOPS | Bellek BW (GB/s) | TDP (W) |
|---|---:|---:|---:|---:|
| RTX 4090 | 1321 | 165 | 1008 | 450 |
| RTX 4080 | 780 | 97 | 717 | 320 |
| RTX 4070 | 466 | 58 | 504 | 200 |
| RTX 3090 | 568 | 71 | 936 | 350 |
| RTX 3080 | 476 | 59 | 760 | 320 |
| RTX 3070 | 326 | 40 | 448 | 220 |
| A100 SXM4 | 624 | 156 | 1555 | 400 |
| **Orin Nano 15W** | **40** | **10** | **68** | **15** |

NOT: NVIDIA'nın yayınladığı "sparse" TOPS değerleri yarıya bölünür ("dense")
çünkü gerçek dünyada YOLOv8 modelleri %50 sparse değil.

## Validation

Modelin doğruluğunu test etmek için aynı yapılandırmayı (YOLOv8n FP16 @ 640²)
hem RTX 4090'da hem Jetson AGX Orin Dev Kit'te ölçtüm (proje 06 verisi
kullanıldı, AGX Orin 275 TOPS @ 60W):

| Metrik | RTX 4090 ölçülen | Modelin tahmini AGX | Gerçek AGX |
|---|---:|---:|---:|
| FPS | 245 | 56 | 51 (-9%) |
| Latency (ms) | 4.1 | 17.9 | 19.6 (+10%) |

±%15 sapma kabul edilebilir — public benchmark'lar arasındaki tipik fark
zaten ±%10.  Daha hassas tahmin için `compute_bound_fraction`'ı modelin
roofline analizine göre ayarlamak gerekir (örn. YOLOv8x için 0.85'e yakın
çünkü daha derin = daha çok compute).

## Güç Tahmini

Güç ile ütilizasyon arasındaki ilişki lineer değil — Jetson'da %25-30 idle
power baseline'ı var, sonra ütilizasyonla beraber TDP'ye yaklaşır:

```
power_W = idle_power + (TDP − idle_power) × utilization
idle_power ≈ TDP × 0.25
```

Bu Jetson AGX Orin ölçümlerinden çıkarıldı; Orin Nano için aynı eğri
geçerli kabul ediliyor.

## Sıcaklık Tahmini

Sıcaklığı doğru simüle etmek için thermal RC modeli gerekir — bunun yerine
basit bir lineer yaklaşım kullanıyoruz:

```
soc_temp_c = 38 + max(0, power_W − idle_baseline_W) × 1.5
```

Bu **kararlı durum** tahminidir; soğuk başlangıçtan tepe sıcaklığa ulaşma
süresi (~60-90 s, fanless durumda ~3-4 dk) modellenmez.

## Modelin Geçerli Olmadığı Durumlar

❌ **Çok küçük modeller (<10M parametre)**: Launch overhead'i baskın olur,
ratio çok pesimistik tahmin verir.  Bu durumda α=0.4 deneyin.

❌ **Çok büyük modeller (LLM, SAM gibi)**: Memory bandwidth dominant olur
ve aynı zamanda PCIe / NVLink karakteristiği de devreye girer (Jetson'da yok).
Bu modeller için ayrı bir scaling tablosu gerekir.

❌ **Multi-stream / batch > 1**: Jetson'da memory kapasitesi limit olur
(8 GB), x86'da olmaz.  Batch 4+ için empirik olarak ekstra %20-30 yavaşlama
beklenmeli.

❌ **DLA (Orin NX/AGX)**: DLA pathway GPU'dan ayrı characteristic gösterir.
Orin Nano'da DLA yok — bu problem değil.

## Sonuç

Bu simülasyon, **gerçek bir Jetson aldığınızda kodunuzun çalışacağını
doğrulamak için yeterlidir**.  Donanım gelmeden:

- ✅ Pipeline darboğazlarını tespit edebilirsin
- ✅ FPS/W trade-off'larını gözlemleyebilirsin
- ✅ Hangi precision'ın yeterli olacağını tahmin edebilirsin
- ❌ Ama termal davranışı, NVMM zero-copy avantajını veya DLA boost'unu
  ölçemezsin
