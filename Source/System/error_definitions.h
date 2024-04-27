
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

#ifndef ERROR_DEFINITIONS_H
#define ERROR_DEFINITIONS_H

#include <cstdio>

// Macro for handling errors and returning on error
#define RETURN_ON_ERROR_WITH_OUTPUT(expr, logWrapper)                                                                                                                                                  \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        sys_error_t err = (expr);                                                                                                                                                                      \
        if (err != ERROR_SUCCESS)                                                                                                                                                                      \
        {                                                                                                                                                                                              \
            char errMsg[256];                                                                                                                                                                          \
            std::sprintf(errMsg, "Error in %s at line %d: Error Code: &d\n", __FILE__, __LINE__, err);                                                                                                 \
            logWrapper.log(LogWrapper::LogLevel::ERROR, errMsg);                                                                                                                                       \
            return err;                                                                                                                                                                                \
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

typedef enum
{
    // General error codes
    ERROR_SUCCESS         = 0,  // Succesful
    ERROR_FAIL            = -1, // Generic failure
    ERROR_INVALID_ARG     = -2, // Invalid argument passed to a function
    ERROR_TIMEOUT         = -3, // Operation timed out
    ERROR_MEMORY          = -4, // Memory allocation or deallocation failure
    ERROR_IO              = -5, // Input/output error
    ERROR_NOT_SUPPORTED   = -6, // The operation or feature is not supported
    ERROR_NOT_IMPLEMENTED = -7, // Not implemented yet
    ERROR_INVALID_CONFIG  = -8, // Invalid configuration of the module
    ERROR_UNKNOWN         = -9, // Unknown or unspecified error

    // Error codes related to hardware and peripherals
    ERROR_INIT_FAILED  = -100, // Hardware initialization failure
    ERROR_READ_FAILED  = -101, // Data read from hardware failed
    ERROR_WRITE_FAILED = -102, // Data write to hardware failed
    ERROR_IRQ_HANDLER  = -103, // Error in interrupt handler
    ERROR_DEVICE_BUSY  = -104, // Device or resource is busy

    // Error codes related to memory
    ERROR_OUT_OF_MEMORY    = -201, // Memory allocation failed, out of available memory
    ERROR_MEMORY_CORRUPT   = -202, // Memory corruption detected
    ERROR_NULL_POINTER     = -203, // Attempted to dereference a null pointer
    ERROR_INVALID_POINTER  = -204, // Invalid memory pointer passed to a function
    ERROR_BUFFER_OVERFLOW  = -205, // Buffer overflow detected
    ERROR_BUFFER_UNDERFLOW = -206, // Buffer underflow detected
    ERROR_MEMORY_ALIGNMENT = -207, // Memory alignment error

    // Error codes related to communication
    ERROR_CONNECTION_FAILED   = -300, // Failed to establish a connection
    ERROR_PACKET_LOSS         = -301, // Data packet loss during communication
    ERROR_PROTOCOL            = -302, // Communication protocol error
    ERROR_TRANSMIT_FAILED     = -303, // Failed to transmit data
    ERROR_RECEIVE_FAILED      = -304, // Failed to receive data
    ERROR_DATA_CORRUPTED      = -305, // Data corruption during communication
    ERROR_CONNECTION_LOST     = -306, // Connection lost unexpectedly
    ERROR_DEVICE_UNRESPONSIVE = -307, // The communication device is not responding
    ERROR_BUSY                = -308, // The communication channel is busy
    ERROR_MESSAGE_TOO_LARGE   = -309, // The message size exceeds the allowed limit
    ERROR_CONNECTION_CLOSED   = -310, // The connection was closed by the remote end

    // Error codes related to network and internet
    ERROR_NETWORK_UNAVAILABLE  = -401, // Network connection is not available
    ERROR_DNS_LOOKUP_FAILED    = -402, // DNS lookup failed
    ERROR_SERVER_UNREACHABLE   = -403, // Server or host is unreachable
    ERROR_CONNECTION_REFUSED   = -404, // Connection to the server refused
    ERROR_CONNECTION_RESET     = -405, // Connection reset by peer
    ERROR_NETWORK_DISCONNECTED = -406, // Network disconnected unexpectedly
    ERROR_NETWORK_CONFIG       = -407, // Network configuration error
    ERROR_CONNECTION_ABORTED   = -408, // Connection aborted
    ERROR_NO_ROUTE_TO_HOST     = -409, // No route to the host or server
    ERROR_NETWORK_LIMIT        = -410, // Network resources exceeded or network bandwidth limit
    ERROR_NETWORK_AUTH         = -411, // Network authentication failed
    ERROR_NETWORK_TIMEOUT      = -412, // Network operation timed out
} sys_error_t;

#endif