# ByteTrack — İmplementasyon Notları

ByteTrack ([Zhang et al., 2021](https://arxiv.org/abs/2110.06864)) detection
confidence dağılımındaki bilgiyi atmamayı öneren bir tracker. Standart SORT/IoU
algoritmaları, eşik altı kalan düşük güvenli kutuları doğrudan atar; ByteTrack
bunları **ikinci asosyasyon turunda** mevcut track'ler için kurtarmaya çalışır.

## Bizim Implementasyon

`include/tracking/byte_tracker.h` üç sınıfa bölünmüş:

1. **`KalmanFilter`** — sabit hızlı (cx, cy, a, h, ẋ, ẏ, ȧ, ḣ) durum modeli.
   SORT'tan birebir alınmış ölçek faktörleri (`std_weight_pos = 1/20`).
2. **`Hungarian`** — Jonker-Volgenant rectangular assignment, INF maliyeti
   destekler.  Bu projedeki match matrisi ≤ 100×100 olduğu için
   O((n+m)³) yeterli — küçük cost'lara hassas.
3. **`ByteTracker`** — frame başına iki turlu asosyasyon + yaşam döngüsü
   yönetimi (NEW → TRACKED → LOST → REMOVED).

## Algoritma Sözde-kodu

```
on_frame(detections):
    high  = [d for d in detections if d.conf >= τ_high]
    low   = [d for d in detections if τ_low <= d.conf < τ_high]

    # 1. tur — predicted (tracked+lost) ↔ high
    pool = tracked + lost
    for t in pool: t.predict()
    cost = 1 − IoU(pool, high)
    a1   = Hungarian(cost, threshold=τ_match)
    apply(a1) → match'leri TRACKED'a yükselt + Kalman update

    # 2. tur — unmatched(tracked) ↔ low
    remain_t = [t for t in pool if not matched(t)]
    cost2    = 1 − IoU(remain_t, low)
    a2       = Hungarian(cost2, threshold=0.5)
    apply(a2)

    # Unmatched tracked → LOST
    # Unmatched high with conf ≥ τ_new → yeni track id
    # lost timer ≥ track_buffer → REMOVED
```

## Parametre Rehberi

| Parametre | Default | Anlamı |
|---|---:|---|
| `track_high_thresh` | 0.6 | Birinci tur için gereken min güven |
| `track_low_thresh`  | 0.1 | İkinci tur için min güven (altı düşer) |
| `new_track_thresh`  | 0.7 | Yeni id oluşturmak için min güven |
| `match_thresh`      | 0.8 | İlk asosyasyon için max IoU mesafesi (= 1 − IoU) |
| `track_buffer`      | 30  | Lost → Removed olmadan tutulacak frame sayısı |

### Yüksek hızlı sahneler (araçlar, spor)

```cpp
ByteTrackConfig cfg;
cfg.match_thresh = 0.9f;   // daha gevşek IoU eşleştirme
cfg.track_buffer = 15;     // kısa hafıza (id swap riskine karşı)
```

### Yoğun trafik (kalabalık)

```cpp
cfg.track_low_thresh = 0.05f;   // düşük güvenli kutuları bile değerlendir
cfg.new_track_thresh = 0.85f;   // false positive yeni id'yi azalt
```

## Diğer Tracker'larla Karşılaştırma

| Tracker | Re-ID | Hız | ID switch (MOT17) | Implementasyon karmaşıklığı |
|---|---|---|---:|---|
| **IoU-tracker** | yok | ⚡ çok hızlı | yüksek | trivial |
| **SORT** | yok | hızlı | orta | Kalman + Hungarian |
| **DeepSORT** | CNN embedding | yavaş | düşük | + Re-ID network (~10 MB) |
| **ByteTrack** | yok | hızlı | düşük | + ikinci asosyasyon turu |
| **BoT-SORT** | opsiyonel | orta | en düşük | + camera motion + GMC |
| **NvDCF** (DeepStream) | embedding | hızlı (GPU) | düşük | DeepStream stack |

ByteTrack, Re-ID network olmadan **DeepSORT seviyesinde** ID switch oranı verir
→ bizim için sweet spot.

## Edge Case'ler

### Kamera ani pan/tilt
Tüm box'lar bir anda atlar; IoU eşik altı kalır.  Çözüm: `match_thresh` artır
veya BoT-SORT'taki Global Motion Compensation'ı ekle.

### Tam oklüzyon (>1 saniye)
`track_buffer` artırılmazsa track ölür ve geri geldiğinde yeni id alır.
`track_buffer = 60` ile 2 saniyelik oklüzyon kurtarılır.

### Çok hızlı nesne (motorlu araç highway'de)
Predict adımı bir sonraki frame'i pas geçer → IoU sıfırlanır.  Çözüm:
yüksek frame rate kamerası (60 fps+) veya constant-velocity yerine
constant-acceleration Kalman modeli.

## Performance

Bizim impl tek thread'de:

- 100 detection / frame: **~0.6 ms** (Ryzen 7 7700X)
- 500 detection / frame: **~4 ms**

Bu, YOLOv8n inference'inin yanında ihmal edilebilir.  Eğer 1000+ detection/frame
sahneleriniz varsa Hungarian'ı SciPy'nin `linear_sum_assignment` C
implementasyonu ile değiştirin (aynı algoritma, vektorize).
