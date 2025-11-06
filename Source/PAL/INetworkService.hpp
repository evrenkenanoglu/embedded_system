#pragma once

#include "System/system.h" // Or your common error type header

// Enum to define which network mode a service should run in.
enum class NetworkServiceMode
{
    AP_MODE,  // Service runs when the device is an Access Point
    STA_MODE, // Service runs when the device is connected to a Wi-Fi network
    ANY       // Service runs in either mode
};

class INetworkService
{
public:
    virtual ~INetworkService() = default;

    /**
     * @brief Starts the network service.
     * This is called by the manager when the network becomes available.
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t start() = 0;

    /**
     * @brief Stops the network service.
     * This is called by the manager when the network is lost.
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t stop() = 0;

    /**
     * @brief Restarts the network service.
     * This is called by the manager when the network is restarted.
     * @return sys_error_t ERROR_SUCCESS on success, error code otherwise.
     */
    virtual sys_error_t restart() = 0;

    /**
     * @brief Gets the network mode in which the service should run.
     * @return NetworkServiceMode The mode in which the service should operate.
     */
    virtual NetworkServiceMode getMode() const = 0;

    /**
     * @brief Gets the name of the service for logging.
     * @return const char* service name.
     */
    virtual const char* getName() const = 0;
};