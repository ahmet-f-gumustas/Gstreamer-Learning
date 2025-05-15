# Gstreamer-Learning
I am currently learning Gstreamer with Cpp &amp; Python

 - run code => gcc basic-tutorial-1.c -o basic-tutorial-1 `pkg-config --cflags --libs gstreamer-1.0`




# GStreamer C/C++ Öğreticisi - Türkçe

<img src="./data/Gstreamer-logo.png" alt="GStreamer Logo" width="400"/>

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
```

---


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


---


## 7. Daha Karmaşık Pipeline'lar

### 7.1. Farklı Kaynaklar

GStreamer'da birçok kaynak element bulunur:

1. **filesrc**: Dosyadan veri okuyan element
   ```c
   GstElement *source = gst_element_factory_make("filesrc", "file-source");
   g_object_set(G_OBJECT(source), "location", "video.mp4", NULL);
   ```

2. **uridecodebin**: URI'den veri okuyan ve demux/decode yapan element
   ```c
   GstElement *source = gst_element_factory_make("uridecodebin", "uri-source");
   g_object_set(G_OBJECT(source), "uri", "https://example.com/video.mp4", NULL);
   ```

3. **videotestsrc**: Test video desenleri üreten element
   ```c
   GstElement *source = gst_element_factory_make("videotestsrc", "test-source");
   g_object_set(G_OBJECT(source), "pattern", 0, NULL); // SMPTE deseni
   ```

4. **audiotestsrc**: Test ses sinyalleri üreten element
   ```c
   GstElement *source = gst_element_factory_make("audiotestsrc", "test-source");
   g_object_set(G_OBJECT(source), "freq", 440.0, NULL); // 440 Hz sinüs dalgası (A notası)
   ```

5. **v4l2src**: Video4Linux2 uyumlu kameralar için kaynak
   ```c
   GstElement *source = gst_element_factory_make("v4l2src", "camera-source");
   g_object_set(G_OBJECT(source), "device", "/dev/video0", NULL);
   ```

### 7.2. Demuxer'lar

Demuxer'lar, konteynır formatlarından (MP4, MKV, AVI vb.) ses ve video akışlarını ayıran elementlerdir.

Popüler demuxer'lar:
- **qtdemux**: MP4/MOV dosyaları için
- **matroskademux**: MKV dosyaları için
- **oggdemux**: OGG dosyaları için
- **flvdemux**: FLV dosyaları için

Demuxer'ların özelliği, dinamik pad'ler oluşturmasıdır. Her bir stream (akış) bulunduğunda yeni bir src pad oluşturulur. Bu nedenle, demuxer'ları diğer elementlere bağlamak için sinyal işleyicileri kullanmalıyız:

```c
// Demuxer oluşturma
GstElement *demuxer = gst_element_factory_make("qtdemux", "demuxer");

// Demuxer'a pad-added sinyal işleyicisi ekleme
g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), NULL);

// Sinyal işleyicisi
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    // Pad'in yeteneklerini (capabilities) al
    GstCaps *caps = gst_pad_get_current_caps(pad);
    GstStructure *str = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(str);
    
    // Video akışını işle
    if (g_str_has_prefix(name, "video/")) {
        // Video decoder'a bağla
        // ...
    }
    // Ses akışını işle
    else if (g_str_has_prefix(name, "audio/")) {
        // Ses decoder'a bağla
        // ...
    }
    
    gst_caps_unref(caps);
}
```

### 7.3. Dekoder'lar

Dekoder'lar, sıkıştırılmış medya verisini (H.264, VP8, MP3, AAC vb.) işlenebilir raw formatlara dönüştüren elementlerdir.

Popüler video dekoder'ları:
- **avdec_h264**: H.264 video dekoderi
- **avdec_mpeg2video**: MPEG-2 video dekoderi
- **vp8dec**: VP8 video dekoderi
- **vp9dec**: VP9 video dekoderi

Popüler ses dekoder'ları:
- **avdec_mp3**: MP3 ses dekoderi
- **avdec_aac**: AAC ses dekoderi
- **vorbisdec**: Vorbis ses dekoderi
- **opusdec**: Opus ses dekoderi

Genellikle, belirli bir codec'i doğrudan kullanmak yerine, otomatik codec seçimi yapabilen **decodebin** elementi kullanmak daha kolaydır:

```c
// Otomatik codec seçimli decoder
GstElement *decoder = gst_element_factory_make("decodebin", "decoder");

