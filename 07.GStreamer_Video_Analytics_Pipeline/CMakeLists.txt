# GStreamer Video Analytics Pipeline CMake File
# Minimum CMake version
cmake_minimum_required(VERSION 3.22)

# Project definition
project(gstreamer_video_analytics
    VERSION 1.0.0
    DESCRIPTION "GStreamer-based video analytics application"
    LANGUAGES CXX C)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default build type is Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Load PkgConfig module
find_package(PkgConfig REQUIRED)

# Find GStreamer components
pkg_check_modules(GSTREAMER REQUIRED
    gstreamer-1.0>=1.20
    gstreamer-base-1.0>=1.20
    gstreamer-app-1.0>=1.20
    gstreamer-video-1.0>=1.20
    gstreamer-pbutils-1.0>=1.20
    gstreamer-rtsp-server-1.0>=1.20
)

# Find OpenCV (optional)
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    add_definitions(-DHAVE_OPENCV)
else()
    message(STATUS "OpenCV not found, motion detection disabled")
endif()

# Find YAML-CPP
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)

# Thread library
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GSTREAMER_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIRS}
)

# OpenCV include directories (if found)
if(OpenCV_FOUND)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/video_processor.cpp
    src/rtsp_streamer.cpp
    src/motion_detector.cpp
    src/pipeline_manager.cpp
)

# Header files
set(HEADERS
    include/video_processor.h
    include/rtsp_streamer.h
    include/motion_detector.h
    include/pipeline_manager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${GSTREAMER_LIBRARIES}
    ${YAML_CPP_LIBRARIES}
    Threads::Threads
)

# Link OpenCV libraries (if found)
if(OpenCV_FOUND)
    target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
endif()

# Add compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    ${GSTREAMER_CFLAGS_OTHER}
    ${YAML_CPP_CFLAGS_OTHER}
)

# Install targets
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install config files
install(DIRECTORY config/
    DESTINATION share/${PROJECT_NAME}/config
)

# Install test video files (optional)
install(DIRECTORY assets/
    DESTINATION share/${PROJECT_NAME}/assets
    OPTIONAL
)

# Custom targets
# clang-format target for code formatting
add_custom_target(format
    COMMAND clang-format -i ${SOURCES} ${HEADERS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running code formatting..."
)

# Documentation generation (Doxygen)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating documentation with Doxygen..."
        VERBATIM
    )
endif()

# Debug information
message(STATUS "=== Build Configuration ===")
message(STATUS "CMake Version: ${CMAKE_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "GStreamer Version: ${GSTREAMER_VERSION}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===============================")

# CPack configuration (for packaging)
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "GStreamer Learning Project")
set(CPACK_PACKAGE_CONTACT "developer@example.com")
set(CPACK_GENERATOR "DEB;TGZ")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgstreamer1.0-0 (>= 1.20), libgstreamer-plugins-base1.0-0 (>= 1.20)")

include(CPack)
