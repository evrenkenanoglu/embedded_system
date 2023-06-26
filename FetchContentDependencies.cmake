include(FetchContent)


# Google Test Framework

# Set the runtime library to use
# These lines tell Google Test to use the dynamic release version of the runtime library (/MD) and to match your project's runtime library settings.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(gtest_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MD" FORCE)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0 # Replace with the release tag you want to use
)

FetchContent_MakeAvailable(googletest)