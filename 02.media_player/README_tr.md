## GStreamer C++ Media Player

Bu proje, GStreamer framework'ünü kullanarak basit bir medya oynatıcı uygulaması oluşturacaktır. Aşağıdaki özellikleri içerecek:

1. Video ve ses dosyalarını oynatabilme
2. Duraklat/Devam et kontrolü
3. İleri/geri atlama özelliği
4. Dosya seçme yeteneği
5. Medya bilgilerini gösterme (süre, codec, vb.)

### Proje Yapısı

```
gstreamer-cpp-player/
│
├── CMakeLists.txt          # Ana CMake yapılandırma dosyası
├── include/                # Header dosyaları
│   ├── MediaPlayer.hpp     # MediaPlayer sınıfı tanımı
│   └── Utils.hpp           # Yardımcı fonksiyonlar
│
├── src/                    # Kaynak kodlar
│   ├── main.cpp            # Ana uygulama
│   ├── MediaPlayer.cpp     # MediaPlayer sınıfı implementasyonu
│   └── Utils.cpp           # Yardımcı fonksiyonlar implementasyonu
│
└── README.md               # Proje hakkında bilgi
```

```markdown
# GStreamer C++ Media Player

Bu uygulama, GStreamer multimedia framework'ünü kullanan basit bir C++ medya oynatıcısıdır.

## Özellikler

- Video ve ses dosyalarını oynatma
- Temel oynatıcı kontrolleri (oynat, duraklat, durdur)
- İleri/geri atlama
- Medya bilgilerini gösterme

## Gereksinimler

- C++14 uyumlu derleyici
- CMake 3.10 veya üzeri
- GStreamer 1.0 ve geliştirme paketleri

## Kurulum

### Linux

```bash
# GStreamer geliştirme paketlerini yükleyin
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-good1.0-dev

# Projeyi derleyin
mkdir build && cd build
cmake ..
make

# Çalıştırın
./media-player [dosya_adı]
```

# Çalıştırın
./media-player.exe [dosya_adı]
```

## Kullanım

Program çalıştığında, bir menü gösterecektir:

1. Dosya Aç - Bir medya dosyası açar
2. Oynat - Mevcut dosyayı oynatır
3. Duraklat - Oynatmayı duraklatır
4. Durdur - Oynatmayı durdurur
5. 10 saniye ileri - 10 saniye ileri atlar
6. 10 saniye geri - 10 saniye geri atlar
7. Belirli bir konuma atla - Belirtilen saniyeye atlar
8. Medya bilgilerini göster - Mevcut dosya hakkında bilgi gösterir
9. Konum bilgisini güncelle - Pozisyon bilgisi güncellemesini açar/kapatır
0. Çıkış - Programdan çıkar

## Lisans

Bu proje MIT lisansı altında dağıtılmaktadır.
```

### Derleme ve Çalıştırma

1. Projeyi derlemek için:
```bash
mkdir build && cd build
cmake ..
make
```

2. Programı çalıştırmak için:
```bash
./media-player [dosya_adı]
```

3. İsteğe bağlı olarak, programı yüklemek için:
```bash
sudo make install
```
