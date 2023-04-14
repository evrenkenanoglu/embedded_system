cmake_minimum_required(VERSION 3.10)

project(Embedded_System_Tests)

# set the C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)

############################################################################# 
# GOOGLE TEST LIBRARY SETTINGS
#############################################################################

# set the path to the location of the GoogleTest library
set(GTEST_DIR "C:/Workspace_Personal/googletest")

# Set the runtime library to use
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# add the GoogleTest library
add_subdirectory(${GTEST_DIR} gtest)


############################################################################# 
# EMBEDDED SYSTEM LIBRARY SETTINGS
#############################################################################

# add the source files for your project
file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/../Library/*.c*
                            ${PROJECT_SOURCE_DIR}/../Library/*.h*
                            # ${PROJECT_SOURCE_DIR}/../HAL/*.c*
                            # ${PROJECT_SOURCE_DIR}/../HAL/*.h*
                            # ${PROJECT_SOURCE_DIR}/../Process/*.c* 
                            # ${PROJECT_SOURCE_DIR}/../Process/*.h*
                            # ${PROJECT_SOURCE_DIR}/../System/*.c* 
                            # ${PROJECT_SOURCE_DIR}/../System/*.h*
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
# INCLUDE DIRECTORY
#############################################################################
include_directories(${PROJECT_SOURCE_DIR}/../)
include_directories(${PROJECT_SOURCE_DIR}/../HAL)
include_directories(${PROJECT_SOURCE_DIR}/../Library)
include_directories(${PROJECT_SOURCE_DIR}/../Process)
include_directories(${PROJECT_SOURCE_DIR}/../System)


############################################################################# 
# UNIT TESTS SETTINGS
#############################################################################

# add the unit test executable
file(GLOB_RECURSE UNIT_TEST_FILES   ${PROJECT_SOURCE_DIR}/../Tests/Unit_Test/*.cpp
                                    ${PROJECT_SOURCE_DIR}/../Tests/Unit_Test/Support/*.cpp  )
add_executable(unit_tests ${UNIT_TEST_FILES})

# link the test executable with the GoogleTest library and your project library
target_link_libraries(unit_tests gtest_main Embedded_System_Library)

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
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/../build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
