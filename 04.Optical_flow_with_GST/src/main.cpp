#include "OpticalFlowDetector.hpp"
#include "Utils.hpp"
#include <iostream>
#include <signal.h>
#include <memory>

// Global detector pointer for signal handling
std::unique_ptr<OpticalFlowDetector> g_detector;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        Utils::logInfo("Çıkış sinyali alındı, program sonlandırılıyor...");
        if (g_detector) {
            g_detector->stop();
        }
        Utils::cleanupGStreamer();
        exit(0);
    }
}

void printUsage(const char* programName) {
    std::cout << "Kullanım: " << programName << " [seçenekler]" << std::endl;
    std::cout << "Seçenekler:" << std::endl;
    std::cout << "  -h, --help          Bu yardım mesajını göster" << std::endl;
    std::cout << "  -f, --file <path>   Video dosyası kullan" << std::endl;
    std::cout << "  -w, --webcam        Webcam kullan (varsayılan)" << std::endl;
    std::cout << "  -t, --test          Test pattern kullan" << std::endl;
    std::cout << std::endl;
    std::cout << "Örnekler:" << std::endl;
    std::cout << "  " << programName << "                    # Webcam kullan" << std::endl;
    std::cout << "  " << programName << " -t                # Test pattern kullan" << std::endl;
    std::cout << "  " << programName << " -f video.mp4      # Video dosyası kullan" << std::endl;
}

int main(int argc, char* argv[]) {
    // Signal handler kurulumu
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // GStreamer başlat
    if (!Utils::initializeGStreamer(argc, argv)) {
        Utils::logError("GStreamer başlatılamadı!");
        return -1;
    }
    
    // Command line argümanlarını parse et
    std::string videoSource = "";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                videoSource = argv[++i];
                Utils::logInfo("Video dosyası kullanılacak: " + videoSource);
            } else {
                Utils::logError("Dosya yolu belirtilmedi!");
                printUsage(argv[0]);
                return -1;
            }
        } else if (arg == "-w" || arg == "--webcam") {
            videoSource = "webcam";
            Utils::logInfo("Webcam kullanılacak");
        } else if (arg == "-t" || arg == "--test") {
            videoSource = "test";
            Utils::logInfo("Test pattern kullanılacak");
        } else {
            Utils::logWarning("Bilinmeyen argüman: " + arg);
        }
    }
    
    // Optical Flow Detector oluştur
    g_detector = std::make_unique<OpticalFlowDetector>();
    
    if (!g_detector->initialize(videoSource)) {
        Utils::logError("OpticalFlowDetector başlatılamadı!");
        Utils::cleanupGStreamer();
        return -1;
    }
    
    Utils::logInfo("=== GStreamer Optical Flow Detector ===");
    Utils::logInfo("Kontroller:");
    Utils::logInfo("  'q' tuşu: Çıkış");
    Utils::logInfo("  Ctrl+C: Programı sonlandır");
    Utils::logInfo("========================================");
    
    try {
        // Ana döngüyü başlat
        g_detector->run();
    } catch (const std::exception& e) {
        Utils::logError("Hata oluştu: " + std::string(e.what()));
    }
    
    // Temizlik
    g_detector->stop();
    g_detector.reset();
    Utils::cleanupGStreamer();
    
    Utils::logInfo("Program başarıyla sonlandırıldı");
    return 0;
}