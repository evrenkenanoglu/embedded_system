cmake_minimum_required(VERSION 3.25)

project(Embedded_System_Tests)

# Set Workspace directory for common base for other projects
set(WORKSPACE_DIR "${PROJECT_SOURCE_DIR}/..")

set(CMAKE_LIB_DIR "CMake")

# Set embedded system directory
set(EMBEDDED_SYSTEM_DIR ${PROJECT_SOURCE_DIR})

# set the C++ standard to C++11
set(CMAKE_CXX_STANDARD 11)

# ############################################################################
# FETCH CONTENTS DEPENDENCIES AND LIBRARY SETTINGS
# ############################################################################

# use FetchContent and Download content and build dependencies
include(${CMAKE_LIB_DIR}/FetchContentDependencies.cmake)

# ############################################################################
# EMBEDDED SYSTEM LIBRARY SETTINGS
# ############################################################################
include(${CMAKE_LIB_DIR}/EmbeddedSystemLibrarySettings.cmake)

# ############################################################################
# UNIT TESTS SETTINGS
# ############################################################################
include(${CMAKE_LIB_DIR}/UnitTestSettings.cmake)

# ############################################################################
# CREATING CLANG-FORMAT TARGETS
# ############################################################################
include(${CMAKE_LIB_DIR}/ClangFormat.cmake)
# To be able to use those clang-format application must be installed
createClangFormatCheckTarget("format_check_all" "System")
createClangFormatApplyTarget("format_apply_all" "System")

# ############################################################################
# CREATING CLANG-TIDY TARGETS
# ############################################################################
include(${CMAKE_LIB_DIR}/ClangTidy.cmake)
createClangTidyCheckTarget("tidy_check_all" "System")