// Decoder'a sinyal işleyicisi ekleme
g_signal_connect(decoder, "pad-added", G_CALLBACK(on_decoder_pad_added), NULL);
```

### 7.4. Çözücüler (Converter'lar)

Converter'lar, bir medya formatını başka bir formata dönüştüren elementlerdir. Format dönüşümleri, farklı elementlerin uyumlu çalışmasını sağlar.

Popüler converter'lar:
- **videoconvert**: Video piksel formatı dönüştürücü
- **audioconvert**: Ses formatı dönüştürücü
- **audioresample**: Ses örnekleme hızı dönüştürücü
- **videoscale**: Video boyutu değiştirici
- **videorate**: Video kare hızı (framerate) dönüştürücü

```c
// Video dönüştürücü
GstElement *converter = gst_element_factory_make("videoconvert", "converter");

// Ses dönüştürücü ve örnekleme hızı değiştirici
GstElement *audioconvert = gst_element_factory_make("audioconvert", "aconv");
GstElement *audioresample = gst_element_factory_make("audioresample", "aresample");
```

### 7.5. Çıkışlar (Sink'ler)

Sink'ler, pipeline'ın son elementleridir ve veriyi tüketen (görüntüleme, kaydetme, ağ üzerinden gönderme vb.) görevleri yerine getirirler.

Popüler sink'ler:
- **autovideosink**: Sistem için uygun video gösterici
- **autoaudiosink**: Sistem için uygun ses çıkış birimi
- **filesink**: Dosyaya veri kaydedici
- **udpsink**: UDP üzerinden veri gönderici
- **rtmpsink**: RTMP üzerinden yayın yapıcı

```c
// Video gösterici
GstElement *videosink = gst_element_factory_make("autovideosink", "video-sink");

// Ses çıkışı
GstElement *audiosink = gst_element_factory_make("autoaudiosink", "audio-sink");

// Dosyaya kayıt
GstElement *filesink = gst_element_factory_make("filesink", "file-sink");
g_object_set(G_OBJECT(filesink), "location", "output.mp4", NULL);
```

### 7.6. Örnek: Video Dosyası Oynatma

Aşağıdaki örnek, bir video dosyasını oynatmak için daha karmaşık bir pipeline oluşturur:

```c
#include <gst/gst.h>

/* Sinyal işleyicisi fonksiyon prototipi */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data);

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *demuxer, *video_decoder, *video_convert, *video_sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    
    /* GStreamer'ı başlat */
    gst_init(&argc, &argv);
    
    /* Bir medya dosyası belirtildiğinden emin ol */
    if (argc != 2) {
        g_printerr("Bir video dosyası belirtin: %s <dosya>\n", argv[0]);
        return -1;
    }
    
    /* Elementleri oluştur */
    pipeline = gst_pipeline_new("video-player");
    source = gst_element_factory_make("filesrc", "file-source");
    demuxer = gst_element_factory_make("qtdemux", "demuxer");      // MP4/MOV demuxer
    video_decoder = gst_element_factory_make("avdec_h264", "video-decoder");  // H.264 decoder
    video_convert = gst_element_factory_make("videoconvert", "converter");
    video_sink = gst_element_factory_make("autovideosink", "video-sink");
    
    /* Tüm elementlerin oluşturulduğundan emin ol */
    if (!pipeline || !source || !demuxer || !video_decoder || !video_convert || !video_sink) {
        g_printerr("Elementlerden biri oluşturulamadı.\n");
        return -1;
    }
    
    /* Dosya konumunu ayarla */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    
    /* Pipeline'a elementleri ekle */
    gst_bin_add_many(GST_BIN(pipeline), source, demuxer, video_decoder, video_convert, video_sink, NULL);
    
    /* Elementleri bağlamaya başla */
    /* source -> demuxer */
    if (!gst_element_link(source, demuxer)) {
        g_printerr("Elementler bağlanamadı: source -> demuxer\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* demuxer'ın h264 video akışı için sinyal izleyicisi ekle */
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), video_decoder);
    
    /* video akışını sonraki elementlere bağla */
    if (!gst_element_link_many(video_decoder, video_convert, video_sink, NULL)) {
        g_printerr("Elementler bağlanamadı: decoder -> converter -> sink\n");
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
    
    /* Bus üzerinden mesajları bekle */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    /* Mesaj işleme */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Hata %s\n", err->message);
                if (debug_info) {
                    g_printerr("Hata ayıklama bilgisi: %s\n", debug_info);
                }
                g_free(debug_info);
                g_error_free(err);
                break;
            case GST_MESSAGE_EOS:
                g_print("End of stream\n");
                break;
            default:
                g_printerr("Beklenmeyen mesaj alındı\n");
                break;
        }
        gst_message_unref(msg);
    }
    
    /* Pipeline'ı durdur */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Kaynakları temizle */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    return 0;
}

