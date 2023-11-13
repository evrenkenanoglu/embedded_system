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
