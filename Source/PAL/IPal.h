#ifndef FILE_IPAL_H
#define FILE_IPAL_H

#include "System/system.h"

/**
 * @class IPAL_Service
 * @brief Interface for Platform Abstraction Layer (PAL) service operations.
 */
class IPAL_Service {
public:
    /**
     * @brief Initialize the service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t init() = 0;

    /**
     * @brief Start the service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t start() = 0;

    /**
     * @brief Stop the service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t stop() = 0;

    /**
     * @brief Restart the service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t restart() = 0;

    /**
     * @brief Get the status of the service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t getStatus() = 0;

    /**
     * @brief Destructor for IPAL_Service.
     */
    virtual ~IPAL_Service() {}
};


/**
 * @class IPAL_IO
 * @brief Interface for Platform Abstraction Layer (PAL) I/O operations.
 */
class IPAL_IO {
public:
    /**
     * @brief Initialize the I/O device.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t init() = 0;

    /**
     * @brief Read data from the I/O device.
     *
     * @param data Pointer to the buffer where the data will be stored.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t read(void* data) = 0;

    /**
     * @brief Write data to the I/O device.
     *
     * @param data Pointer to the buffer containing the data to be written.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t write(const void* data) = 0;

    /**
     * @brief Get the status of the I/O device.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t getStatus() = 0;

    /**
     * @brief Destructor for IPAL_IO.
     */
    virtual ~IPAL_IO() {}
};

#endif // FILE_IPAL_H