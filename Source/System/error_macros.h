
/**
 * @file Error_Definitions.h
 * @brief This file contains definitions for error codes and macros to handle errors.
 * @version 0.1
 * @date 2023-02-12
 *
 * This file defines the sys_error_t enum, which contains error codes for different types of errors
 * that can occur in the system. It also defines macros to handle errors and return on error.
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef ERROR_MACROS_H
#define ERROR_MACROS_H

#include "errorTranslateHandler.h"
#include "error_definitions.h"
#include <cstdio>

#if defined(DEBUG_PRINT_ENABLED)
#define DEBUG_OUTPUT(err) SYS_LOG_E(ERROR_MESSAGE(err))
#else
#define DEBUG_OUTPUT(err) ((void)0)
#endif

// Macro for handling errors and returning on error
#define RETURN_ON_ERROR_WITH_OUTPUT(expr, message)                                                                                                                                                     \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        sys_error_t err = (expr);                                                                                                                                                                      \
        if (err != ERROR_SUCCESS)                                                                                                                                                                      \
        {                                                                                                                                                                                              \
            char errMsg[256];                                                                                                                                                                          \
            std::sprintf(errMsg, "Error in %s at line %d: Error Code: %d - %s", __FILE__, __LINE__, err, message);                                                                                     \
            SYS_LOG_E(errMsg);                                                                                                                                                                         \
            DEBUG_OUTPUT(err);                                                                                                                                                                         \
            return err;                                                                                                                                                                                \
        }                                                                                                                                                                                              \
    } while (0)

// Macro for handling errors and logging the error
#define ON_ERROR_WITH_OUTPUT(expr, message)                                                                                                                                                            \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        sys_error_t err = (expr);                                                                                                                                                                      \
        if (err != ERROR_SUCCESS)                                                                                                                                                                      \
        {                                                                                                                                                                                              \
            char errMsg[256];                                                                                                                                                                          \
            std::sprintf(errMsg, "Error in %s at line %d: Error Code: %d - %s", __FILE__, __LINE__, err, message);                                                                                     \
            SYS_LOG_E(errMsg);                                                                                                                                                                         \
            DEBUG_OUTPUT(err);                                                                                                                                                                         \
        }                                                                                                                                                                                              \
    } while (0)

// Macro for handling errors and returning on error
#define RETURN_ON_ERROR(expr)                                                                                                                                                                          \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        sys_error_t err = (expr);                                                                                                                                                                      \
        if (err != ERROR_SUCCESS)                                                                                                                                                                      \
        {                                                                                                                                                                                              \
            return err;                                                                                                                                                                                \
        }                                                                                                                                                                                              \
    } while (0)

#endif