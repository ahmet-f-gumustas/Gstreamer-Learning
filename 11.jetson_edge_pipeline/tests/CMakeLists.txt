cmake_minimum_required(VERSION 3.18)

# Hafif assert-tabanlı testler — harici test framework gerekmesin.
# Her test dosyası kendi başına bir executable.

set(TEST_SOURCES
    test_camera_factory.cpp
    test_byte_tracker.cpp
    test_orin_simulator.cpp
    test_hungarian.cpp
    test_kalman.cpp
)

set(PARENT_SOURCES
    ../src/camera/camera_factory.cpp
    ../src/camera/usb_camera.cpp
    ../src/camera/csi_camera.cpp
    ../src/camera/gmsl_camera.cpp
    ../src/camera/gige_camera.cpp
    ../src/tracking/kalman_filter.cpp
    ../src/tracking/hungarian.cpp
    ../src/tracking/byte_tracker.cpp
    ../src/monitoring/orin_simulator.cpp
)

foreach(src ${TEST_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src} ${PARENT_SOURCES})
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${name} PRIVATE
        ${OpenCV_LIBS} CUDA::cudart pthread)
    add_test(NAME ${name} COMMAND ${name})
endforeach()
