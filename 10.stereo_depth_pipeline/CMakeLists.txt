cmake_minimum_required(VERSION 3.16)
project(stereo-depth-pipeline VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# ── Dependencies ─────────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED
    gstreamer-1.0>=1.16
    gstreamer-app-1.0>=1.16
    gstreamer-video-1.0>=1.16
)

find_package(OpenCV REQUIRED COMPONENTS core imgproc calib3d highgui)
find_package(Threads REQUIRED)

# ── Include directories ──────────────────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GST_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

# ── Source files ─────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/stereo_pipeline.cpp
    src/depth_estimator.cpp
    src/obstacle_detector.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    ${GST_LIBRARIES}
    ${OpenCV_LIBS}
    Threads::Threads
)

target_compile_options(${PROJECT_NAME} PRIVATE ${GST_CFLAGS_OTHER})

# ── Build messages ───────────────────────────────────────────────────────────
message(STATUS "Build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "GStreamer ver : ${GST_gstreamer-1.0_VERSION}")
message(STATUS "OpenCV ver    : ${OpenCV_VERSION}")
