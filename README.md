# Gstreamer-Learning
I am currently learning Gstreamer with Cpp &amp; Python

 - run code => gcc basic-tutorial-1.c -o basic-tutorial-1 `pkg-config --cflags --libs gstreamer-1.0`




# GStreamer C/C++ Öğreticisi - Türkçe

<img src="https://gstreamer.freedesktop.org/assets/header-logo-top.png" alt="GStreamer Logo" width="400"/>

## İçindekiler

- [1. Giriş](#1-giriş)
  - [1.1. GStreamer Nedir? Neden Kullanılır?](#11-gstreamer-nedir-neden-kullanılır)
  - [1.2. Medya Frameworklerine Genel Bakış](#12-medya-frameworklerine-genel-bakış)
  - [1.3. GStreamer'ın Temel Felsefesi: Pipeline Yaklaşımı](#13-gstreamerin-temel-felsefesi-pipeline-yaklaşımı)
- [2. Kurulum](#2-kurulum)
  - [2.1. Linux Sistemlerde Kurulum](#21-linux-sistemlerde-kurulum)
  - [2.2. Windows Sistemlerde Kurulum](#22-windows-sistemlerde-kurulum)
  - [2.3. macOS Sistemlerde Kurulum](#23-macos-sistemlerde-kurulum)
- [3. Temel Kavramlar](#3-temel-kavramlar)
  - [3.1. Elementler (Elements)](#31-elementler-elements)
  - [3.2. Pad'ler (Pads)](#32-padler-pads)
  - [3.3. Pipeline (Boru Hattı)](#33-pipeline-boru-hattı)
  - [3.4. Bin'ler (Bins)](#34-binler-bins)
  - [3.5. Bus (Veri Yolu)](#35-bus-veri-yolu)
- [4. Basit Pipeline Oluşturma](#4-basit-pipeline-oluşturma)
  - [4.1. GStreamer Kütüphanesini Başlatma](#41-gstreamer-kütüphanesini-başlatma)
  - [4.2. Element Oluşturma](#42-element-oluşturma)
  - [4.3. Elementleri Pipeline'a Ekleme](#43-elementleri-pipelinea-ekleme)
  - [4.4. Elementleri Bağlama](#44-elementleri-bağlama)
  - [4.5. Örnek: En Basit Oynatma Uygulaması](#45-örnek-en-basit-oynatma-uygulaması)
- [5. Pipeline Durumları (States)](#5-pipeline-durumları-states)
  - [5.1. Durum Türleri](#51-durum-türleri)
  - [5.2. Durum Değiştirme](#52-durum-değiştirme)
  - [5.3. Örnek: Durum Değişikliklerini Yönetme](#53-örnek-durum-değişikliklerini-yönetme)
- [6. Bus ve Mesajlar](#6-bus-ve-mesajlar)
  - [6.1. Bus'a Erişim](#61-busa-erişim)
  - [6.2. Mesajları Çekme](#62-mesajları-çekme)
  - [6.3. Mesaj Türleri](#63-mesaj-türleri)
  - [6.4. Örnek: Bus Mesajlarını Dinleme ve İşleme](#64-örnek-bus-mesajlarını-dinleme-ve-işleme)
- [7. Daha Karmaşık Pipeline'lar](#7-daha-karmaşık-pipelinelar)
  - [7.1. Farklı Kaynaklar](#71-farklı-kaynaklar)
  - [7.2. Demuxer'lar](#72-demuxerlar)
  - [7.3. Dekoder'lar](#73-dekoderlar)
  - [7.4. Çözücüler (Converter'lar)](#74-çözücüler-converterlar)
  - [7.5. Çıkışlar (Sink'ler)](#75-çıkışlar-sinkler)
  - [7.6. Örnek: Video Dosyası Oynatma](#76-örnek-video-dosyası-oynatma)
  - [7.7. Örnek: Ses Dosyası Oynatma](#77-örnek-ses-dosyası-oynatma)
- [8. Hata Yönetimi](#8-hata-yönetimi)
  - [8.1. Dönüş Değerlerini Kontrol Etme](#81-dönüş-değerlerini-kontrol-etme)
  - [8.2. Bus Üzerinden Hata Mesajlarını Yakalama](#82-bus-üzerinden-hata-mesajlarını-yakalama)
  - [8.3. Örnek: Sağlam Hata Kontrolü](#83-örnek-sağlam-hata-kontrolü)
- [9. Etkinlikler (Events) ve Sorgular (Queries)](#9-etkinlikler-events-ve-sorgular-queries)
  - [9.1. Etkinliklerin Rolü](#91-etkinliklerin-rolü)
  - [9.2. Sorguların Rolü](#92-sorguların-rolü)
  - [9.3. Örnek: Basit Bir Seek Uygulaması](#93-örnek-basit-bir-seek-uygulaması)
- [10. Pratik Örnekler ve İpuçları](#10-pratik-örnekler-ve-ipuçları)
  - [10.1. Command-line Araçları ile Pipeline Test Etme](#101-command-line-araçları-ile-pipeline-test-etme)
  - [10.2. Kullanışlı Elementler](#102-kullanışlı-elementler)
  - [10.3. Multithreading ve GStreamer](#103-multithreading-ve-gstreamer)
- [11. Sonraki Adımlar ve Kaynaklar](#11-sonraki-adımlar-ve-kaynaklar)
  - [11.1. GStreamer Dokümantasyonu](#111-gstreamer-dokümantasyonu)
  - [11.2. GStreamer Plugin'leri](#112-gstreamer-pluginleri)
  - [11.3. İleri Konular](#113-ileri-konular)

## 1. Giriş

### 1.1. GStreamer Nedir? Neden Kullanılır?

GStreamer, çoklu ortam (multimedia) uygulamaları geliştirmek için tasarlanmış güçlü ve esnek bir açık kaynak multimedia framework'üdür. Ses, video ve diğer akış verilerini işlemek için bir dizi araç ve kütüphane sunar.

GStreamer'ın kullanım alanları:
- Medya oynatıcılar
- Video/ses düzenleme yazılımları
- Yayın sistemleri (streaming)
- Video konferans uygulamaları
- Çoklu ortam dönüştürücüler
- Medya analiz araçları

GStreamer'ın temel avantajları:
- **Modüler yapı**: Yeni format ve codec'lerin eklenmesi kolaydır
- **Platform bağımsızlığı**: Linux, Windows, macOS, iOS, Android ve diğer platformlarda çalışır
- **Performans odaklı**: C dilinde yazılmış, düşük gecikme süreleri (latency) sağlar
- **Zengin eklenti (plugin) ekosistemi**: Yüzlerce hazır bileşen mevcuttur
- **Esnek lisanslama**: LGPL lisansı ile ticari projelerde de kullanılabilir

### 1.2. Medya Frameworklerine Genel Bakış

Medya geliştirmenin karmaşıklığını düşünürsek, framework kullanmanın önemi hemen ortaya çıkar. Medya framework'leri, geliştiricilere düşük seviyeli medya işlemleri için soyutlamalar sunar.

Yaygın medya framework'leri:
- **GStreamer**: Çapraz platform, genel amaçlı multimedia framework
- **FFmpeg**: Güçlü komut satırı araçları ve kütüphaneleri
- **DirectShow**: Windows için Microsoft'un multimedia framework'ü
- **Media Foundation**: DirectShow'un modern halefi
- **AVFoundation**: Apple'ın iOS ve macOS için çoklu ortam framework'ü

GStreamer, platformlar arası uyumluluğu ve modüler pipeline-tabanlı mimarisi sayesinde bu alternatiflere göre öne çıkmaktadır.

### 1.3. GStreamer'ın Temel Felsefesi: Pipeline Yaklaşımı

GStreamer'ın merkezinde **pipeline** (boru hattı) kavramı yatar. Bu yaklaşım, Unix felsefesindeki "bir iş yap ve onu iyi yap" prensibine dayanır. Her bir medya işlemi, özel bir görev için tasarlanmış küçük, bağımsız bileşenler tarafından gerçekleştirilir.

Bu bileşenler (elementler) bir boru hattı boyunca birbirine bağlanır ve veriler bu boru hattı boyunca akar:

```
[Kaynak] -> [İşleme1] -> [İşleme2] -> ... -> [Çıkış]
```

Örneğin, basit bir video oynatma pipeline'ı şöyle olabilir:

```
[Dosya Kaynağı] -> [Demuxer] -> [Video Decoder] -> [Video Dönüştürücü] -> [Video Gösterici]
```

Bu modüler yaklaşım şu avantajları sağlar:
- **Yeniden kullanılabilirlik**: Aynı bileşenler farklı pipeline'larda kullanılabilir
- **Esneklik**: Bileşenler istenildiği gibi birleştirilebilir
- **Genişletilebilirlik**: Yeni bileşenler kolayca eklenebilir
- **Bakım kolaylığı**: Her bileşen bağımsız olarak bakımı yapılabilir

## 2. Kurulum

### 2.1. Linux Sistemlerde Kurulum

Debian/Ubuntu tabanlı sistemlerde:

```bash
# Geliştirme kütüphaneleri ve araçları
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

Fedora/Red Hat tabanlı sistemlerde:

```bash
sudo dnf install gstreamer1-devel gstreamer1-plugins-base-devel gstreamer1-plugins-good gstreamer1-plugins-good-extras gstreamer1-plugins-bad-free gstreamer1-plugins-bad-free-devel gstreamer1-plugins-bad-free-extras
```

Arch Linux:

```bash
sudo pacman -S gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav
```

### 2.2. Windows Sistemlerde Kurulum

Windows'ta GStreamer kullanmak için iki temel yöntem vardır:

1. **MSYS2/MinGW ile Kullanım**:
   ```bash
   pacman -S mingw-w64-x86_64-gstreamer mingw-w64-x86_64-gst-plugins-base mingw-w64-x86_64-gst-plugins-good mingw-w64-x86_64-gst-plugins-bad mingw-w64-x86_64-gst-plugins-ugly mingw-w64-x86_64-gst-libav
   ```

2. **Resmi Windows İndiricileri**:
   - [GStreamer indirme sayfası](https://gstreamer.freedesktop.org/download/)
   - Hem geliştirme hem de runtime paketlerini indirin
   - İndirdikten sonra, PATH ortam değişkenine GStreamer bin dizinini ekleyin

Visual Studio ile kullanırken pkgconfig desteği için [vcpkg](https://github.com/microsoft/vcpkg) kullanabilirsiniz.

### 2.3. macOS Sistemlerde Kurulum

macOS'ta Homebrew kullanarak kurulum:

```bash
brew install gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly gst-libav
```

## 3. Temel Kavramlar

### 3.1. Elementler (Elements)

**Elementler**, GStreamer'ın temel yapı taşlarıdır. Her element belirli bir görevi yerine getirir ve pipeline içinde bir işlem noktasını temsil eder. 

Element türleri:
- **Source Elements (Kaynak Elementleri)**: Veri üreten elementler (örn: `filesrc`, `videotestsrc`)
- **Filter Elements (Filtre Elementleri)**: Veriyi değiştiren elementler (örn: `videoconvert`, `audioresample`)
- **Sink Elements (Çıkış Elementleri)**: Veriyi tüketen elementler (örn: `autovideosink`, `filesink`)

Elementlerin her biri kendi başına bağımsız bir bileşendir ve kendi içlerinde bir durum (state) yönetimi vardır.

```c
// Bir element oluşturma örneği
GstElement *source = gst_element_factory_make("videotestsrc", "source");
GstElement *sink = gst_element_factory_make("autovideosink", "sink");

// Element özelliklerini ayarlama
g_object_set(G_OBJECT(source), "pattern", 0, NULL);  // Test deseni 0 (SMPTE)
```

### 3.2. Pad'ler (Pads)

**Pad'ler**, elementlerin bağlantı noktalarıdır. Pad'ler sayesinde elementler birbirine bağlanarak veri akışı sağlanır.

Pad türleri:
- **Source Pad (Kaynak Pad)**: Verinin element dışına aktığı nokta
- **Sink Pad (Alıcı Pad)**: Verinin element içine girdiği nokta

Her pad'in belirli bir **capability** (yetenek, kısaca "caps") seti vardır. Bu yetenekler, pad'in hangi veri türlerini işleyebileceğini tanımlar.

```
[Element A] --source pad--> sink pad-->[Element B]
```

```c
// İki elementi pad'ler üzerinden manuel bağlama
GstPad *src_pad = gst_element_get_static_pad(element_a, "src");
GstPad *sink_pad = gst_element_get_static_pad(element_b, "sink");
gst_pad_link(src_pad, sink_pad);
gst_object_unref(src_pad);
gst_object_unref(sink_pad);
```

### 3.3. Pipeline (Boru Hattı)

**Pipeline**, elementleri bir araya getiren ve senkronize eden özel bir konteynerdir. Pipeline'lar şunları sağlar:
- Elementler arasında veri akışının koordinasyonu
- Global saat (clock) yönetimi
- Pipeline durumlarının yönetimi
- Medya pozisyonu ve süresi izleme

Pipeline, aslında özel bir Bin türüdür.

```c
// Pipeline oluşturma
GstElement *pipeline = gst_pipeline_new("test-pipeline");

// Pipeline'a element ekleme
gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);

// Pipeline durumunu değiştirme
gst_element_set_state(pipeline, GST_STATE_PLAYING);
```

### 3.4. Bin'ler (Bins)

**Bin'ler**, elementleri gruplayabilen konteynerlardır. Bir Bin, içindeki elementleri tek bir element gibi yönetir, böylece karmaşık pipeline'ları modüler bir şekilde oluşturabilirsiniz.

Bin'lerin faydaları:
- Karmaşıklığı yönetme
- Yeniden kullanılabilir element grupları oluşturma
- Hiyerarşik pipeline'lar oluşturma

```c
// Bir bin oluşturma
GstElement *bin = gst_bin_new("my-bin");

// Bin'e elementler ekleme
gst_bin_add_many(GST_BIN(bin), decoder, converter, NULL);

// Pipeline'a bin'i ekleme
gst_bin_add(GST_BIN(pipeline), bin);
```

### 3.5. Bus (Veri Yolu)

**Bus**, pipeline içindeki elementlerden gelen asenkron mesajların iletildiği bir iletişim sistemidir. Bus, elementlerden gelen mesajları uygulama ana döngüsüne taşır.

Bus üzerinden alınabilecek mesaj türleri:
- Hata mesajları
- End-of-Stream (EOS) mesajları
- Durum değişikliği mesajları
- Tag mesajları (metadata)
- Uyarı mesajları

```c
// Pipeline'ın bus'ını alma
GstBus *bus = gst_element_get_bus(pipeline);

// Mesajları almak için bloklamadan bekleme (polling)
GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
    GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);

// Mesaj işleme
// ...

// Temizlik
gst_message_unref(msg);
gst_object_unref(bus);
```

## 4. Basit Pipeline Oluşturma

### 4.1. GStreamer Kütüphanesini Başlatma

GStreamer uygulaması yazmadan önce, kütüphaneyi başlatmak gerekir. Bu, komut satırı argümanlarını işleme, plugin'leri yükleme ve GStreamer'ı kullanıma hazır hale getirme işlemlerini yapar.

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    // GStreamer'ı başlat
    gst_init(&argc, &argv);
    
    // GStreamer kodları...
    
    // Programdan çıkarken (opsiyonel)
    gst_deinit();
    
    return 0;
}
```

### 4.2. Element Oluşturma

Elementler, `gst_element_factory_make()` fonksiyonu kullanılarak oluşturulur. Bu fonksiyon iki parametre alır:
1. Element fabrikasının adı (hangi element türü)
2. Elemente verilecek benzersiz ad (NULL olabilir)

```c
// Bir kaynak element oluşturma
GstElement *source = gst_element_factory_make("videotestsrc", "test-source");
if (!source) {
    g_printerr("Videotestsrc elementi oluşturulamadı\n");
    return -1;
}

// Bir çıkış elementi oluşturma
GstElement *sink = gst_element_factory_make("autovideosink", "test-sink");
if (!sink) {
    g_printerr("Autovideosink elementi oluşturulamadı\n");
    return -1;
}
```

### 4.3. Elementleri Pipeline'a Ekleme

Oluşturulan elementleri pipeline'a eklemek için `gst_bin_add()` veya `gst_bin_add_many()` fonksiyonları kullanılır.

```c
// Pipeline oluşturma
GstElement *pipeline = gst_pipeline_new("test-pipeline");

// Pipeline'a tek element ekleme
gst_bin_add(GST_BIN(pipeline), source);

// Pipeline'a birden fazla element ekleme
gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);
```

### 4.4. Elementleri Bağlama

Elementleri birbirine bağlamak için `gst_element_link()` veya `gst_element_link_many()` fonksiyonları kullanılır.

```c
// İki elementi bağlama
if (!gst_element_link(source, sink)) {
    g_printerr("Elementler bağlanamadı\n");
    gst_object_unref(pipeline);
    return -1;
}

// Birden fazla elementi sırayla bağlama
if (!gst_element_link_many(source, filter1, filter2, sink, NULL)) {
    g_printerr("Elementler bağlanamadı\n");
    gst_object_unref(pipeline);
    return -1;
}
```

### 4.5. Örnek: En Basit Oynatma Uygulaması

Aşağıda, test video deseni üreten ve ekranda gösteren basit bir GStreamer uygulaması bulunmaktadır:

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    
    /* GStreamer'ı başlat */
    gst_init(&argc, &argv);
    
    /* Elementleri oluştur */
    source = gst_element_factory_make("videotestsrc", "source");
    sink = gst_element_factory_make("autovideosink", "sink");
    
    /* Elementlerin başarıyla oluşturulduğunu kontrol et */
    if (!source || !sink) {
        g_printerr("Elementlerden biri oluşturulamadı. Gstreamer eklentilerinin doğru kurulduğundan emin olun.\n");
        return -1;
    }
    
    /* Bir property ayarlama örneği */
    g_object_set(G_OBJECT(source), "pattern", 0, NULL);  // SMPTE test deseni
    
    /* Boş bir pipeline oluştur */
    pipeline = gst_pipeline_new("test-pipeline");
    
    /* Pipeline'ın başarıyla oluşturulduğunu kontrol et */
    if (!pipeline) {
        g_printerr("Pipeline oluşturulamadı.\n");
        return -1;
    }
    
    /* Pipeline'a elementleri ekle */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    
    /* Elementleri bağla */
    if (!gst_element_link(source, sink)) {
        g_printerr("Elementler bağlanamadı.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Pipeline durumunu PLAYING olarak ayarla */
    g_print("Pipeline başlatılıyor...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline PLAYING durumuna getirilemedi.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Pipeline'ın bus'ını al */
    bus = gst_element_get_bus(pipeline);
    
    /* ERROR veya EOS mesajları için bekle */
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    /* Mesaj analizi ve uygun işlemi gerçekleştirme */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Hata alındı: %s\n", err->message);
                g_printerr("Hata ayıklama bilgisi: %s\n", debug_info ? debug_info : "Yok");
                g_free(debug_info);
                g_error_free(err);
                break;
            case GST_MESSAGE_EOS:
                g_print("End-Of-Stream alındı.\n");
                break;
            default:
                g_printerr("Beklenmeyen mesaj alındı.\n");
                break;
        }
        gst_message_unref(msg);
    }
    
    /* Pipeline'ı durdur */
    g_print("Pipeline durduruluyor...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Kaynakları temizle */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    return 0;
}
```

Derleme:

```bash
gcc -o basic-player basic-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

## 5. Pipeline Durumları (States)

### 5.1. Durum Türleri

GStreamer pipeline'ları ve elementleri dört temel durumda bulunabilir:

1. **GST_STATE_NULL**: Varsayılan başlangıç durumu. Hiçbir kaynak ayrılmamış, element hazırlanmamış durumdadır.
2. **GST_STATE_READY**: Element başlatılmış ancak veriler akmıyor. Kaynaklar ayrılmış fakat tampon bellek ayrılmamış olabilir.
3. **GST_STATE_PAUSED**: Element başlatılmış, veriler işleniyor fakat akış durdurulmuş (en fazla bir tampon işlenmiş) durumdadır.
4. **GST_STATE_PLAYING**: Element tamamen aktif, veriler normal hızda akıyor.

Durumlar arasındaki geçişler aşağıdaki gibi olmalıdır:
```
NULL <-> READY <-> PAUSED <-> PLAYING
```

Her durum değişikliği, belirli hazırlık işlemlerini ve kaynak ayırma/serbest bırakma işlemlerini tetikler.

### 5.2. Durum Değiştirme

Pipeline veya element durumunu değiştirmek için `gst_element_set_state()` fonksiyonu kullanılır:

```c
GstStateChangeReturn ret;

// Pipeline'ı PLAYING durumuna getir
ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);

// Durum değişikliği sonucunu kontrol et
switch (ret) {
    case GST_STATE_CHANGE_FAILURE:
        g_printerr("Durum değiştirme başarısız oldu!\n");
        // Hata işleme...
        break;
    case GST_STATE_CHANGE_SUCCESS:
        g_print("Durum başarıyla değiştirildi\n");
        break;
    case GST_STATE_CHANGE_ASYNC:
        g_print("Durum değiştirme asenkron olarak devam ediyor\n");
        // İstenirse durum değişikliğinin tamamlanmasını bekleyebiliriz
        ret = gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
        break;
    case GST_STATE_CHANGE_NO_PREROLL:
        g_print("Canlı kaynak, preroll gerçekleşmeyecek\n");
        break;
}
```

### 5.3. Örnek: Durum Değişikliklerini Yönetme

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *sink;
    GstStateChangeReturn ret;
    
    /* GStreamer'ı başlat */
    gst_init(&argc, &argv);
    
    /* Elementleri oluştur */
    source = gst_element_factory_make("videotestsrc", "source");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("test-pipeline");
    
    /* Kontroller ve pipeline kurulumu... */
    gst_bin_add_many(GST_BIN(pipeline), source, sink, NULL);
    gst_element_link(source, sink);
    
    /* READY durumuna geçiş */
    g_print("Pipeline READY durumuna getiriliyor...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_READY);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline READY durumuna getirilemedi.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* Pipeline'ın mevcut durumunu al */
    GstState current, pending;
    ret = gst_element_get_state(pipeline, &current, &pending, GST_CLOCK_TIME_NONE);
    g_print("Mevcut durum: %s, Bekleyen durum: %s\n", 
            gst_element_state_get_name(current), 
            gst_element_state_get_name(pending));
    
    /* PAUSED durumuna geçiş */
    g_print("Pipeline PAUSED durumuna getiriliyor...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline PAUSED durumuna getirilemedi.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* PAUSED durumuna geçişin tamamlanmasını bekle */
    ret = gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
    g_print("PAUSED durumuna geçiş tamamlandı\n");
    
    /* PLAYING durumuna geçiş */
    g_print("Pipeline PLAYING durumuna getiriliyor...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline PLAYING durumuna getirilemedi.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* 5 saniye çalışmasını bekle */
    g_print("5 saniye oynatılıyor...\n");
    g_usleep(5 * 1000000);
    
    /* PAUSED durumuna geçiş (duraklat) */
    g_print("Pipeline duraklatılıyor (PAUSED durumu)...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    
    /* 2 saniye bekle */
    g_usleep(2 * 1000000);
    
    /* PLAYING durumuna geçiş (devam et) */
    g_print("Pipeline devam ettiriliyor (PLAYING durumu)...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    /* 3 saniye daha çalışmasını bekle */
    g_usleep(3 * 1000000);
    
    /* NULL durumuna geçiş (tamamen durdur ve kaynakları serbest bırak) */
    g_print("Pipeline durduruluyor (NULL durumu)...\n");
    ret = gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Kaynakları temizle */
    gst_object_unref(pipeline);
    
    return 0;
}



Tabii, kusura bakmayın. 6. maddeyi (Bus ve Mesajlar) aşağıda bulabilirsiniz:

## 6. Bus ve Mesajlar

### 6.1. Bus'a Erişim

GStreamer bus, elementlerden uygulama ana döngüsüne mesaj taşıyan bir mekanizmadır. Bus'a erişmek için:

```c
// Pipeline'ın bus'ını alma
GstBus *bus = gst_element_get_bus(pipeline);
```

### 6.2. Mesajları Çekme

Bus'tan mesaj almak için birkaç yöntem vardır:

1. **Engelleyici (Blocking) Mesaj Alma**:
   ```c
   // Sonsuza kadar bekle ve belirli türdeki mesajları al
   GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
       GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
   ```

2. **Engelleyici Olmayan (Non-blocking) Mesaj Kontrol**:
   ```c
   // Mesaj varsa hemen al, yoksa NULL dön
   GstMessage *msg = gst_bus_pop_filtered(bus,
       GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
   ```

3. **Zaman Aşımlı Bekleme**:
   ```c
   // En fazla 100ms bekle
   GstMessage *msg = gst_bus_timed_pop_filtered(bus, 100 * GST_MSECOND,
       GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
   ```

### 6.3. Mesaj Türleri

En yaygın mesaj türleri:

1. **GST_MESSAGE_ERROR**: Bir hata oluştuğunda gönderilir
2. **GST_MESSAGE_EOS**: End-of-Stream, akışın sonuna ulaşıldığında gönderilir
3. **GST_MESSAGE_STATE_CHANGED**: Bir elementin durumu değiştiğinde gönderilir
4. **GST_MESSAGE_TAG**: Medya metadata'sı (etiketler) bulunduğunda gönderilir
5. **GST_MESSAGE_BUFFERING**: Arabellekleme durumu değiştiğinde gönderilir
6. **GST_MESSAGE_CLOCK_LOST**: Pipeline saati kaybedildiğinde gönderilir
7. **GST_MESSAGE_NEW_CLOCK**: Yeni bir saat seçildiğinde gönderilir
8. **GST_MESSAGE_DURATION_CHANGED**: Medya süresi değiştiğinde gönderilir
9. **GST_MESSAGE_ASYNC_DONE**: Asenkron durum değişikliği tamamlandığında gönderilir

### 6.4. Örnek: Bus Mesajlarını Dinleme ve İşleme

```c
#include <gst/gst.h>

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *) data;
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
            
        case GST_MESSAGE_ERROR: {
            gchar  *debug;
            GError *error;
            
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            
            g_printerr("Hata: %s\n", error->message);
            g_error_free(error);
            
            g_main_loop_quit(loop);
            break;
        }
        
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state, pending_state;
            
            // Sadece pipeline'ın durum değişikliklerini izle
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data)) {
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                g_print("Pipeline durum değişikliği: %s -> %s\n",
                        gst_element_state_get_name(old_state),
                        gst_element_state_get_name(new_state));
            }
            break;
        }
        
        case GST_MESSAGE_TAG: {
            GstTagList *tags = NULL;
            
            gst_message_parse_tag(msg, &tags);
            
            g_print("Yeni etiketler alındı:\n");
            gst_tag_list_foreach(tags, print_tag, NULL);
            
            gst_tag_list_unref(tags);
            break;
        }
        
        default:
            break;
    }
    
    // FALSE döndürerek sinyalin işlenmesine devam edilmesini sağlıyoruz
    return TRUE;
}

static void print_tag(const GstTagList *tags, const gchar *tag, gpointer user_data) {
    GValue val = { 0, };
    gchar *str;
    gint count;
    
    count = gst_tag_list_get_tag_size(tags, tag);
    
    for (gint i = 0; i < count; i++) {
        gst_tag_list_index(tags, tag, i, &val);
        
        if (G_VALUE_HOLDS_STRING(&val))
            str = g_value_dup_string(&val);
        else
            str = gst_value_serialize(&val);
        
        g_print("  %s: %s\n", tag, str);
        g_free(str);
        
        g_value_unset(&val);
    }
}

int main(int argc, char *argv[]) {
    GMainLoop *loop;
    GstElement *pipeline, *source, *decoder, *sink;
    GstBus *bus;
    guint bus_watch_id;
    
    /* GStreamer'ı başlat */
    gst_init(&argc, &argv);
    
    /* Kontrol etmek için dosya adı gerekli */
    if (argc != 2) {
        g_printerr("Bir medya dosyası belirtin: %s dosya_adı\n", argv[0]);
        return -1;
    }
    
    /* Main loop oluştur */
    loop = g_main_loop_new(NULL, FALSE);
    
    /* Pipeline elementlerini oluştur */
    pipeline = gst_pipeline_new("audio-player");
    source = gst_element_factory_make("filesrc", "file-source");
    decoder = gst_element_factory_make("decodebin", "decoder");
    sink = gst_element_factory_make("autoaudiosink", "audio-sink");
    
    if (!pipeline || !source || !decoder || !sink) {
        g_printerr("Elementlerden biri oluşturulamadı.\n");
        return -1;
    }
    
    /* Dosya adını source'a ayarla */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    
    /* Bus izleyici ekle */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);
    
    /* Pipeline'ı kur */
    gst_bin_add_many(GST_BIN(pipeline), source, decoder, sink, NULL);
    
    /* source ve decoder'ı bağla */
    gst_element_link(source, decoder);
    
    /* decoder için pad-added sinyal işleyicisi ekle */
    g_signal_connect(decoder, "pad-added", G_CALLBACK(on_pad_added), sink);
    
    /* Pipeline'ı PLAYING durumuna getir */
    g_print("Pipeline başlatılıyor...\n");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    /* Main loop'u çalıştır */
    g_print("Main loop çalışıyor...\n");
    g_main_loop_run(loop);
    
    /* Temizlik yap */
    g_print("Main loop sonlandırıldı, kaynaklar temizleniyor...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    
    return 0;
}

/* decodebin bir pad oluşturduğunda çağrılacak işlev */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *sink = (GstElement *) data;
    GstPad *sinkpad;
    
    /* Sink'in sink pad'ini al */
    sinkpad = gst_element_get_static_pad(sink, "sink");
    
    /* Eğer sink pad zaten bağlıysa işlem yapma */
    if (gst_pad_is_linked(sinkpad)) {
        gst_object_unref(sinkpad);
        return;
    }
    
    /* decoder'dan gelen pad'i sink pad'e bağla */
    GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);
    
    if (GST_PAD_LINK_FAILED(ret)) {
        g_print("Pad bağlantısı başarısız oldu\n");
    } else {
        g_print("Pad bağlantısı başarılı\n");
    }
    
    gst_object_unref(sinkpad);
}
```

Derleme:

```bash
gcc -o bus-example bus-example.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Çalıştırma:

```bash
./bus-example medya_dosyasi.mp3
```





#### TODO: Devamı eklenecek en kısa zamanda !!!



---

Bu kapsamlı GStreamer öğreticisi, C ve C++ programcılarına GStreamer mimarisini ve kullanımını temel seviyeden orta seviyeye doğru adım adım anlatmayı amaçlamaktadır. Örnekler derlenebilir ve çalıştırılabilir olarak tasarlanmıştır. Daha ileri seviye konulara ilerlemek isteyen geliştiriciler için resmi GStreamer belgelerine ve örneklerine başvurulması önerilir.

## Lisans

Bu öğretici [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) lisansı altında sunulmaktadır. Kaynak belirtilerek paylaşılabilir, değiştirilebilir ve ticari olmayan amaçlarla kullanılabilir.