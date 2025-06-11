cmake_minimum_required(VERSION 3.20)
project(deepdetect-plugin VERSION 1.0.0 LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(CUDA REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=1.26)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0>=1.26)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.26)
pkg_check_modules(GSTREAMER_CUDA REQUIRED gstreamer-cuda-1.0>=1.26)

# Find TensorRT
find_path(TENSORRT_INCLUDE_DIR NvInfer.h
    HINTS ${TENSORRT_ROOT} ${CUDA_TOOLKIT_ROOT_DIR}
    PATH_SUFFIXES include)
find_library(TENSORRT_LIBRARY nvinfer
    HINTS ${TENSORRT_ROOT} ${CUDA_TOOLKIT_ROOT_DIR}
    PATH_SUFFIXES lib lib64 lib/x64)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_BASE_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    ${TENSORRT_INCLUDE_DIR}
    ${CUDA_INCLUDE_DIRS}
)

# Plugin library
add_library(gstdeepdetect SHARED
    src/gstdeepdetect.cc
    src/yolo_engine.cc
)

target_link_libraries(gstdeepdetect
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${TENSORRT_LIBRARY}
    ${CUDA_LIBRARIES}
    cudart
    nvbufsurface
    nvbufsurftransform
)

# Set plugin properties
set_target_properties(gstdeepdetect PROPERTIES
    PREFIX ""
    SUFFIX ".so"
)

# Install
install(TARGETS gstdeepdetect
    LIBRARY DESTINATION lib/gstreamer-1.0
)

# Tests
enable_testing()
find_package(CppUTest QUIET)
if(CppUTest_FOUND)
    add_subdirectory(tests)
endif()