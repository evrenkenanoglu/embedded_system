cmake_minimum_required(VERSION 3.10)

project(Embedded_System_Tests)

# Set embedded system directory
set(EMBEDDED_SYSTEM_DIR ${PROJECT_SOURCE_DIR})

# set the C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)

############################################################################# 
# GOOGLE TEST LIBRARY SETTINGS
#############################################################################

# Set the runtime library to use
# These lines tell Google Test to use the dynamic release version of the runtime library (/MD) and to match your project's runtime library settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(gtest_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MD" FORCE)

# use FetchContent to download and build googletest
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0 # Replace with the release tag you want to use
)

FetchContent_MakeAvailable(googletest)

############################################################################# 
# EMBEDDED SYSTEM LIBRARY SETTINGS
#############################################################################

# add the source files for your project
file(GLOB_RECURSE SRC_FILES ${EMBEDDED_SYSTEM_DIR}/Library/*.c*
                            ${EMBEDDED_SYSTEM_DIR}/Library/*.h*
                            # ${EMBEDDED_SYSTEM_DIR}/HAL/*.c*
                            # ${EMBEDDED_SYSTEM_DIR}/HAL/*.h*
                            # ${EMBEDDED_SYSTEM_DIR}/Process/*.c* 
                            # ${EMBEDDED_SYSTEM_DIR}/Process/*.h*
                            # ${EMBEDDED_SYSTEM_DIR}/System/*.c* 
                            # ${EMBEDDED_SYSTEM_DIR}/System/*.h*
                            )

add_library(Embedded_System_Library STATIC ${SRC_FILES})

set_target_properties(Embedded_System_Library PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME Embedded_System_Library
    DEBUG_POSTFIX d
    RELEASE_POSTFIX ""
)


############################################################################# 
# INCLUDE DIRECTORIES
#############################################################################
include_directories(${EMBEDDED_SYSTEM_DIR}/)
include_directories(${EMBEDDED_SYSTEM_DIR}/HAL)
include_directories(${EMBEDDED_SYSTEM_DIR}/Library)
include_directories(${EMBEDDED_SYSTEM_DIR}/Process)
include_directories(${EMBEDDED_SYSTEM_DIR}/System)


############################################################################# 
# UNIT TESTS SETTINGS
#############################################################################

# add the unit test executable
file(GLOB_RECURSE UNIT_TEST_FILES   ${EMBEDDED_SYSTEM_DIR}/Tests/Unit_Test/*.cpp
                                    ${EMBEDDED_SYSTEM_DIR}/Tests/Unit_Test/Support/*.cpp  )
add_executable(unit_tests ${UNIT_TEST_FILES})

# link the test executable with the GoogleTest library and your project library
target_link_libraries(unit_tests gtest gtest_main Embedded_System_Library)

# add the test to CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)

# enable testing with CTest
enable_testing()

# # add a custom command to generate mock classes and functions
# add_custom_command(
# OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mocks.h
# COMMAND python ${GTEST_DIR}/googlemock/scripts/generator/gmock_gen.py --output-dir=${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/Mock/myMock.h
# DEPENDS ${PROJECT_SOURCE_DIR}/Mock/myMock.h
# )

# # add the generated mocks.h file to the include directories for the project
# target_include_directories(Embedded_System_Library PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# # add the generated mocks.h file to the unit test executable
# target_sources(unit_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/mocks.h)

# Specify the output directory for makefile
set(CMAKE_BINARY_DIR ${EMBEDDED_SYSTEM_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
