cmake_minimum_required(VERSION 3.16)
project(VideoConverter VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

# CUDA support (optional)
find_package(CUDA QUIET)
if(CUDA_FOUND)
    add_definitions(-DHAS_CUDA)
    message(STATUS "CUDA found - GPU acceleration enabled")
else()
    message(STATUS "CUDA not found - using CPU only")
endif()

# Directories
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(BUILD_DIR ${CMAKE_BINARY_DIR})

# Create output directory
file(MAKE_DIRECTORY ${BUILD_DIR}/bin)

# Source files
file(GLOB SOURCES "${SRC_DIR}/*.cpp")

# Create executable
add_executable(video_converter ${SOURCES})

# Set output directory
set_target_properties(video_converter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}/bin
)

# Include directories
target_include_directories(video_converter PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${GSTREAMER_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(video_converter ${GSTREAMER_LIBRARIES})

# Compiler flags
target_compile_options(video_converter PRIVATE 
    ${GSTREAMER_CFLAGS_OTHER}
    -Wall -Wextra -O2
)

# CUDA libraries (if available)
if(CUDA_FOUND)
    target_link_libraries(video_converter ${CUDA_LIBRARIES})
    target_include_directories(video_converter PRIVATE ${CUDA_INCLUDE_DIRS})
endif()

message(STATUS "Build configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output: ${BUILD_DIR}/bin/video_converter")
message(STATUS "  CUDA support: ${CUDA_FOUND}")
message(STATUS "  GStreamer version: ${GSTREAMER_VERSION}")