/* Demuxer bir pad oluşturduğunda çağrılacak fonksiyon */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *decoder = (GstElement *) data;
    GstPad *sink_pad;
    GstCaps *caps;
    GstStructure *structure;
    const gchar *name;
    
    /* Pad'in yeteneklerini al */
    caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, NULL);
    }
    
    structure = gst_caps_get_structure(caps, 0);
    name = gst_structure_get_name(structure);
    
    /* Sadece video akışlarıyla ilgileniyoruz */
    if (g_str_has_prefix(name, "video/x-h264")) {
        /* Decoder'ın sink pad'ini al */
        sink_pad = gst_element_get_static_pad(decoder, "sink");
        
        /* Pad'leri bağla */
        if (GST_PAD_LINK_FAILED(gst_pad_link(pad, sink_pad))) {
            g_printerr("Demuxer'dan video decoder'a pad bağlantısı başarısız oldu.\n");
        } else {
            g_print("Demuxer -> video decoder pad bağlantısı başarılı.\n");
        }
        
        gst_object_unref(sink_pad);
    }
    
    if (caps) {
        gst_caps_unref(caps);
    }
}
```

Derleme:

```bash
gcc -o video-player video-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Çalıştırma:

```bash
./video-player video.mp4
```

### 7.7. Örnek: Ses Dosyası Oynatma

Aşağıdaki örnek, bir ses dosyasını oynatmak için bir pipeline oluşturur:

```c
#include <gst/gst.h>

/* Sinyal işleyici fonksiyon prototipi */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data);

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *demuxer, *audio_decoder, *audio_convert, *audio_resample, *audio_sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    
    /* GStreamer'ı başlat */
    gst_init(&argc, &argv);
    
    /* Bir medya dosyası belirtildiğinden emin ol */
    if (argc != 2) {
        g_printerr("Bir ses dosyası belirtin: %s <dosya>\n", argv[0]);
        return -1;
    }
    
    /* Elementleri oluştur */
    pipeline = gst_pipeline_new("audio-player");
    source = gst_element_factory_make("filesrc", "file-source");
    demuxer = gst_element_factory_make("oggdemux", "demuxer");       // OGG demuxer
    audio_decoder = gst_element_factory_make("vorbisdec", "audio-decoder");  // Vorbis decoder
    audio_convert = gst_element_factory_make("audioconvert", "converter");
    audio_resample = gst_element_factory_make("audioresample", "resampler");
    audio_sink = gst_element_factory_make("autoaudiosink", "audio-sink");
    
    /* Tüm elementlerin oluşturulduğundan emin ol */
    if (!pipeline || !source || !demuxer || !audio_decoder || 
        !audio_convert || !audio_resample || !audio_sink) {
        g_printerr("Elementlerden biri oluşturulamadı.\n");
        return -1;
    }
    
    /* Dosya konumunu ayarla */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    
    /* Pipeline'a elementleri ekle */
    gst_bin_add_many(GST_BIN(pipeline), 
                    source, demuxer, audio_decoder, 
                    audio_convert, audio_resample, audio_sink, NULL);
    
    /* Elementleri bağlamaya başla */
    /* source -> demuxer */
    if (!gst_element_link(source, demuxer)) {
        g_printerr("Elementler bağlanamadı: source -> demuxer\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* demuxer'ın vorbis ses akışı için sinyal izleyicisi ekle */
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), audio_decoder);
    
    /* ses akışını sonraki elementlere bağla */
    if (!gst_element_link_many(audio_decoder, audio_convert, audio_resample, audio_sink, NULL)) {
        g_printerr("Elementler bağlanamadı: decoder -> converter -> resampler -> sink\n");
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
    
    /* Bus üzerinden mesajları bekle */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    /* Mesaj işleme */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Hata %s\n", err->message);
                if (debug_info) {
                    g_printerr("Hata ayıklama bilgisi: %s\n", debug_info);
                }
                g_free(debug_info);
                g_error_free(err);
                break;
            case GST_MESSAGE_EOS:
                g_print("End of stream\n");
                break;
            default:
                g_printerr("Beklenmeyen mesaj alındı\n");
                break;
        }
        gst_message_unref(msg);
    }
    
    /* Pipeline'ı durdur */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Kaynakları temizle */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    return 0;
}

/* Demuxer bir pad oluşturduğunda çağrılacak fonksiyon */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *decoder = (GstElement *) data;
    GstPad *sink_pad;
    GstCaps *caps;
    GstStructure *structure;
    const gchar *name;
    
    /* Pad'in yeteneklerini al */
    caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, NULL);
    }
    
    structure = gst_caps_get_structure(caps, 0);
    name = gst_structure_get_name(structure);
    
    /* Sadece ses akışlarıyla ilgileniyoruz */
    if (g_str_has_prefix(name, "audio/x-vorbis")) {
        /* Decoder'ın sink pad'ini al */
        sink_pad = gst_element_get_static_pad(decoder, "sink");
        
        /* Pad'leri bağla */
        if (GST_PAD_LINK_FAILED(gst_pad_link(pad, sink_pad))) {
            g_printerr("Demuxer'dan ses decoder'a pad bağlantısı başarısız oldu.\n");
        } else {
            g_print("Demuxer -> ses decoder pad bağlantısı başarılı.\n");
        }
        
        gst_object_unref(sink_pad);
    }
    
    if (caps) {
        gst_caps_unref(caps);
    }
}
```

