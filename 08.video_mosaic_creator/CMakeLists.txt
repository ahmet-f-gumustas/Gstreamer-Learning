cmake_minimum_required(VERSION 3.16)
project(video-mosaic-creator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED 
    gstreamer-1.0>=1.14
    gstreamer-video-1.0>=1.14
    gstreamer-app-1.0>=1.14
)

find_package(OpenCV REQUIRED)
find_package(yaml-cpp REQUIRED)

# Include directories
include_directories(
    ${GST_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Source files
set(SOURCES
    src/main.cpp
    src/video_mosaic.cpp
    src/input_manager.cpp
    src/mosaic_layout.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${GST_LIBRARIES}
    ${OpenCV_LIBS}
    yaml-cpp
    pthread
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE ${GST_CFLAGS_OTHER})