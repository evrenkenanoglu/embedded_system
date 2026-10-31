/**
 * @file       ISntpClient.hpp
 * @brief      Vendor-free interface for SNTP time-synchronization clients.
 * @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *             Permission to use, reproduce, copy, prepare derivative works,
 *             modify, distribute, perform, display or sell this software and/or
 *             its documentation for any purpose is prohibited without the express
 *             written consent of Evren Kenanoglu.
 * @date       27/10/2026
 */

#pragma once

#include "System/system.h"
#include <cstdint>
#include <functional>

/**
 * @brief Synchronization progress/state of an SNTP client.
 */
enum class sntp_sync_status_t : uint8_t
{
    RESET = 0,   /*!< No synchronization performed (or status cleared) */
    IN_PROGRESS, /*!< A synchronization attempt is currently running */
    COMPLETED    /*!< The current or most recent synchronization attempt completed */
};

/**
 * @brief How a received SNTP sample is applied to the system clock.
 */
enum class sntp_sync_mode_t : uint8_t
{
    STEP = 0,   /*!< Step the clock immediately (default, RFC-traceable) */
    SMOOTH_SLEW /*!< Gradually slew the clock (no time jumps, for logs/RTC) */
};

/**
 * @brief Callback invoked when the synchronization status changes.
 *
 * @param[in] status   New synchronization status.
 * @param[in] userData Opaque user context supplied at registration time.
 */
typedef void (*sntp_sync_cb_t)(sntp_sync_status_t status, void* userData);

/**
 * @brief SNTP client configuration (mirrors SSoT "time:" block).
 */
typedef struct
{
    const char*      server_primary;   /*!< Primary NTP server (hostname/IP) */
    const char*      server_secondary; /*!< Secondary NTP server, nullptr = unused */
    const char*      server_tertiary;  /*!< Tertiary NTP server, nullptr = unused */
    uint32_t         sync_interval_ms; /*!< Periodic re-sync interval in ms (>= 15000) */
    sntp_sync_mode_t sync_mode;        /*!< Clock discipline mode */
} sntp_client_config_t;

/**
 * @class ISntpClient
 * @brief Pure interface for SNTP time-synchronization operations.
 *
 * Implementations must be safe for concurrent use and must not allocate
 * memory inside synchronization callback contexts.
 */
class ISntpClient
{
public:
    virtual ~ISntpClient() = default;

    /**
     * @brief Configure the client (servers, interval, sync mode).
     * @param[in] config Configuration to apply; server strings must outlive init().
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t init(const sntp_client_config_t& config) = 0;

    /**
     * @brief Release the client and any underlying protocol resources.
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t deInit() = 0;

    /**
     * @brief Start periodic synchronization polling.
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NOT_INITIALIZED / ERROR_INVALID_STATE otherwise.
     */
    virtual sys_error_t start() = 0;

    /**
     * @brief Stop periodic synchronization polling.
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t stop() = 0;

    /**
     * @brief Trigger an immediate on-demand synchronization attempt.
     * @return sys_error_t ERROR_SUCCESS when the attempt was scheduled.
     */
    virtual sys_error_t triggerSync() = 0;

    /**
     * @brief Get the current synchronization status.
     * @return sntp_sync_status_t Current status (RESET / IN_PROGRESS / COMPLETED).
     */
    virtual sntp_sync_status_t getSyncStatus() = 0;

    /**
     * @brief Register a synchronization status callback.
     * @param[in] callback Callback function or nullptr to unregister.
     * @param[in] userData Opaque context passed to the callback (default nullptr).
     * @return sys_error_t ERROR_SUCCESS on success.
     */
    virtual sys_error_t setSyncCallback(sntp_sync_cb_t callback, void* userData = nullptr) = 0;
};
