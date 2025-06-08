#include <gst/gst.h>
#include <iostream>
#include <string>
#include <iomanip>

#ifdef HAS_CUDA
#include <cuda_runtime.h>
#endif

class VideoConverter {
private:
    GstElement *pipeline;
    GstElement *source, *decoder, *converter, *encoder, *sink;
    GstBus *bus;
    GMainLoop *loop;

public:
    VideoConverter() : pipeline(nullptr), loop(nullptr) {
        // GStreamer'ı başlat
        gst_init(nullptr, nullptr);
        loop = g_main_loop_new(nullptr, FALSE);
    }

    ~VideoConverter() {
        cleanup();
    }

    bool createPipeline(const std::string& inputFile, const std::string& outputFile, 
                       const std::string& outputFormat = "mp4") {
        // Pipeline oluştur
        pipeline = gst_pipeline_new("video-converter");
        if (!pipeline) {
            std::cerr << "Pipeline oluşturulamadı!" << std::endl;
            return false;
        }

        // Elementleri oluştur
        source = gst_element_factory_make("filesrc", "source");
        decoder = gst_element_factory_make("decodebin", "decoder");
        converter = gst_element_factory_make("videoconvert", "converter");
        
        // Çıkış formatına göre encoder seç
        if (outputFormat == "mp4" || outputFormat == "h264") {
#ifdef HAS_CUDA
            // CUDA encoder'ı dene, başarısız olursa CPU encoder'a geç
            encoder = gst_element_factory_make("nvh264enc", "encoder");
            if (!encoder) {
                std::cout << "NVIDIA encoder bulunamadı, CPU encoder kullanılıyor..." << std::endl;
                encoder = gst_element_factory_make("x264enc", "encoder");
            } else {
                std::cout << "NVIDIA H.264 encoder kullanılıyor..." << std::endl;
            }
#else
            encoder = gst_element_factory_make("x264enc", "encoder");
#endif
        } else if (outputFormat == "webm") {
            encoder = gst_element_factory_make("vp8enc", "encoder");
        } else if (outputFormat == "avi") {
            encoder = gst_element_factory_make("xvid", "encoder");
        } else {
            encoder = gst_element_factory_make("x264enc", "encoder"); // varsayılan
        }
        
        sink = gst_element_factory_make("filesink", "sink");

        // Elementlerin oluşturulup oluşturulmadığını kontrol et
        if (!source || !decoder || !converter || !encoder || !sink) {
            std::cerr << "Bir veya daha fazla element oluşturulamadı!" << std::endl;
            return false;
        }

        // Dosya yollarını ayarla
        g_object_set(source, "location", inputFile.c_str(), nullptr);
        g_object_set(sink, "location", outputFile.c_str(), nullptr);

        // Encoder ayarları (kalite ve performans için)
        if (outputFormat == "mp4" || outputFormat == "h264") {
#ifdef HAS_CUDA
            // NVIDIA encoder ayarları
            if (g_str_has_prefix(GST_ELEMENT_NAME(encoder), "nvh264enc")) {
                g_object_set(encoder, "bitrate", 2000, nullptr); // 2Mbps
                g_object_set(encoder, "preset", 2, nullptr); // medium preset
                g_object_set(encoder, "rc-mode", 1, nullptr); // CBR mode
            } else {
                // x264 encoder ayarları
                g_object_set(encoder, "bitrate", 2000, nullptr); // 2Mbps
                g_object_set(encoder, "speed-preset", 6, nullptr); // medium preset
            }
#else
            g_object_set(encoder, "bitrate", 2000, nullptr); // 2Mbps
            g_object_set(encoder, "speed-preset", 6, nullptr); // medium preset
#endif
        }

        // Elementleri pipeline'a ekle
        gst_bin_add_many(GST_BIN(pipeline), source, decoder, converter, encoder, sink, nullptr);

        // Static elementleri bağla (decoder hariç, o dinamik)
        if (!gst_element_link(source, decoder)) {
            std::cerr << "Source ve decoder bağlanamadı!" << std::endl;
            return false;
        }

        if (!gst_element_link_many(converter, encoder, sink, nullptr)) {
            std::cerr << "Converter, encoder ve sink bağlanamadı!" << std::endl;
            return false;
        }

        // Decoder için dinamik pad bağlantısı
        g_signal_connect(decoder, "pad-added", G_CALLBACK(on_pad_added), converter);

        return true;
    }

