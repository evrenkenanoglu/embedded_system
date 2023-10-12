cmake_minimum_required(VERSION 3.25)

project(Embedded_System_Tests)

# Set Workspace directory for common base for other projects
set(WORKSPACE_DIR "${PROJECT_SOURCE_DIR}/..")

# Set embedded system directory
set(EMBEDDED_SYSTEM_DIR ${PROJECT_SOURCE_DIR})

# set the C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)

############################################################################# 
# FETCH CONTENTS DEPENDENCIES AND LIBRARY SETTINGS
#############################################################################

# use FetchContent and Download content and build dependencies
include(FetchContentDependencies.cmake)

############################################################################# 
# EMBEDDED SYSTEM LIBRARY SETTINGS
#############################################################################

include(EmbeddedSystemLibrarySettings.cmake)

############################################################################# 
# UNIT TESTS SETTINGS
#############################################################################

include(UnitTestSettings.cmake)