Derleme:

```bash
gcc -o audio-player audio-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Çalıştırma:

```bash
./audio-player muzik.ogg
```

---


## 8. Hata Yönetimi

### 8.1. Dönüş Değerlerini Kontrol Etme

GStreamer API fonksiyonlarının çoğu, başarı veya başarısızlık durumunu gösteren değerler döndürür. Bu değerleri kontrol etmek, hataları erken tespit etmenin en iyi yoludur:

```c
// Element oluşturma
GstElement *element = gst_element_factory_make("elementadı", "özel-ad");
if (!element) {
    g_printerr("Element oluşturulamadı. Element eklentisi kurulu olabilir mi?\n");
    // Hata işleme...
    return -1;
}

// Element bağlama
if (!gst_element_link(element1, element2)) {
    g_printerr("Elementler bağlanamadı. Pad'ler uyumsuz olabilir mi?\n");
    // Hata işleme...
    return -1;
}

// Durum değiştirme
GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr("Pipeline durumu değiştirilemedi. Elementlerde sorun olabilir.\n");
    // Hata işleme...
    return -1;
}
```

### 8.2. Bus Üzerinden Hata Mesajlarını Yakalama

Pipeline çalışırken oluşan hataları yakalamak için bus mesajlarını dinlemek gerekir:

```c
// Bus üzerinden hata veya EOS mesajları için bekle
GstBus *bus = gst_element_get_bus(pipeline);
GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
    GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

// Mesajı işle
if (msg != NULL) {
    GError *err;
    gchar *debug_info;
    
    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        gst_message_parse_error(msg, &err, &debug_info);
        g_printerr("Pipeline hatası: %s\n", err->message);
        g_printerr("Hata ayıklama bilgisi: %s\n", debug_info ? debug_info : "Yok");
        g_free(debug_info);
        g_error_free(err);
    }
    
    gst_message_unref(msg);
}
```

### 8.3. Örnek: Sağlam Hata Kontrolü

Aşağıdaki örnek, kapsamlı hata kontrolü yapan bir uygulamayı göstermektedir:

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline = NULL;
    GstElement *source = NULL, *converter = NULL, *sink = NULL;
    GstBus *bus = NULL;
    GstMessage *msg = NULL;
    GstStateChangeReturn state_ret;
    gboolean terminate = FALSE;
    
    /* GStreamer'ı başlat */
    gst_init(&argc, &argv);
    
    /* Hata işleme için temizleme noktaları belirt */
    do {
        /* Pipeline elementlerini oluştur */
        pipeline = gst_pipeline_new("error-handling-example");
        if (!pipeline) {
            g_printerr("Pipeline oluşturulamadı\n");
            break;
        }
        
        source = gst_element_factory_make("videotestsrc", "source");
        if (!source) {
            g_printerr("Videotestsrc oluşturulamadı. GStreamer eklentileri doğru kurulu mu?\n");
            break;
        }
        
        converter = gst_element_factory_make("videoconvert", "converter");
        if (!converter) {
            g_printerr("Videoconvert oluşturulamadı\n");
            break;
        }
        
        sink = gst_element_factory_make("autovideosink", "sink");
        if (!sink) {
            g_printerr("Autovideosink oluşturulamadı\n");
            break;
        }
        
        /* Pipeline'a elementleri ekle */
        gst_bin_add_many(GST_BIN(pipeline), source, converter, sink, NULL);
        
        /* Elementleri bağla */
        if (!gst_element_link_many(source, converter, sink, NULL)) {
            g_printerr("Elementler bağlanamadı\n");
            break;
        }
        
        /* Pipeline durumunu PLAYING olarak ayarla */
        g_print("Pipeline başlatılıyor...\n");
        state_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        if (state_ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr("Pipeline PLAYING durumuna getirilemedi\n");
            break;
        }
        
        /* Bus'ı al ve mesajları dinle */
        bus = gst_element_get_bus(pipeline);
        
        /* Asenkron durum değişikliğinin tamamlanmasını bekle */
        if (state_ret == GST_STATE_CHANGE_ASYNC) {
            g_print("Asenkron durum değişikliği için bekleniyor...\n");
            state_ret = gst_element_get_state(pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
            if (state_ret == GST_STATE_CHANGE_FAILURE) {
                g_printerr("Durum değişikliği başarısız oldu\n");
                break;
            }
        }
        
        g_print("Pipeline başlatıldı, 5 saniye çalıştırılacak...\n");
        
        /* Mesaj döngüsü */
        terminate = FALSE;
        do {
            msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
                GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);
            
            /* Zaman aşımı (NULL mesaj) - normal çıkış */
            if (!msg) {
                g_print("5 saniyelik zaman aşımına ulaşıldı, çıkılıyor...\n");
                terminate = TRUE;
                break;
            }
            
            /* Mesajı işle */
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR: {
                    GError *err;
                    gchar *debug;
                    
                    gst_message_parse_error(msg, &err, &debug);
                    g_printerr("Hata: %s\n", err->message);
                    if (debug) {
                        g_printerr("Hata Ayıklama Detayları: %s\n", debug);
                    }
                    g_error_free(err);
                    g_free(debug);
                    
                    terminate = TRUE;
                    break;
                }
                case GST_MESSAGE_EOS:
                    g_print("End of Stream alındı\n");
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    /* Sadece pipeline'ın durum değişikliklerini izle */
                    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                        GstState old_state, new_state, pending_state;
                        gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                        g_print("Pipeline durum değişikliği: %s -> %s\n",
                                gst_element_state_get_name(old_state),
                                gst_element_state_get_name(new_state));
                    }
                    break;
                default:
                    g_printerr("Beklenmeyen mesaj alındı\n");
                    break;
            }
            
            gst_message_unref(msg);
            
        } while (!terminate);
        
    } while (FALSE); /* Bu do-while bloğu sadece bir kez çalışır, erken çıkış için kullanılır */
    
    /* Temizlik */
    g_print("Kaynakları temizleniyor...\n");
    
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }
    
    if (bus) {
        gst_object_unref(bus);
    }
    
    return 0;
}
```

