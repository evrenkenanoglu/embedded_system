# #######################################################################################
# CREATING CLANG-TIDY TARGETS
# #######################################################################################
set(CLANG_TIDY_PATH "")

function(createClangTidyTargets targetName subdirectory checkonly)
    # Find the program
    find_program(CLANG_TIDY "clang-tidy")
    message(STATUS "${CLANG_TIDY}")
    file(GLOB_RECURSE FILES_TO_TIDY RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${subdirectory}/*.cpp ${subdirectory}/*.hpp ${subdirectory}/*.h)

    if(${checkonly})
        set(TIDY_EXPORT_PARAMS
            --export-fixes=results.json
        )
        # Define the TIDYting check and apply parameters as variables
        message(STATUS "${TIDY_CONFIG}")
        message(STATUS "${FILES_TO_TIDY}")
        message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}")
        add_custom_target(${targetName}
            COMMAND ${CLANG_TIDY}
            ${TIDY_EXPORT_PARAMS}
            ${FILES_TO_TIDY}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

    else() # Not used yet
        set(TIDY_FIX_PARAMS
            --export-fixes=fixes.yaml
            --fix)

        # Define a custom target for applying the TIDYting to all source files
        message(STATUS "${TIDY_FIX_PARAMS}")
        message(STATUS "${FILES_TO_TIDY}")
        message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}")
        add_custom_target(${targetName}
            COMMAND ${CLANG_TIDY}
            ${TIDY_FIX_PARAMS}
            ${FILES_TO_TIDY}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endfunction()

function(createClangTidyFixTarget targetName subdirectory)
    createClangTidyTargets(${targetName} ${subdirectory} FALSE)
endfunction()

function(createClangTidyCheckTarget targetName subdirectory)
    createClangTidyTargets(${targetName} ${subdirectory} TRUE)
endfunction()
