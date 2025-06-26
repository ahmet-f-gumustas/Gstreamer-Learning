# GStreamer Video Analytics Pipeline CMake Dosyası
# Minimum CMake versiyonu
cmake_minimum_required(VERSION 3.22)

# Proje tanımı
project(gstreamer_video_analytics 
    VERSION 1.0.0
    DESCRIPTION "GStreamer tabanlı video analiz uygulaması"
    LANGUAGES CXX C)

# C++ standardı
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Derleme türü varsayılan olarak Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Derleme bayrakları
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# PkgConfig modülünü yükle
find_package(PkgConfig REQUIRED)

# GStreamer bileşenlerini bul
pkg_check_modules(GSTREAMER REQUIRED 
    gstreamer-1.0>=1.20
    gstreamer-base-1.0>=1.20
    gstreamer-app-1.0>=1.20
    gstreamer-video-1.0>=1.20
    gstreamer-pbutils-1.0>=1.20
    gstreamer-rtsp-server-1.0>=1.20
)

# OpenCV'yi bul (opsiyonel)
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "OpenCV bulundu: ${OpenCV_VERSION}")
    add_definitions(-DHAVE_OPENCV)
else()
    message(STATUS "OpenCV bulunamadı, hareket algılama devre dışı")
endif()

# YAML-CPP'yi bul
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

# Thread kütüphanesi
find_package(Threads REQUIRED)

# Include dizinleri
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GSTREAMER_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIRS}
)

# OpenCV include dizinleri (eğer bulunduysa)
if(OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# Kaynak dosyaları
set(SOURCES
    src/main.cpp
    src/video_processor.cpp
    src/rtsp_streamer.cpp
    src/motion_detector.cpp
    src/pipeline_manager.cpp
)

# Header dosyaları
set(HEADERS
    include/video_processor.h
    include/rtsp_streamer.h
    include/motion_detector.h
    include/pipeline_manager.h
)

# Çalıştırılabilir dosya oluştur
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Kütüphaneleri bağla
target_link_libraries(${PROJECT_NAME}
    ${GSTREAMER_LIBRARIES}
    ${YAML_CPP_LIBRARIES}
    Threads::Threads
)

# OpenCV kütüphanelerini bağla (eğer bulunduysa)
if(OpenCV_FOUND)
    target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
endif()

# Derleme bayraklarını ekle
target_compile_options(${PROJECT_NAME} PRIVATE 
    ${GSTREAMER_CFLAGS_OTHER}
    ${YAML_CPP_CFLAGS_OTHER}
)

# Kurulum hedefleri
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Config dosyalarını kur
install(DIRECTORY config/
    DESTINATION share/${PROJECT_NAME}/config
)

# Test video dosyasını kur (opsiyonel)
install(DIRECTORY assets/
    DESTINATION share/${PROJECT_NAME}/assets
    OPTIONAL
)

# Özel hedefler
# Format kontrolü için clang-format hedefi
add_custom_target(format
    COMMAND clang-format -i ${SOURCES} ${HEADERS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Kod formatlama yapılıyor..."
)

# Dokümantasyon oluşturma (Doxygen)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Doxygen ile dokümantasyon oluşturuluyor..."
        VERBATIM
    )
endif()

# Debug bilgileri
message(STATUS "=== Derleme Yapılandırması ===")
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "GStreamer Version: ${GSTREAMER_VERSION}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===============================")

# CPack yapılandırması (paketleme için)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "GStreamer Learning Project")
set(CPACK_PACKAGE_CONTACT "developer@example.com")
set(CPACK_GENERATOR "DEB;TGZ")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgstreamer1.0-0 (>= 1.20), libgstreamer-plugins-base1.0-0 (>= 1.20)")

include(CPack)