Derleme:

```bash
gcc -o error-handling error-handling.c $(pkg-config --cflags --libs gstreamer-1.0)
```

## 9. Etkinlikler (Events) ve Sorgular (Queries)

### 9.1. Etkinliklerin Rolü

**Etkinlikler (Events)**, pipeline içinde element durumlarını veya akış davranışını değiştirmek için kullanılır. Etkinlikler genellikle akışın tersine ilerler (upstream veya downstream).

Yaygın etkinlik türleri:
- **Seek Events**: Medya konumunu değiştirmek için
- **Flush Events**: Tamponları boşaltmak için
- **EOS Events**: End-of-Stream (akış sonu) sinyali göndermek için
- **QOS Events**: Quality of Service (hizmet kalitesi) bilgisi göndermek için

```c
// Bir seek etkinliği gönderme örneği (medyada belirli bir konuma atlama)
gboolean seek_res;
seek_res = gst_element_seek_simple(
    pipeline,                  // İşlemi yapacak element
    GST_FORMAT_TIME,           // Konum formatı (zaman)
    GST_SEEK_FLAG_FLUSH |      // Tamponları temizle
    GST_SEEK_FLAG_KEY_UNIT,    // En yakın anahtar karede dur
    30 * GST_SECOND            // 30. saniyeye atla
);

if (!seek_res) {
    g_printerr("Seek işlemi başarısız oldu\n");
}
```

### 9.2. Sorguların Rolü

**Sorgular (Queries)**, elementlerden bilgi almak için kullanılır. Pipeline içindeki elementlere çeşitli bilgi soruları gönderebilirsiniz.

Yaygın sorgu türleri:
- **Position Query**: Mevcut konum bilgisi
- **Duration Query**: Medya süresi
- **Seeking Query**: Atlama (seeking) desteği ve aralığı
- **Formats Query**: Desteklenen format türleri

```c
// Medya süresini sorgulama örneği
gint64 duration;
gboolean res;

res = gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration);
if (res) {
    g_print("Medya süresi: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(duration));
} else {
    g_printerr("Süre sorgulanamadı\n");
}

// Mevcut konumu sorgulama örneği
gint64 position;
res = gst_element_query_position(pipeline, GST_FORMAT_TIME, &position);
if (res) {
    g_print("Mevcut konum: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS(position));
} else {
    g_printerr("Konum sorgulanamadı\n");
}
```

### 9.3. Örnek: Basit Bir Seek Uygulaması

