# #######################################################################################
# CREATING CLANG-FORMAT TARGETS
# #######################################################################################
set(CLANG_FORMAT_PATH "")

function(createClangFormatTargets targetName subdirectory checkonly)
    # Find the program
    find_program(CLANG_FORMAT "clang-format")
    message(STATUS "${CLANG_FORMAT}")
    file(GLOB_RECURSE FILES_TO_FORMAT RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${subdirectory}/*.cpp ${subdirectory}/*.hpp ${subdirectory}/*.h)

    if(${checkonly})
        set(FORMAT_CHECK_PARAMS
            --style=file
            --dry-run
            --Werror)

        # Define the formatting check and apply parameters as variables
        message(STATUS "${FORMAT_CHECK_PARAMS}")
        message(STATUS "${FILES_TO_FORMAT}")
        message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}")
        add_custom_target(${targetName}
            COMMAND ${CLANG_FORMAT}
            ${FORMAT_CHECK_PARAMS}
            ${FILES_TO_FORMAT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

    else()
        set(FORMAT_APPLY_PARAMS
            --style=file
            -i)

        # Define a custom target for applying the formatting to all source files
        message(STATUS "${FORMAT_APPLY_PARAMS}")
        message(STATUS "${FILES_TO_FORMAT}")
        message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}")
        add_custom_target(${targetName}
            COMMAND ${CLANG_FORMAT}
            ${FORMAT_APPLY_PARAMS}
            ${FILES_TO_FORMAT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endfunction()

function(createClangFormatApplyTarget targetName subdirectory)
    createClangFormatTargets(${targetName} ${subdirectory} FALSE)
endfunction()

function(createClangFormatCheckTarget targetName subdirectory)
    createClangFormatTargets(${targetName} ${subdirectory} TRUE)
endfunction()