    static void on_pad_added(GstElement *src, GstPad *new_pad, gpointer data) {
        GstElement *converter = GST_ELEMENT(data);
        GstPad *sink_pad = gst_element_get_static_pad(converter, "sink");
        
        // Pad zaten bağlıysa çık
        if (gst_pad_is_linked(sink_pad)) {
            gst_object_unref(sink_pad);
            return;
        }

        // Pad'in video olup olmadığını kontrol et
        GstCaps *new_pad_caps = gst_pad_get_current_caps(new_pad);
        GstStructure *new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
        const gchar *new_pad_type = gst_structure_get_name(new_pad_struct);

        if (g_str_has_prefix(new_pad_type, "video/x-raw")) {
            // Video pad'ini bağla
            GstPadLinkReturn ret = gst_pad_link(new_pad, sink_pad);
            if (GST_PAD_LINK_FAILED(ret)) {
                std::cerr << "Video pad bağlanamadı!" << std::endl;
            } else {
                std::cout << "Video pad başarıyla bağlandı!" << std::endl;
            }
        }

        if (new_pad_caps != nullptr)
            gst_caps_unref(new_pad_caps);
        gst_object_unref(sink_pad);
    }

    static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
        GMainLoop *loop = (GMainLoop*)data;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_EOS:
                std::cout << "Dönüştürme tamamlandı!" << std::endl;
                g_main_loop_quit(loop);
                break;
            
            case GST_MESSAGE_ERROR: {
                gchar *debug;
                GError *error;
                gst_message_parse_error(msg, &error, &debug);
                std::cerr << "Hata: " << error->message << std::endl;
                if (debug) {
                    std::cerr << "Debug: " << debug << std::endl;
                    g_free(debug);
                }
                g_error_free(error);
                g_main_loop_quit(loop);
                break;
            }
            
            case GST_MESSAGE_STATE_CHANGED: {
                if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data)) {
                    GstState old_state, new_state, pending_state;
                    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                    std::cout << "Pipeline durumu: " << gst_element_state_get_name(old_state) 
                             << " -> " << gst_element_state_get_name(new_state) << std::endl;
                }
                break;
            }
            
            default:
                break;
        }
        return TRUE;
    }

    bool convert() {
        if (!pipeline) {
            std::cerr << "Pipeline oluşturulmamış!" << std::endl;
            return false;
        }

        // Bus ayarla
        bus = gst_element_get_bus(pipeline);
        gst_bus_add_watch(bus, bus_call, loop);

        // Pipeline'ı başlat
        std::cout << "Video dönüştürme başlıyor..." << std::endl;
        GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
        
        if (ret == GST_STATE_CHANGE_FAILURE) {
            std::cerr << "Pipeline başlatılamadı!" << std::endl;
            return false;
        }

        // Ana döngüyü başlat
        g_main_loop_run(loop);

        return true;
    }

    void cleanup() {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
            pipeline = nullptr;
        }
        if (bus) {
            gst_object_unref(bus);
            bus = nullptr;
        }
        if (loop) {
            g_main_loop_unref(loop);
            loop = nullptr;
        }
    }

    void printProgress() {
        if (!pipeline) return;

        gint64 current = -1, total = -1;
        
        if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &current) &&
            gst_element_query_duration(pipeline, GST_FORMAT_TIME, &total)) {
            
            if (total > 0) {
                double progress = (double)current / total * 100.0;
                std::cout << "İlerleme: " << std::fixed << std::setprecision(1) 
                         << progress << "%" << std::endl;
            }
        }
    }
};

void printUsage(const char* programName) {
    std::cout << "Kullanım: " << programName << " <giriş_dosyası> <çıkış_dosyası> [format]" << std::endl;
    std::cout << "Formatlar: mp4 (varsayılan), webm, avi" << std::endl;
    std::cout << "Örnek: " << programName << " input.mov output.mp4" << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return -1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    std::string format = (argc > 3) ? argv[3] : "mp4";

    std::cout << "=== GStreamer Video Converter ===" << std::endl;
    std::cout << "Giriş dosyası: " << inputFile << std::endl;
    std::cout << "Çıkış dosyası: " << outputFile << std::endl;
    std::cout << "Format: " << format << std::endl;
    std::cout << "=================================" << std::endl;

    VideoConverter converter;
    
    if (!converter.createPipeline(inputFile, outputFile, format)) {
        std::cerr << "Pipeline oluşturulamadı!" << std::endl;
        return -1;
    }

    if (!converter.convert()) {
        std::cerr << "Dönüştürme başarısız!" << std::endl;
        return -1;
    }

    std::cout << "Dönüştürme başarıyla tamamlandı!" << std::endl;
    return 0;
}