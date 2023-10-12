/**
 * @file Error_Definitions.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-02-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "logWrapper.h"
#include <cstdio>

typedef int32_t error_t;

// Macro for handling errors and returning on error
#define RETURN_ON_ERROR_WITH_OUTPUT(expr, logWrapper)                                                                                                                                                  \
    do                                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        error_t err = (expr);                                                                                                                                                                          \
        if (err != ERROR_NONE)                                                                                                                                                                         \
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
        error_t err = (expr);                                                                                                                                                                          \
        if (err != ERROR_NONE)                                                                                                                                                                         \
        {                                                                                                                                                                                              \
            return err;                                                                                                                                                                                \
        }                                                                                                                                                                                              \
    } while (0)

// General error codes
#define ERROR_NONE                 0  // No error
#define ERROR_UNKNOWN              -1 // Unknown or unspecified error
#define ERROR_INVALID_ARG          -2 // Invalid argument passed to a function
#define ERROR_TIMEOUT              -3 // Operation timed out
#define ERROR_MEMORY               -4 // Memory allocation or deallocation failure
#define ERROR_IO                   -5 // Input/output error
#define ERROR_NOT_SUPPORTED        -6 // The operation or feature is not supported

// Error codes related to hardware and peripherals
#define ERROR_INIT_FAILED          -100 // Hardware initialization failure
#define ERROR_READ_FAILED          -101 // Data read from hardware failed
#define ERROR_WRITE_FAILED         -102 // Data write to hardware failed
#define ERROR_IRQ_HANDLER          -103 // Error in interrupt handler
#define ERROR_DEVICE_BUSY          -104 // Device or resource is busy

// Error codes related to memory
#define ERROR_OUT_OF_MEMORY        -201 // Memory allocation failed, out of available memory
#define ERROR_MEMORY_CORRUPT       -202 // Memory corruption detected
#define ERROR_NULL_POINTER         -203 // Attempted to dereference a null pointer
#define ERROR_INVALID_POINTER      -204 // Invalid memory pointer passed to a function
#define ERROR_BUFFER_OVERFLOW      -205 // Buffer overflow detected
#define ERROR_BUFFER_UNDERFLOW     -206 // Buffer underflow detected
#define ERROR_MEMORY_ALIGNMENT     -207 // Memory alignment error

// Error codes related to communication
#define ERROR_CONNECTION_FAILED    -300 // Failed to establish a connection
#define ERROR_PACKET_LOSS          -301 // Data packet loss during communication
#define ERROR_PROTOCOL             -302 // Communication protocol error
#define ERROR_TRANSMIT_FAILED      -303 // Failed to transmit data
#define ERROR_RECEIVE_FAILED       -304 // Failed to receive data
#define ERROR_DATA_CORRUPTED       -305 // Data corruption during communication
#define ERROR_CONNECTION_LOST      -306 // Connection lost unexpectedly
#define ERROR_DEVICE_UNRESPONSIVE  -307 // The communication device is not responding
#define ERROR_BUSY                 -308 // The communication channel is busy
#define ERROR_MESSAGE_TOO_LARGE    -309 // The message size exceeds the allowed limit
#define ERROR_CONNECTION_CLOSED    -310 // The connection was closed by the remote end

// Error codes related to network and internet
#define ERROR_NETWORK_UNAVAILABLE  -401 // Network connection is not available
#define ERROR_DNS_LOOKUP_FAILED    -402 // DNS lookup failed
#define ERROR_SERVER_UNREACHABLE   -403 // Server or host is unreachable
#define ERROR_CONNECTION_REFUSED   -404 // Connection to the server refused
#define ERROR_CONNECTION_RESET     -405 // Connection reset by peer
#define ERROR_NETWORK_DISCONNECTED -406 // Network disconnected unexpectedly
#define ERROR_NETWORK_CONFIG       -407 // Network configuration error
#define ERROR_CONNECTION_ABORTED   -408 // Connection aborted
#define ERROR_NO_ROUTE_TO_HOST     -409 // No route to the host or server
#define ERROR_NETWORK_LIMIT        -410 // Network resources exceeded or network bandwidth limit
#define ERROR_NETWORK_AUTH         -411 // Network authentication failed
#define ERROR_NETWORK_TIMEOUT      -412 // Network operation timed out
