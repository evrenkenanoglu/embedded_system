cmake_minimum_required(VERSION 3.10)

project(Embedded_System_Tests)

# Set embedded system directory
set(EMBEDDED_SYSTEM_DIR ${PROJECT_SOURCE_DIR})

# set the C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)

############################################################################# 
# FETCH CONTENTS DEPENDENCIES AND LIBRARY SETTINGS
#############################################################################

# use FetchContent to download and build dependencies
include(FetchContentDependencies.cmake)

############################################################################# 
# EMBEDDED SYSTEM LIBRARY SETTINGS
#############################################################################

include(EmbeddedSystemLibrarySettings.cmake)

############################################################################# 
# UNIT TESTS SETTINGS
#############################################################################

include(UnitTestSettings.cmake)

############################################################################# 
# CPACK CONFIGURATION
#############################################################################

include(CPackConfig.cmake)