Aşağıdaki örnek, bir medya dosyasını oynatan ve kullanıcının medyada ileri/geri atlamasına izin veren basit bir uygulamadır:

```c
#include <gst/gst.h>
#include <stdio.h>

static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, gpointer data);
static gboolean seek_element(GstElement *pipeline, gdouble rate, gint64 position);

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GIOChannel *io_stdin;
    guint io_watch_id;
    
    /* GStreamer'ı başlat */
    gst_init(&argc, &argv);
    
    /* Kullanım bilgisi */
    if (argc != 2) {
        g_printerr("Kullanım: %s <medya dosyası>\n", argv[0]);
        return -1;
    }
    
    /* playbin elementi oluştur (otomatik pipeline kurar) */
    pipeline = gst_element_factory_make("playbin", "player");
    if (!pipeline) {
        g_printerr("Playbin elementi oluşturulamadı\n");
        return -1;
    }
    
    /* Oynatılacak dosyayı ayarla */
    g_object_set(G_OBJECT(pipeline), "uri", gst_filename_to_uri(argv[1], NULL), NULL);
    
    /* Klavye giriş kanalını ayarla */
    io_stdin = g_io_channel_unix_new(fileno(stdin));
    g_io_channel_set_flags(io_stdin, G_IO_FLAG_NONBLOCK, NULL);
    
    /* Kullanım bilgilerini göster */
    g_print(
        "\n"
        "*************************************************\n"
        "* 'q': Çıkış                                    *\n"
        "* 'p': Oynat/Duraklat                           *\n"
        "* '+': 10 saniye ileri                          *\n"
        "* '-': 10 saniye geri                           *\n"
        "* 'r': Normal hıza dön (1.0x)                   *\n"
        "* '1': 0.5x hız (yavaş)                         *\n"
        "* '2': 2.0x hız (hızlı)                         *\n"
        "*************************************************\n\n"
    );
    
    /* Klavye izleyicisi ekle */
    io_watch_id = g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)handle_keyboard, pipeline);
    
    /* Pipeline'ı oynat */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    /* Main loop başlat */
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    
    /* Temizlik */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(io_watch_id);
    g_io_channel_unref(io_stdin);
    g_main_loop_unref(loop);
    
    return 0;
}

/* Klavye tuşlarını işle */
static gboolean handle_keyboard(GIOChannel *source, GIOCondition cond, gpointer data) {
    GstElement *pipeline = (GstElement *)data;
    GstState state, pending;
    gchar *str = NULL;
    
    if (g_io_channel_read_line(source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL) {
        return TRUE;
    }
    
    switch (str[0]) {
        case 'q':
        case 'Q':
            g_main_loop_quit((GMainLoop *)g_main_loop_new(NULL, FALSE));
            break;
        case 'p':
        case 'P':
            gst_element_get_state(pipeline, &state, &pending, 0);
            if (state == GST_STATE_PLAYING)
                gst_element_set_state(pipeline, GST_STATE_PAUSED);
            else
                gst_element_set_state(pipeline, GST_STATE_PLAYING);
            break;
        case '+':
            {
                gint64 position;
                if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
                    seek_element(pipeline, 1.0, position + 10 * GST_SECOND);
                }
            }
            break;
        case '-':
            {
                gint64 position;
                if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
                    seek_element(pipeline, 1.0, position - 10 * GST_SECOND);
                }
            }
            break;
        case 'r':
        case 'R':
            seek_element(pipeline, 1.0, -1);
            break;
        case '1':
            seek_element(pipeline, 0.5, -1);
            break;
        case '2':
            seek_element(pipeline, 2.0, -1);
            break;
        default:
            break;
    }
    
    g_free(str);
    return TRUE;
}

/* Seek işlemini gerçekleştir */
static gboolean seek_element(GstElement *pipeline, gdouble rate, gint64 position) {
    GstEvent *seek_event;
    
    /* Mevcut konumu koru (-1 ise) */
    if (position < 0) {
        if (!gst_element_query_position(pipeline, GST_FORMAT_TIME, &position)) {
            g_printerr("Mevcut konum alınamadı\n");
            return FALSE;
        }
    }
    
    /* Yeni bir seek event oluştur */
    if (rate > 0) {
        seek_event = gst_event_new_seek(rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                                       GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_END, 0);
    } else {
        seek_event = gst_event_new_seek(rate, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
                                       GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, position);
    }
    
    /* Seek event'i gönder */
    if (!gst_element_send_event(pipeline, seek_event)) {
        g_printerr("Seek işlemi başarısız oldu\n");
        return FALSE;
    }
    
    g_print("Pozisyon: %" GST_TIME_FORMAT ", Hız: %.2f\n", GST_TIME_ARGS(position), rate);
    return TRUE;
}
```

