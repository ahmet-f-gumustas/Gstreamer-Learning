cmake_minimum_required(VERSION 3.10)
project(gstreamer-cpp-player VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GStreamer packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-video-1.0)

# Set include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GSTREAMER_INCLUDE_DIRS}
)

# Define source files
set(SOURCES
    src/main.cpp
    src/MediaPlayer.cpp
    src/Utils.cpp
)

# Create executable
add_executable(media-player ${SOURCES})

# Link libraries
target_link_libraries(media-player
    ${GSTREAMER_LIBRARIES}
)

# Install target
install(TARGETS media-player DESTINATION bin)