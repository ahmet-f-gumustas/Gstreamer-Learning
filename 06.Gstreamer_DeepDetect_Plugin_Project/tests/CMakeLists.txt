# Tests CMakeLists.txt

find_package(CppUTest REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CPPUTEST_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_BASE_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
)

# Test executable
add_executable(test_synthetic
    test_synthetic.cpp
)

target_link_libraries(test_synthetic
    ${CPPUTEST_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    gstdeepdetect
)

# Add test to CTest
add_test(NAME synthetic_tests COMMAND test_synthetic)

# Copy golden reference file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/golden_meta.json
    ${CMAKE_CURRENT_BINARY_DIR}/golden_meta.json
    COPYONLY
)

# Integration test script
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/integration_tests.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/integration_tests.sh
    @ONLY
)

# Make integration test script executable
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/integration_tests.sh
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ WORLD_EXECUTE)

# Add integration test
add_test(NAME integration_tests 
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/integration_tests.sh)

# Memory leak test with valgrind (if available)
find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
    add_test(NAME memory_leak_test
             COMMAND ${VALGRIND_EXECUTABLE} 
                     --tool=memcheck 
                     --leak-check=full 
                     --error-exitcode=1
                     ./test_synthetic)
endif()

# Performance test
add_test(NAME performance_test
         COMMAND ${CMAKE_CURRENT_BINARY_DIR}/performance_test.sh)