Derleme:

```bash
gcc -o seek-player seek-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Çalıştırma:

```bash
./seek-player video.mp4
```

## 10. Pratik Örnekler ve İpuçları

### 10.1. Command-line Araçları ile Pipeline Test Etme

GStreamer, pipeline'ları komut satırından test etmek için `gst-launch-1.0` adlı güçlü bir araç sunar. Bu araç, kod yazmadan hızlıca pipeline'lar oluşturmanızı ve test etmenizi sağlar.

Temel kullanım:

```bash
gst-launch-1.0 [seçenekler] ELEMENT [özellikleri] ! ELEMENT [özellikleri] ! ...
```

Örnekler:

1. **Basit video oynatma**:
   ```bash
   gst-launch-1.0 playbin uri=file:///path/to/video.mp4
   ```

2. **Test video gösterimi**:
   ```bash
   gst-launch-1.0 videotestsrc ! videoconvert ! autovideosink
   ```

3. **Ses çalma**:
   ```bash
   gst-launch-1.0 audiotestsrc ! audioconvert ! autoaudiosink
   ```

4. **Dosyadan video oynatma (elementler ile)**:
   ```bash
   gst-launch-1.0 filesrc location=/path/to/video.mp4 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert ! autovideosink
   ```

5. **Webcam'den görüntü alma**:
   ```bash
   gst-launch-1.0 v4l2src ! videoconvert ! autovideosink
   ```

6. **Video dosyasını farklı formata dönüştürme**:
   ```bash
   gst-launch-1.0 filesrc location=input.mp4 ! qtdemux ! h264parse ! avdec_h264 ! x264enc ! mp4mux ! filesink location=output.mp4
   ```

7. **Ekran kaydı**:
   ```bash
   gst-launch-1.0 ximagesrc ! videoconvert ! x264enc ! mp4mux ! filesink location=screen-recording.mp4
   ```

Bu komutlar, GStreamer uygulamanızı geliştirmeden önce pipeline tasarımınızı test etmenize yardımcı olur.

### 10.2. Kullanışlı Elementler

1. **queue**: Elementler arasında kuyruk (buffer) oluşturur ve farklı thread'lerde çalışmalarını sağlar
   ```c
   GstElement *queue = gst_element_factory_make("queue", "buffer");
   g_object_set(G_OBJECT(queue), "max-size-buffers", 100, NULL);
   ```

2. **tee**: Bir kaynaktan gelen veriyi birden fazla çıkışa dağıtmak için kullanılır
   ```c
   GstElement *tee = gst_element_factory_make("tee", "stream-divider");
   GstPad *tee_audio_pad = gst_element_get_request_pad(tee, "src_%u");
   GstPad *tee_video_pad = gst_element_get_request_pad(tee, "src_%u");
   ```

3. **fakesink**: Veri akışını test etmek için gerçek bir çıkış olmadan veriyi tüketen element
   ```c
   GstElement *fakesink = gst_element_factory_make("fakesink", "test-sink");
   g_object_set(G_OBJECT(fakesink), "sync", TRUE, NULL);
   ```

4. **identity**: Veri akışını değiştirmeden geçiren, isteğe bağlı olarak veri hakkında bilgi veren element
   ```c
   GstElement *identity = gst_element_factory_make("identity", "monitor");
   g_object_set(G_OBJECT(identity), "dump", TRUE, "silent", FALSE, NULL);
   ```

5. **valve**: Bir akışı açıp kapatmak için kullanılan kontrol element
   ```c
   GstElement *valve = gst_element_factory_make("valve", "control");
   g_object_set(G_OBJECT(valve), "drop", FALSE, NULL);  // TRUE = veriyi engelle, FALSE = veriyi geçir
   ```

### 10.3. Multithreading ve GStreamer

GStreamer, içsel olarak multithreaded bir mimariye sahiptir. Bununla birlikte, uygulamanızda GStreamer pipeline'ı ile UI thread'i arasındaki etkileşimi yönetmek için bazı desenler kullanabilirsiniz:

1. **GMainLoop ile Bus İzleme**:
   ```c
   /* Bus izleyici oluşturma */
   GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
   guint bus_watch_id = gst_bus_add_watch(bus, bus_callback, user_data);
   gst_object_unref(bus);### 7.6. Örnek: Video Dosyası Oynatma

Aşağıdaki örnek, bir video dosyasını oynatmak için daha karmaşık bir pipeline oluşturur:

