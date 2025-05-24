cmake_minimum_required(VERSION 3.16)
project(optical-flow-detector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find GStreamer packages
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${GSTREAMER_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})

# Add compile flags
add_compile_options(${GSTREAMER_CFLAGS})

# Source files
set(SOURCES
    src/main.cpp
    src/OpticalFlowDetector.cpp
    src/Utils.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${OpenCV_LIBS}
)

# Set link directories
link_directories(${GSTREAMER_LIBRARY_DIRS})