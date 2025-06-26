/**
 * @file main.cpp
 * @brief GStreamer Video Analytics Pipeline ana programı
 * 
 * Bu dosya, uygulamanın giriş noktasını içerir ve komut satırı
 * argümanlarını işleyerek pipeline'ı başlatır.
 */

#include <iostream>
#include <memory>
#include <string>
#include <csignal>
#include <cstring>
#include <thread>
#include <chrono>
#include <iomanip>
#include <yaml-cpp/yaml.h>
#include <gst/gst.h>

#include "pipeline_manager.h"
#include "video_processor.h"
#include "motion_detector.h"
#include "rtsp_streamer.h"

// Global pipeline yöneticisi (sinyal işleyici için)
std::unique_ptr<PipelineManager> g_pipeline_manager;

/**
 * @brief Sinyal işleyici (SIGINT, SIGTERM)
 * @param signum Sinyal numarası
 */
void signalHandler(int signum) {
    std::cout << "\n[INFO] Sinyal alındı (" << signum << "), kapatılıyor..." << std::endl;
    
    if (g_pipeline_manager && g_pipeline_manager->isRunning()) {
        g_pipeline_manager->stop();
    }
    
    // Çıkış
    std::exit(0);
}

/**
 * @brief Kullanım bilgisini gösterir
 * @param program_name Program adı
 */
void showUsage(const std::string& program_name) {
    std::cout << "Kullanım: " << program_name << " [SEÇENEKLER]\n\n"
              << "Seçenekler:\n"
              << "  -i, --input <kaynak>      Video kaynağı (dosya/webcam/rtsp://...)\n"
              << "  -o, --output <hedef>      Çıkış hedefi (display/file/rtsp://...)\n"
              << "  -c, --config <dosya>      Yapılandırma dosyası (YAML)\n"
              << "  --motion-detect           Hareket algılamayı etkinleştir\n"
              << "  --use-gpu                 GPU hızlandırma kullan\n"
              << "  --record <dosya>          Video kaydı yap\n"
              << "  --width <genişlik>        Video genişliği (varsayılan: 1920)\n"
              << "  --height <yükseklik>      Video yüksekliği (varsayılan: 1080)\n"
              << "  --fps <fps>               Kare hızı (varsayılan: 30)\n"
              << "  --bitrate <bitrate>       Bit hızı (varsayılan: 4000000)\n"
              << "  --rtsp-port <port>        RTSP server portu (varsayılan: 8554)\n"
              << "  -v, --verbose             Ayrıntılı çıktı\n"
              << "  -h, --help                Bu yardım mesajını göster\n\n"
              << "Örnekler:\n"
              << "  # Video dosyası işleme\n"
              << "  " << program_name << " -i video.mp4\n\n"
              << "  # Web kamerası ile hareket algılama\n"
              << "  " << program_name << " -i webcam --motion-detect\n\n"
              << "  # RTSP stream işleme ve yayınlama\n"
              << "  " << program_name << " -i rtsp://192.168.1.100:554/stream -o rtsp://0.0.0.0:8554/live\n\n"
              << "  # GPU ile video işleme ve kayıt\n"
              << "  " << program_name << " -i video.mp4 --use-gpu --record output.mp4\n"
              << std::endl;
}

/**
 * @brief Komut satırı argümanlarını parse eder
 * @param argc Argüman sayısı
 * @param argv Argüman dizisi
 * @param config Pipeline yapılandırması
 * @return Başarılı ise true
 */
