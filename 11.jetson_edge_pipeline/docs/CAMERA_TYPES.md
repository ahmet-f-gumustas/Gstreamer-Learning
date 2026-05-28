# Kamera Backend Seçimi

| Kriter | USB (UVC) | CSI | GMSL | GigE Vision |
|---|---|---|---|---|
| **Çalışma mesafesi** | <5 m (kablo) | <30 cm (FFC) | <15 m (koax) | Network kadar |
| **Bant genişliği** | USB 3.x: 5 Gbps | CSI-2: ~10 Gbps/lane × 4 | GMSL2: 6 Gbps | GigE: 1 Gbps |
| **Latency** | 20-50 ms | <5 ms (zero-copy) | <10 ms | 30-80 ms |
| **CPU overhead** | Orta | Düşük | Düşük | Düşük (Aravis DMA) |
| **Sensör eko-sistemi** | Çok geniş | NVIDIA ortakları | Otomotiv + endüstriyel | Endüstriyel |
| **Maliyet** | $30-200 | $25-150 | $200-1500 | $400-5000 |
| **Sürücü zorluğu** | "Bağla çalışsın" | Device-tree config gerek | Vendor kernel modülü | Aravis IP config |
| **Sync (multi-cam)** | Yok (yazılım) | HW sync (FSIN) | HW sync + frame ID | PTP (IEEE 1588) |

## Hangi Senaryoda Hangisi

### USB / UVC
- **Prototyping**: 5 dakikada çalışan ilk pipeline
- **Düşük ürün hacmi**: tüketici ürünleri
- **Webcam**: 1080p30 kompakt çözüm
- **Sınır**: HW sync yok, kablo uzunluğu kısıtlı, kalite değişken

### CSI (Camera Serial Interface)
- **Jetson native** entegrasyon
- **Düşük gecikme**: NVMM zero-copy yolu
- **ISP erişimi**: HDR, denoise, AE/AWB lock
- **Sınır**: FFC kablo kısa (<30 cm), sensör driver'ı Jetson'a özgü

### GMSL (Gigabit Multimedia Serial Link)
- **Otomotiv**: ADAS, sürüş asistanları
- **Uzun mesafe**: 15 m koak kablo
- **Mekanik dayanıklılık**: locking konnektör
- **Sınır**: vendor-locked sürücü, BOM maliyeti yüksek
- **Önemli notlar**:
  - Deserializer (`MAX9296`, `MAX96712`) Jetson'da device-tree overlay ile
    enable edilmeli
  - Bazı GMSL kameralar Bayer raw verir → Jetson ISP gerekir (CSI-equivalent path)
  - Diğerleri YUV422 verir → doğrudan v4l2src ile okunabilir
  - Üretici tarafından sağlanan custom GStreamer plugin'leri varsa onları
    tercih edin (örn. Leopard `nvargusgmsl`)

### GigE Vision
- **Endüstriyel inspection**: line scan, machine vision
- **Uzun mesafe**: switch'ler arasında yüzlerce metre
- **Multi-vendor**: GenICam standardı
- **Sınır**: jumbo frame (MTU 9000) ve PoE switch önerilir; aksi takdirde
  packet drop ve resync overhead'i belirginleşir

## Sorun Giderme

### USB kamera 1080p30 vermez
UVC kameranın çoğu 1080p30'u sadece MJPG ile destekler:

```bash
v4l2-ctl --list-formats-ext -d /dev/video0
# YUY2: 1920x1080@5fps, 1280x720@10fps
# MJPG: 1920x1080@30fps  ← bunu kullan
```

config'te `format: MJPG` yap; pipeline `jpegdec` ekleyecektir.

### CSI sensor-id geçersiz hatası
`/etc/nvidia/nvargus-daemon` log'larını kontrol edin:
```bash
sudo journalctl -u nvargus-daemon -f
```
Genelde yanlış device-tree overlay seçilmiştir.

### GMSL "no video" — deserializer çıktısı boş
```bash
# Modüller yüklü mü?
lsmod | grep -E 'max9296|max96712|max9286'

# Linkler eğitildi mi?
dmesg | grep -i 'link.*lock'
```

### GigE timeout
- Switch jumbo frame destekliyor mu? (`ifconfig eth0 mtu 9000`)
- Kameranın IP'si subnet'inizde mi? `arv-tool-0.8` ile kontrol
- Multicast routing kapalı mı? (sorunsuz başlangıç için unicast yapılandırın)
