#include "MediaPlayer.hpp"
#include "Utils.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// Show main menu
void showMenu() {
    std::cout << "\n--- GStreamer C++ Media Player ---\n";
    std::cout << "1. Open File\n";
    std::cout << "2. Play\n";
    std::cout << "3. Pause\n";
    std::cout << "4. Stop\n";
    std::cout << "5. Skip 10 seconds forward\n";
    std::cout << "6. Skip 10 seconds backward\n";
    std::cout << "7. Seek to a specific position\n";
    std::cout << "8. Show media information\n";
    std::cout << "9. Toggle position update\n";
    std::cout << "0. Exit\n";
    std::cout << "Your choice: ";
}

// Position update function for a thread
void positionUpdateThread(MediaPlayer& player, bool& running) {
    while (running) {
        if (player.isPlaying()) {
            gint64 position = player.getPosition();
            gint64 duration = player.getDuration();

            if (position >= 0 && duration > 0) {
                std::cout << "\rPosition: " << Utils::formatTime(position)
                          << " / " << Utils::formatTime(duration)
                          << " (%)" << (position * 100 / duration) << ")" << std::flush;
            }
        }

        // Wait 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(int argc, char *argv[]) {
    MediaPlayer player;
    bool running = true;
    bool positionUpdateEnabled = false;
    std::thread updateThread;
    
    std::cout << "Welcome to GStreamer C++ Media Player!\n";

    // Load file from command line argument
    if (argc > 1) {
        std::string filePath = argv[1];
        std::cout << "Opening file: " << filePath << std::endl;

        if (player.openFile(filePath)) {
            std::cout << "File opened successfully.\n";
            std::cout << player.getMediaInfo() << std::endl;

            if (Utils::getYesNoInput("Play now?")) {
                player.play();

                // Start position update thread
                positionUpdateEnabled = true;
                updateThread = std::thread(positionUpdateThread, std::ref(player), std::ref(running));
            }
        } else {
            std::cerr << "Failed to open file!\n";
        }
    }
    
    // Main loop
    int choice = -1;
    while (choice != 0) {
        showMenu();
        std::cin >> choice;

        switch (choice) {
            case 1: {  // Open File
                std::string filePath = Utils::getInput("Enter file path");

                if (player.openFile(filePath)) {
                    std::cout << "File opened successfully.\n";
                    std::cout << player.getMediaInfo() << std::endl;
                } else {
                    std::cerr << "Failed to open file!\n";
                }
                break;
            }
            case 2:  // Play
                if (player.play()) {
                    std::cout << "Playing...\n";

                    if (!positionUpdateEnabled && Utils::getYesNoInput("Start position update?")) {
                        positionUpdateEnabled = true;

                        if (updateThread.joinable()) {
                            updateThread.join();
                        }

                        updateThread = std::thread(positionUpdateThread, std::ref(player), std::ref(running));
                    }
                } else {
                    std::cerr << "Failed to play!\n";
                }
                break;
            case 3:  // Pause
                if (player.pause()) {
                    std::cout << "Paused.\n";
                } else {
                    std::cerr << "Failed to pause!\n";
                }
                break;
            case 4:  // Stop
                if (player.stop()) {
                    std::cout << "Stopped.\n";
                } else {
                    std::cerr << "Failed to stop!\n";
                }
                break;
            case 5:  // Skip 10 seconds forward
                if (player.seekRelative(10 * GST_SECOND)) {
                    std::cout << "Skipped 10 seconds forward.\n";
                } else {
                    std::cerr << "Failed to skip forward!\n";
                }
                break;
            case 6:  // Skip 10 seconds backward
                if (player.seekRelative(-10 * GST_SECOND)) {
                    std::cout << "Skipped 10 seconds backward.\n";
                } else {
                    std::cerr << "Failed to skip backward!\n";
                }
                break;
            case 7: {  // Seek to a specific position
                std::string posStr = Utils::getInput("Enter position in seconds to seek to");
                try {
                    int seconds = std::stoi(posStr);
                    if (player.seek(seconds * GST_SECOND)) {
                        std::cout << "Seeked to " << seconds << " seconds.\n";
                    } else {
                        std::cerr << "Failed to seek!\n";
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Invalid value entered!\n";
                }
                break;
            }
            case 8:  // Show media information
                std::cout << "Media Information:\n";
                std::cout << player.getMediaInfo() << std::endl;
                break;
            case 9:  // Toggle position update
                positionUpdateEnabled = !positionUpdateEnabled;

                if (positionUpdateEnabled) {
                    std::cout << "Position update enabled.\n";

                    if (updateThread.joinable()) {
                        updateThread.join();
                    }

                    updateThread = std::thread(positionUpdateThread, std::ref(player), std::ref(running));
                } else {
                    std::cout << "Position update disabled.\n";
                }
                break;
            case 0:  // Exit
                std::cout << "Exiting program...\n";
                break;
            default:
                std::cerr << "Invalid choice!\n";
                break;
        }
    }

    // Clean up thread
    running = false;
    if (updateThread.joinable()) {
        updateThread.join();
    }
    
    return 0;
}