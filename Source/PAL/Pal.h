#ifndef FILE_PAL_H
#define FILE_PAL_H

#include "IPal.h"

/**
 * @class PAL_NetworkService
 * @brief Abstract class for Platform Abstraction Layer (PAL) network service operations.
 */
class PAL_NetworkService : public IPAL_NetworkService {
public:
    /**
     * @enum Status
     * @brief Enum class for network service status.
     */
    enum class Status {
        UNINITIALIZED,
        INITIALIZED,
        STARTED,
        STOPPED,
        RESTARTED,
        ERROR
    };

    /**
     * @brief Initialize the network service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t init() override = 0;

    /**
     * @brief Start the network service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t start() override = 0;

    /**
     * @brief Stop the network service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t stop() override = 0;

    /**
     * @brief Restart the network service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t restart() override = 0;

    /**
     * @brief Get the status of the network service.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t getStatus() override {
        return static_cast<sys_error_t>(status);
    }

    /**
     * @brief Set the status of the network service.
     *
     * @param newStatus The new status to set.
     */
    void setStatus(Status newStatus) {
        status = newStatus;
    }

    /**
     * @brief Destructor for PAL_NetworkService.
     */
    virtual ~PAL_NetworkService() {}

protected:
    Status status = Status::UNINITIALIZED;
};

/**
 * @class PAL_IO
 * @brief Abstract class for Platform Abstraction Layer (PAL) I/O operations.
 */
class PAL_IO : public IPAL_IO {
public:
    /**
     * @enum Status
     * @brief Enum class for I/O status.
     */
    enum class Status {
        UNINITIALIZED,
        INITIALIZED,
        READING,
        WRITING,
        ERROR
    };

    /**
     * @brief Initialize the I/O device.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t init() override = 0;

    /**
     * @brief Read data from the I/O device.
     *
     * @param data Pointer to the buffer where the data will be stored.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t read(void* data) override = 0;

    /**
     * @brief Write data to the I/O device.
     *
     * @param data Pointer to the buffer containing the data to be written.
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t write(const void* data) override = 0;

    /**
     * @brief Get the status of the I/O device.
     *
     * @return sys_error_t The error code indicating the success or failure of the operation.
     */
    virtual sys_error_t getStatus() override {
        return static_cast<sys_error_t>(status);
    }

    /**
     * @brief Set the status of the I/O device.
     *
     * @param newStatus The new status to set.
     */
    void setStatus(Status newStatus) {
        status = newStatus;
    }

    /**
     * @brief Destructor for PAL_IO.
     */
    virtual ~PAL_IO() {}

protected:
    Status status = Status::UNINITIALIZED;
};

#endif // FILE_PAL_H
