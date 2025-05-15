#include "MediaPlayer.hpp"
#include "Utils.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// Ana menüyü göster
void showMenu() {
    std::cout << "\n--- GStreamer C++ Media Player ---\n";
    std::cout << "1. Dosya Aç\n";
    std::cout << "2. Oynat\n";
    std::cout << "3. Duraklat\n";
    std::cout << "4. Durdur\n";
    std::cout << "5. 10 saniye ileri\n";
    std::cout << "6. 10 saniye geri\n";
    std::cout << "7. Belirli bir konuma atla\n";
    std::cout << "8. Medya bilgilerini göster\n";
    std::cout << "9. Konum bilgisini güncelle\n";
    std::cout << "0. Çıkış\n";
    std::cout << "Seçiminiz: ";
}

// Bir thread için konum güncelleme fonksiyonu
void positionUpdateThread(MediaPlayer& player, bool& running) {
    while (running) {
        if (player.isPlaying()) {
            gint64 position = player.getPosition();
            gint64 duration = player.getDuration();
            
            if (position >= 0 && duration > 0) {
                std::cout << "\rKonum: " << Utils::formatTime(position) 
                          << " / " << Utils::formatTime(duration) 
                          << " (%)" << (position * 100 / duration) << ")" << std::flush;
            }
        }
        
        // 500ms bekle
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(int argc, char *argv[]) {
    MediaPlayer player;
    bool running = true;
    bool positionUpdateEnabled = false;
    std::thread updateThread;
    
    std::cout << "GStreamer C++ Media Player'a Hoş Geldiniz!\n";
    
    // Komut satırı parametresinden dosya yükle
    if (argc > 1) {
        std::string filePath = argv[1];
        std::cout << "Dosya açılıyor: " << filePath << std::endl;
        
        if (player.openFile(filePath)) {
            std::cout << "Dosya başarıyla açıldı.\n";
            std::cout << player.getMediaInfo() << std::endl;
            
            if (Utils::getYesNoInput("Şimdi oynatılsın mı?")) {
                player.play();
                
                // Konum güncelleme thread'ini başlat
                positionUpdateEnabled = true;
                updateThread = std::thread(positionUpdateThread, std::ref(player), std::ref(running));
            }
        } else {
            std::cerr << "Dosya açılamadı!\n";
        }
    }
    
    // Ana döngü
    int choice = -1;
    while (choice != 0) {
        showMenu();
        std::cin >> choice;
        
        switch (choice) {
            case 1: {  // Dosya Aç
                std::string filePath = Utils::getInput("Dosya yolunu girin");
                
                if (player.openFile(filePath)) {
                    std::cout << "Dosya başarıyla açıldı.\n";
                    std::cout << player.getMediaInfo() << std::endl;
                } else {
                    std::cerr << "Dosya açılamadı!\n";
                }
                break;
            }
            case 2:  // Oynat
                if (player.play()) {
                    std::cout << "Oynatılıyor...\n";
                    
                    if (!positionUpdateEnabled && Utils::getYesNoInput("Konum güncellemesi başlatılsın mı?")) {
                        positionUpdateEnabled = true;
                        
                        if (updateThread.joinable()) {
                            updateThread.join();
                        }
                        
                        updateThread = std::thread(positionUpdateThread, std::ref(player), std::ref(running));
                    }
                } else {
                    std::cerr << "Oynatılamadı!\n";
                }
                break;
            case 3:  // Duraklat
                if (player.pause()) {
                    std::cout << "Duraklatıldı.\n";
                } else {
                    std::cerr << "Duraklatılamadı!\n";
                }
                break;
            case 4:  // Durdur
                if (player.stop()) {
                    std::cout << "Durduruldu.\n";
                } else {
                    std::cerr << "Durdurulamadı!\n";
                }
                break;
            case 5:  // 10 saniye ileri
                if (player.seekRelative(10 * GST_SECOND)) {
                    std::cout << "10 saniye ileri atlandı.\n";
                } else {
                    std::cerr << "İleri atlanamadı!\n";
                }
                break;
            case 6:  // 10 saniye geri
                if (player.seekRelative(-10 * GST_SECOND)) {
                    std::cout << "10 saniye geri atlandı.\n";
                } else {
                    std::cerr << "Geri atlanamadı!\n";
                }
                break;
            case 7: {  // Belirli bir konuma atla
                std::string posStr = Utils::getInput("Atlanacak konumu saniye olarak girin");
                try {
                    int seconds = std::stoi(posStr);
                    if (player.seek(seconds * GST_SECOND)) {
                        std::cout << seconds << " saniyeye atlandı.\n";
                    } else {
                        std::cerr << "Atlanamadı!\n";
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Geçersiz değer girdiniz!\n";
                }
                break;
            }
            case 8:  // Medya bilgilerini göster
                std::cout << "Medya Bilgileri:\n";
                std::cout << player.getMediaInfo() << std::endl;
                break;
            case 9:  // Konum bilgisini güncelle
                positionUpdateEnabled = !positionUpdateEnabled;
                
                if (positionUpdateEnabled) {
                    std::cout << "Konum güncellemesi etkinleştirildi.\n";
                    
                    if (updateThread.joinable()) {
                        updateThread.join();
                    }
                    
                    updateThread = std::thread(positionUpdateThread, std::ref(player), std::ref(running));
                } else {
                    std::cout << "Konum güncellemesi devre dışı bırakıldı.\n";
                }
                break;
            case 0:  // Çıkış
                std::cout << "Program sonlandırılıyor...\n";
                break;
            default:
                std::cerr << "Geçersiz seçim!\n";
                break;
        }
    }
    
    // Thread'i temizle
    running = false;
    if (updateThread.joinable()) {
        updateThread.join();
    }
    
    return 0;
}