bool parseArguments(int argc, char* argv[], PipelineConfig& config) {
    bool verbose = false;
    std::string config_file;
    
    // Argümanları işle
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            showUsage(argv[0]);
            return false;
        }
        else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        }
        else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            std::string input = argv[++i];
            
            // Giriş türünü belirle
            if (input == "webcam") {
                config.source_type = SourceType::WEBCAM;
                config.source_location = "/dev/video0";
            }
            else if (input.find("rtsp://") == 0) {
                config.source_type = SourceType::RTSP;
                config.source_location = input;
            }
            else if (input.find("http://") == 0 || input.find("https://") == 0) {
                config.source_type = SourceType::HTTP;
                config.source_location = input;
            }
            else {
                config.source_type = SourceType::FILE;
                config.source_location = input;
            }
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            std::string output = argv[++i];
            
            // Çıkış türünü belirle
            if (output == "display") {
                config.sink_type = SinkType::DISPLAY;
            }
            else if (output.find("rtsp://") == 0) {
                config.sink_type = SinkType::RTSP;
                config.sink_location = output;
            }
            else {
                config.sink_type = SinkType::FILE;
                config.sink_location = output;
            }
        }
        else if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_file = argv[++i];
        }
        else if (arg == "--motion-detect") {
            config.enable_motion_detection = true;
        }
        else if (arg == "--use-gpu") {
            config.enable_gpu_acceleration = true;
        }
        else if (arg == "--record" && i + 1 < argc) {
            config.enable_recording = true;
            config.record_location = argv[++i];
        }
        else if (arg == "--width" && i + 1 < argc) {
            config.width = std::stoi(argv[++i]);
        }
        else if (arg == "--height" && i + 1 < argc) {
            config.height = std::stoi(argv[++i]);
        }
        else if (arg == "--fps" && i + 1 < argc) {
            config.framerate = std::stoi(argv[++i]);
        }
        else if (arg == "--bitrate" && i + 1 < argc) {
            config.bitrate = std::stoi(argv[++i]);
        }
        else if (arg == "--rtsp-port" && i + 1 < argc) {
            config.rtsp_port = std::stoi(argv[++i]);
        }
        else {
            std::cerr << "[HATA] Bilinmeyen argüman: " << arg << std::endl;
            showUsage(argv[0]);
            return false;
        }
    }
    
    // Config dosyası varsa yükle
    if (!config_file.empty()) {
        try {
            YAML::Node yaml_config = YAML::LoadFile(config_file);
            
            // Pipeline ayarları
            if (yaml_config["pipeline"]) {
                auto pipeline = yaml_config["pipeline"];
                
                // Giriş ayarları
                if (pipeline["input"]) {
                    auto input = pipeline["input"];
                    std::string type = input["type"].as<std::string>();
                    
                    if (type == "file") config.source_type = SourceType::FILE;
                    else if (type == "webcam") config.source_type = SourceType::WEBCAM;
                    else if (type == "rtsp") config.source_type = SourceType::RTSP;
                    else if (type == "http") config.source_type = SourceType::HTTP;
                    
                    config.source_location = input["location"].as<std::string>();
                }
                
                // İşleme ayarları
                if (pipeline["processing"]) {
                    auto processing = pipeline["processing"];
                    config.enable_motion_detection = processing["motion_detection"].as<bool>(false);
                    config.enable_gpu_acceleration = processing["gpu_acceleration"].as<bool>(false);
                }
                
                // Çıkış ayarları
                if (pipeline["output"]) {
                    auto output = pipeline["output"];
                    std::string type = output["type"].as<std::string>();
                    
                    if (type == "display") config.sink_type = SinkType::DISPLAY;
                    else if (type == "file") config.sink_type = SinkType::FILE;
                    else if (type == "rtsp") config.sink_type = SinkType::RTSP;
                    
                    config.sink_location = output["location"].as<std::string>("");
                }
            }
        }
        catch (const YAML::Exception& e) {
            std::cerr << "[HATA] Config dosyası okunamadı: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Varsayılan değerler
    if (config.source_location.empty()) {
        std::cerr << "[HATA] Video kaynağı belirtilmedi!" << std::endl;
        showUsage(argv[0]);
        return false;
    }
    
    // Debug çıktısı
    if (verbose) {
        std::cout << "\n[CONFIG] Pipeline Yapılandırması:" << std::endl;
        std::cout << "  Kaynak: " << config.source_location << std::endl;
        std::cout << "  Çözünürlük: " << config.width << "x" << config.height << "@" << config.framerate << "fps" << std::endl;
        std::cout << "  Bit hızı: " << config.bitrate << " bps" << std::endl;
        std::cout << "  GPU Hızlandırma: " << (config.enable_gpu_acceleration ? "Evet" : "Hayır") << std::endl;
        std::cout << "  Hareket Algılama: " << (config.enable_motion_detection ? "Evet" : "Hayır") << std::endl;
        
        if (config.sink_type == SinkType::RTSP) {
            std::cout << "  RTSP Çıkış: " << config.sink_location << std::endl;
        }
        
        if (config.enable_recording) {
            std::cout << "  Kayıt: " << config.record_location << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    return true;
}

/**
 * @brief Performans bilgilerini gösterir
 * @param pipeline Pipeline yöneticisi
 */
void showPerformanceInfo(PipelineManager* pipeline) {
    // Terminal temizle ve başlık yaz
    std::cout << "\033[2J\033[H"; // Clear screen
    std::cout << "=== GStreamer Video Analytics Pipeline ===" << std::endl;
    std::cout << "Çıkmak için Ctrl+C" << std::endl;
    std::cout << std::string(42, '-') << std::endl;
    
    // FPS bilgisi
    std::cout << "FPS: " << std::fixed << std::setprecision(2) 
              << pipeline->getCurrentFPS() << std::endl;
    
    // Video işleme istatistikleri
    if (auto* processor = pipeline->getVideoProcessor()) {
        auto stats = processor->getStats();
        std::cout << "İşlenen Kareler: " << stats.frames_processed << std::endl;
        std::cout << "Ortalama İşleme Süresi: " << stats.avg_processing_time << " ms" << std::endl;
    }
    
    // Hareket algılama istatistikleri
    if (auto* detector = pipeline->getMotionDetector()) {
        auto stats = detector->getStats();
        std::cout << "\nHareket Algılama:" << std::endl;
        std::cout << "  Hareket Kareleri: " << stats.motion_frames 
                  << " / " << stats.total_frames << std::endl;
        std::cout << "  Ortalama Hareket Alanı: " << std::fixed << std::setprecision(2)
                  << stats.average_motion_area << "%" << std::endl;
        
        // Aktif hareket bölgeleri
        auto motions = detector->getCurrentMotions();
        if (!motions.empty()) {
            std::cout << "  Aktif Bölgeler: " << motions.size() << std::endl;
        }
    }
    
    // RTSP server istatistikleri
    if (auto* streamer = pipeline->getRTSPStreamer()) {
        auto stats = streamer->getStats();
        std::cout << "\nRTSP Server:" << std::endl;
        std::cout << "  URL: " << streamer->getStreamURL() << std::endl;
        std::cout << "  Aktif İstemciler: " << stats.active_clients << std::endl;
        std::cout << "  Toplam Bant Genişliği: " << std::fixed << std::setprecision(2)
                  << stats.average_bandwidth << " Mbps" << std::endl;
    }
}

/**
 * @brief Ana fonksiyon
 */
int main(int argc, char* argv[]) {
    // GStreamer'ı başlat
    gst_init(&argc, &argv);
    
    // Sinyal işleyicileri kur
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Başlangıç mesajı
    std::cout << "GStreamer Video Analytics Pipeline v1.0.0" << std::endl;
    std::cout << "GStreamer " << gst_version_string() << std::endl;
    std::cout << std::string(42, '-') << std::endl;
    
    // Yapılandırmayı oku
    PipelineConfig config;
    if (!parseArguments(argc, argv, config)) {
        return 1;
    }
    
    try {
        // Pipeline yöneticisini oluştur
        g_pipeline_manager = std::make_unique<PipelineManager>(config);
        
        // Hata callback'i ayarla
        g_pipeline_manager->setErrorCallback([](const std::string& error) {
            std::cerr << "\n[HATA] " << error << std::endl;
        });
        
        // EOS callback'i ayarla
        g_pipeline_manager->setEOSCallback([]() {
            std::cout << "\n[INFO] Video sonu algılandı." << std::endl;
            g_pipeline_manager->stop();
        });
        
        // Hareket algılama callback'i
        if (config.enable_motion_detection && g_pipeline_manager->getMotionDetector()) {
            g_pipeline_manager->getMotionDetector()->setMotionEventCallback(
                [](const std::vector<MotionRegion>& regions, guint64 timestamp, double percentage) {
                    if (percentage > 5.0) { // %5'ten fazla hareket
                        std::cout << "\n[HAREKET] " << regions.size() 
                                  << " bölgede hareket algılandı ("
                                  << std::fixed << std::setprecision(1) << percentage 
                                  << "% alan)" << std::endl;
                    }
                }
            );
        }
        
        // Pipeline'ı başlat
        std::cout << "[INFO] Pipeline başlatılıyor..." << std::endl;
        if (!g_pipeline_manager->start()) {
            std::cerr << "[HATA] Pipeline başlatılamadı!" << std::endl;
            return 1;
        }
        
        std::cout << "[INFO] Pipeline başarıyla başlatıldı." << std::endl;
        
        // Ana döngü - performans bilgilerini göster
        while (g_pipeline_manager->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            showPerformanceInfo(g_pipeline_manager.get());
        }
        
    }
    catch (const std::exception& e) {
        std::cerr << "[HATA] İstisna: " << e.what() << std::endl;
        return 1;
    }
    
    // Temizlik
    std::cout << "\n[INFO] Program sonlandırılıyor..." << std::endl;
    g_pipeline_manager.reset();
    
    return 0;
}