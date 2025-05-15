cmake_minimum_required(VERSION 3.10)
project(gstreamer-cpp-player VERSION 1.0)

# C++ standartını ayarla
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GStreamer paketlerini bul
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-video-1.0)

# Include dizinlerini ayarla
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GSTREAMER_INCLUDE_DIRS}
)

# Kaynak dosyalarını tanımla
set(SOURCES
    src/main.cpp
    src/MediaPlayer.cpp
    src/Utils.cpp
)

# Executable oluştur
add_executable(media-player ${SOURCES})

# Kütüphaneleri bağla
target_link_libraries(media-player
    ${GSTREAMER_LIBRARIES}
)

# Install hedefi
install(TARGETS media-player DESTINATION bin)