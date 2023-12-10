# ############################################################################
# UNIT TESTS SETTINGS
# ############################################################################

# add the unit test executable
file(GLOB_RECURSE UNIT_TEST_FILES_ESP32 ${ESP32_SOURCE_DIRECTORY}/*.c*
                                        ${ESP32_SOURCE_DIRECTORY}/*.h
)

############################################################################# 
# INCLUDE DIRECTORIES
#############################################################################
# System Directories
include_directories(${ESP32_SOURCE_DIRECTORY}/)
include_directories(${ESP32_SOURCE_DIRECTORY}/Tests/Unit_tests/)
include_directories(${ESP32_SOURCE_DIRECTORY}/Tests/Unit_tests/Fake)

# For Debug Purpose
message(STATUS "UNIT TEST FILES -> ")

# foreach(file ${UNIT_TEST_FILES_ESP32})
#     message(STATUS ${file})
# endforeach()

add_executable(unit_tests_esp32 ${UNIT_TEST_FILES_ESP32})

# link the test executable with the GoogleTest library and your project library
target_link_libraries(unit_tests_esp32  gtest 
                                        gtest_main 
                                        esp-idf-lib
                                        Embedded_System_Library 
)

# add the test to CTest
include(GoogleTest)
gtest_discover_tests(unit_tests_esp32)

# enable testing with CTest
enable_testing()

# Specify the output directory for makefile
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
