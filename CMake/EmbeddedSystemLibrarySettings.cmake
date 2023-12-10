############################################################################# 
# EMBEDDED SYSTEM LIBRARY SETTINGS
#############################################################################

# add the source files for your project
file(GLOB_RECURSE SRC_FILES ${EMBEDDED_SYSTEM_SOURCE_DIR}/Library/*.c*
                            ${EMBEDDED_SYSTEM_SOURCE_DIR}/Library/*.h
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/HAL/*.c*
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/HAL/*.h
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/Process/*.c* 
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/Process/*.h
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/System/*.c* 
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/System/*.h
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/Tests/Unit_Tests/*.c* 
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/Tests/Unit_Tests/*.h
                            )


message(STATUS "EMBEDDED SYSTEM FILES -> ")
foreach(file ${SRC_FILES})
message(STATUS ${file})
endforeach()

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
include_directories(${EMBEDDED_SYSTEM_SOURCE_DIR}/)
include_directories(${EMBEDDED_SYSTEM_SOURCE_DIR}/HAL)
include_directories(${EMBEDDED_SYSTEM_SOURCE_DIR}/Library)
include_directories(${EMBEDDED_SYSTEM_SOURCE_DIR}/Process)
include_directories(${EMBEDDED_SYSTEM_SOURCE_DIR}/System)
include_directories(${EMBEDDED_SYSTEM_SOURCE_DIR}/Tests/Unit_Tests/)
include_directories(${EMBEDDED_SYSTEM_SOURCE_DIR}/Tests/Unit_Tests/Fake)


# add the source files for your project
file(GLOB_RECURSE SRC_FILES_ESP32 
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/Library/*.c*
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/Library/*.h
                            ${EMBEDDED_SYSTEM_SOURCE_DIR}/HAL/*.c*
                            ${EMBEDDED_SYSTEM_SOURCE_DIR}/HAL/*.h
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/Process/*.c* 
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/Process/*.h
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/System/*.c* 
                            # ${EMBEDDED_SYSTEM_SOURCE_DIR}/System/*.h
                            )