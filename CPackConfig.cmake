############################################################################# 
# CPACK CONFIGURATION
#############################################################################

include(InstallRequiredSystemLibraries)
# enable CPack
include(CPack)

# set the package name and version
set(CPACK_PACKAGE_NAME "Embedded_System_Library")
set(CPACK_PACKAGE_VERSION "1.0.0")

# # set the package description
# set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library for Embedded Systems")

# # set the package maintainer
# set(CPACK_PACKAGE_CONTACT "Evren Kenanoglu evren.kenanoglu@gmail.com")

# # set the package architecture
# set(CPACK_PACKAGE_ARCHITECTURE "x86_64")

# # set the package dependencies (if any)
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 5.2)")

# # copy header files to include directory
# install(DIRECTORY ${EMBEDDED_SYSTEM_DIR}/Library/
#         DESTINATION include
#         FILES_MATCHING PATTERN "*.h")

# set the package generator
set(CPACK_GENERATOR "ZIP")

# set the package options
set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

# set the package output directory
set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/packages)

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

# include the CPack configuration
include(CPack)



# # Generate the package
# set(CPACK_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/CPackConfig.cmake")


# # enable CPack
# include(CPack)

# # set the package name and version
# set(CPACK_PACKAGE_NAME "Embedded_System_Library")
# set(CPACK_PACKAGE_VERSION "1.0.0")

# # set the package description
# set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Library for Embedded Systems")

# # set the package maintainer
# set(CPACK_PACKAGE_CONTACT "Evren Kenanoglu evren.kenanoglu@gmail.com")

# # set the package architecture
# set(CPACK_PACKAGE_ARCHITECTURE "x86_64")

# # set the package dependencies (if any)
# set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>= 5.2)")

# # # set the package files to include
# # install(TARGETS Embedded_System_Library
# #         EXPORT Embedded_System_LibraryTargets
# #         LIBRARY DESTINATION lib
# #         ARCHIVE DESTINATION lib
# #         RUNTIME DESTINATION bin
# #         INCLUDES DESTINATION include
# #         )

# install(DIRECTORY ${EMBEDDED_SYSTEM_DIR}/Library/
#         DESTINATION include
#         FILES_MATCHING PATTERN "*.h")

# # set the package generator
# set(CPACK_GENERATOR "ZIP")

# # set the package options
# set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
# set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

# # set the package output directory
# set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/packages)

# set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

# # include the CPack configuration
# include(CPack)
