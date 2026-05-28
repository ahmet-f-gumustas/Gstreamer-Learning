cmake_minimum_required(VERSION 3.18)
project(jetson_edge_pipeline LANGUAGES CXX CUDA VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ─── Architecture detection ──────────────────────────────────────────────────
set(IS_JETSON OFF)
if(EXISTS "/proc/device-tree/model")
    file(READ "/proc/device-tree/model" DT_MODEL)
    if(DT_MODEL MATCHES "NVIDIA" OR DT_MODEL MATCHES "Jetson" OR DT_MODEL MATCHES "Orin")
        set(IS_JETSON ON)
        message(STATUS "Build host is a Jetson: ${DT_MODEL}")
    endif()
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(IS_AARCH64 ON)
    message(STATUS "Targeting aarch64")
else()
    set(IS_AARCH64 OFF)
endif()

# ─── Dependencies ────────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0
)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio highgui)
find_package(CUDAToolkit REQUIRED)
find_package(yaml-cpp REQUIRED)

# TensorRT — find heuristically (NV doesn't ship CMake config files)
find_path(TRT_INCLUDE_DIR NvInfer.h
    PATHS /usr/include /usr/include/x86_64-linux-gnu /usr/include/aarch64-linux-gnu
          /usr/local/include /usr/local/tensorrt/include)
find_library(TRT_NVINFER nvinfer
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
          /usr/local/lib /usr/local/tensorrt/lib)
find_library(TRT_NVPARSER nvonnxparser
    PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
          /usr/local/lib /usr/local/tensorrt/lib)

if(NOT TRT_INCLUDE_DIR OR NOT TRT_NVINFER OR NOT TRT_NVPARSER)
    message(FATAL_ERROR
        "TensorRT not found. Set TRT_INCLUDE_DIR/TRT_NVINFER/TRT_NVPARSER manually.")
endif()
message(STATUS "TensorRT: ${TRT_NVINFER}")

# ─── Sources ─────────────────────────────────────────────────────────────────
set(EDGE_SOURCES
    src/main.cpp
    src/edge_pipeline.cpp

    src/camera/camera_factory.cpp
    src/camera/usb_camera.cpp
    src/camera/csi_camera.cpp
    src/camera/gmsl_camera.cpp
    src/camera/gige_camera.cpp

    src/inference/tensorrt_engine.cpp
    src/inference/int8_calibrator.cpp

    src/tracking/kalman_filter.cpp
    src/tracking/hungarian.cpp
    src/tracking/byte_tracker.cpp

    src/monitoring/tegrastats_parser.cpp
    src/monitoring/orin_simulator.cpp
    src/monitoring/perf_logger.cpp
)

add_executable(jetson_edge ${EDGE_SOURCES})

target_include_directories(jetson_edge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GSTREAMER_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${TRT_INCLUDE_DIR}
)

target_link_libraries(jetson_edge PRIVATE
    ${GSTREAMER_LIBRARIES}
    ${OpenCV_LIBS}
    CUDA::cudart
    ${TRT_NVINFER}
    ${TRT_NVPARSER}
    yaml-cpp
    pthread
)

target_compile_options(jetson_edge PRIVATE -Wall -Wextra -Wno-unused-parameter)
if(IS_JETSON)
    target_compile_definitions(jetson_edge PRIVATE EDGE_HOST_IS_JETSON=1)
endif()

# ─── Optional: tests ─────────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ─── Install ─────────────────────────────────────────────────────────────────
install(TARGETS jetson_edge DESTINATION bin)
install(DIRECTORY config DESTINATION share/jetson_edge)
install(DIRECTORY scripts DESTINATION share/jetson_edge)
