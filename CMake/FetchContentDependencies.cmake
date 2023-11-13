include(FetchContent)

# Set Dependency Directory as a common base
set(DEPENDENCIES_DIR "${WORKSPACE_DIR}/_deps")

# #######################################################################################
# Google Test Framework
# #######################################################################################
# Set the runtime library to use
# These lines tell Google Test to use the dynamic release version of the runtime library (/MD) and to match your project's runtime library settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(gtest_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MD" FORCE)

# Set gtest git tag as a variable
set(GTEST_GIT_TAG release-1.10.0) # Replace with the release tag you want to use

# Set the source and build directories
set(GTEST_SOURCE_DIR "${DEPENDENCIES_DIR}/googletest-${GTEST_GIT_TAG}-src")
set(GTEST_BINARY_DIR "${DEPENDENCIES_DIR}/googletest-${GTEST_GIT_TAG}-build")
set(GTEST_SUBBUILD_DIR "${DEPENDENCIES_DIR}/googletest-${GTEST_GIT_TAG}-subbuild")

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG ${GTEST_GIT_TAG}
  SOURCE_DIR ${GTEST_SOURCE_DIR}
  BINARY_DIR ${GTEST_BINARY_DIR}
  SUBBUILD_DIR ${GTEST_SUBBUILD_DIR}
)
FetchContent_MakeAvailable(googletest)
