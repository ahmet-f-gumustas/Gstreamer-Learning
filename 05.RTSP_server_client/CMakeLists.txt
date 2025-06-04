cmake_minimum_required(VERSION 3.10)
project(RTSPCameraStreaming)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find GStreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0>=1.16)
pkg_check_modules(GSTREAMER_RTSP_SERVER REQUIRED gstreamer-rtsp-server-1.0>=1.16)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0>=1.16)

# Include directories
include_directories(
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_RTSP_SERVER_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${GSTREAMER_LIBRARY_DIRS}
    ${GSTREAMER_RTSP_SERVER_LIBRARY_DIRS}
    ${GSTREAMER_VIDEO_LIBRARY_DIRS}
)

# Server executable
add_executable(rtsp_cam_server rtsp_cam_server.cpp)
target_link_libraries(rtsp_cam_server
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_RTSP_SERVER_LIBRARIES}
    pthread
)

# Client executable
add_executable(rtsp_cam_client rtsp_cam_client.cpp)
target_link_libraries(rtsp_cam_client
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    pthread
)

# Set compiler flags
add_compile_options(-Wall -Wextra -O2)

# Install targets
install(TARGETS rtsp_cam_server rtsp_cam_client
    RUNTIME DESTINATION bin
)

# Create output directory for reports
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/reports)