```c
#include <gst/gst.h>

/* Sinyal işleyicisi fonksiyon prototipi */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data);

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *demuxer, *video_decoder, *video_convert, *video_sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    
    /* GStreamer'ı başlat */
    gst_init(&argc, &argv);
    
    /* Bir medya dosyası belirtildiğinden emin ol */
    if (argc != 2) {
        g_printerr("Bir video dosyası belirtin: %s <dosya>\n", argv[0]);
        return -1;
    }
    
    /* Elementleri oluştur */
    pipeline = gst_pipeline_new("video-player");
    source = gst_element_factory_make("filesrc", "file-source");
    demuxer = gst_element_factory_make("qtdemux", "demuxer");      // MP4/MOV demuxer
    video_decoder = gst_element_factory_make("avdec_h264", "video-decoder");  // H.264 decoder
    video_convert = gst_element_factory_make("videoconvert", "converter");
    video_sink = gst_element_factory_make("autovideosink", "video-sink");
    
    /* Tüm elementlerin oluşturulduğundan emin ol */
    if (!pipeline || !source || !demuxer || !video_decoder || !video_convert || !video_sink) {
        g_printerr("Elementlerden biri oluşturulamadı.\n");
        return -1;
    }
    
    /* Dosya konumunu ayarla */
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    
    /* Pipeline'a elementleri ekle */
    gst_bin_add_many(GST_BIN(pipeline), source, demuxer, video_decoder, video_convert, video_sink, NULL);
    
    /* Elementleri bağlamaya başla */
    /* source -> demuxer */
    if (!gst_element_link(source, demuxer)) {
        g_printerr("Elementler bağlanamadı: source -> demuxer\n");
        gst_object_unref(pipeline);
        return -1;
    }
    
    /* demuxer'ın h264 video akışı için sinyal izleyicisi ekle */
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), video_decoder);
    
    /* video akışını sonraki elementlere bağla */
    if (!gst_element_link_many(video_decoder, video_convert, video_sink, NULL)) {
        g_printerr("Elementler bağlanamadı: decoder -> converter -> sink\n");
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
    
    /* Bus üzerinden mesajları bekle */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    
    /* Mesaj işleme */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Hata %s\n", err->message);
                if (debug_info) {
                    g_printerr("Hata ayıklama bilgisi: %s\n", debug_info);
                }
                g_free(debug_info);
                g_error_free(err);
                break;
            case GST_MESSAGE_EOS:
                g_print("End of stream\n");
                break;
            default:
                g_printerr("Beklenmeyen mesaj alındı\n");
                break;
        }
        gst_message_unref(msg);
    }
    
    /* Pipeline'ı durdur */
    gst_element_set_state(pipeline, GST_STATE_NULL);
    
    /* Kaynakları temizle */
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    
    return 0;
}

/* Demuxer bir pad oluşturduğunda çağrılacak fonksiyon */
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *decoder = (GstElement *) data;
    GstPad *sink_pad;
    GstCaps *caps;
    GstStructure *structure;
    const gchar *name;
    
    /* Pad'in yeteneklerini al */
    caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, NULL);
    }
    
    structure = gst_caps_get_structure(caps, 0);
    name = gst_structure_get_name(structure);
    
    /* Sadece video akışlarıyla ilgileniyoruz */
    if (g_str_has_prefix(name, "video/x-h264")) {
        /* Decoder'ın sink pad'ini al */
        sink_pad = gst_element_get_static_pad(decoder, "sink");
        
        /* Pad'leri bağla */
        if (GST_PAD_LINK_FAILED(gst_pad_link(pad, sink_pad))) {
            g_printerr("Demuxer'dan video decoder'a pad bağlantısı başarısız oldu.\n");
        } else {
            g_print("Demuxer -> video decoder pad bağlantısı başarılı.\n");
        }
        
        gst_object_unref(sink_pad);
    }
    
    if (caps) {
        gst_caps_unref(caps);
    }
}
```

Derleme:

```bash
gcc -o video-player video-player.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Çalıştırma:

```bash
./video-player video.mp4
```



#### Dosya altında örnekler bulunmaktadır.



---

Bu kapsamlı GStreamer öğreticisi, C ve C++ programcılarına GStreamer mimarisini ve kullanımını temel seviyeden orta seviyeye doğru adım adım anlatmayı amaçlamaktadır. Örnekler derlenebilir ve çalıştırılabilir olarak tasarlanmıştır. Daha ileri seviye konulara ilerlemek isteyen geliştiriciler için resmi GStreamer belgelerine ve örneklerine başvurulması önerilir.

## Lisans

Bu öğretici [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) lisansı altında sunulmaktadır. Kaynak belirtilerek paylaşılabilir, değiştirilebilir ve ticari olmayan amaçlarla kullanılabilir.