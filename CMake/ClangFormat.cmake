# #######################################################################################
# CREATING CLANG-FORMAT TARGETS
# #######################################################################################
set(CLANG_FORMAT_PATH "")

function(createClangFormatTargets targetName subdirectory checkonly)
    # Find the program
    find_program(CLANG_FORMAT "clang-format" ${CLANG_FORMAT_PATH})
    message(STATUS "${CLANG_FORMAT}")
    file(GLOB_RECURSE FILES_TO_FORMAT RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${subdirectory}*.cpp ${subdirectory}*.hpp ${subdirectory}*.h)

    if(${checkonly})
        set(FORMAT_CHECK_PARAMS
            --style=file
            --output-replacements-xml
            --sort-includes
            --recursive
            --dry-run
            --Werror)

        # Define the formatting check and apply parameters as variables
        add_custom_target(${targetName}
            COMMAND
            ${CLANG_FORMAT}
            ${FORMAT_CHECK_PARAMS}
            ${FILES_TO_FORMAT}
            |grep -c '<replacement ' >/dev/null && echo "Formatting check failed" || echo "Formatting check passed"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

    else()
        set(FORMAT_APPLY_PARAMS
            --style=file
            --i
            --sort-includes
            --recursive)

        # Define a custom target for applying the formatting to all source files
        add_custom_target(${targetName}
            COMMAND
            ${CLANG_FORMAT}
            ${FORMAT_APPLY_PARAMS}
            ${FILES_TO_FORMAT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endfunction()

function(createClangFormatApplyTarget targetName subdirectory)
    createClangFormatTargets(${targetName} ${subdirectory} TRUE)
endfunction()

function(createClangFormatCheckTarget targetName subdirectory)
    createClangFormatTargets(${targetName} ${subdirectory} FALSE)
endfunction()
