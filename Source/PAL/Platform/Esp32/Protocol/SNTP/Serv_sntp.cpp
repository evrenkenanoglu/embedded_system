/** @file       Serv_sntp.cpp
 *  @brief      ESP32 platform implementation of ISntpClient (SNTP time synchronization).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#define ENABLE_SYS_LOG_D

/** INCLUDES ******************************************************************/
#include "Serv_sntp.hpp"
#include "System/LogHandler.h"
#include "System/error_macros.h"

// The ESP-IDF lwIP SNTP header typedefs the C enums `sntp_sync_status_t` and
// `sntp_sync_mode_t` in the global namespace. Those names are already taken by
// the scoped enums declared in ISntpClient.hpp (both visible in this TU), so the
// SDK typedefs are macro-renamed before inclusion to keep the two unambiguous.
// All SDK enum values are converted to the interface enums via explicit mapping
// helpers below; the renamed SDK type names never leak past this translation unit.
#define sntp_sync_status_t esp_sdk_sntp_sync_status_t
#define sntp_sync_mode_t   esp_sdk_sntp_sync_mode_t
#include <esp_sntp.h>
#undef sntp_sync_status_t
#undef sntp_sync_mode_t

#include <cstring>

/** CONSTANTS *****************************************************************/
namespace
{
    /// @brief RFC 4330 minimum SNTP polling interval (ms).
    constexpr uint32_t kMinSyncIntervalMs = 15000U;
    constexpr uint32_t kMutexTimeoutMs    = 100U;
    /// @brief Maximum number of SNTP servers supported by lwIP (0=primary, 1=secondary, 2=tertiary).
    constexpr uint8_t kMaxSntpServers = 3U;

    /// @brief Owning instance registered for SDK sync-notification delivery (one SNTP client per process).
    Serv_sntp* sntpInstance = nullptr;
} // namespace

/** LOCAL FUNCTIONS ***********************************************************/
namespace
{
    esp_sdk_sntp_sync_mode_t toSdkSyncMode(sntp_sync_mode_t mode)
    {
        return (mode == sntp_sync_mode_t::SMOOTH_SLEW) ? SNTP_SYNC_MODE_SMOOTH : SNTP_SYNC_MODE_IMMED;
    }

} // namespace

/** FUNCTIONS *****************************************************************/
Serv_sntp::Serv_sntp()
    : _config{}
    , _initialized(0U)
    , _running(0U)
    , _status(sntp_sync_status_t::RESET)
    , _callback(nullptr)
    , _callbackUserData(nullptr)
    , _mutex(nullptr)
{
}

Serv_sntp::~Serv_sntp()
{
    (void)deInit();
}

sys_error_t Serv_sntp::init(const sntp_client_config_t& config)
{
    RETURN_IF_ERROR((config.server_primary == nullptr), ERROR_INVALID_ARG);
    RETURN_IF_ERROR((config.sync_interval_ms < kMinSyncIntervalMs), ERROR_INVALID_CONFIG);

    if (_initialized != 0U)
    {
        SYS_LOG_W("Serv_sntp: re-initializing");
        (void)deInit();
    }

    if (_mutex == nullptr)
    {
        _mutex = xSemaphoreCreateMutex();
        RETURN_IF_ERROR((_mutex == nullptr), ERROR_OUT_OF_MEMORY);
    }

    _config = config;
    _status.store(sntp_sync_status_t::RESET);

    /// Register the instance before arming the SDK notification callback.
    sntpInstance = this;

    /// Configure the SDK client in poll mode before initialization.
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_set_sync_mode(toSdkSyncMode(config.sync_mode));
    sntp_set_sync_interval(config.sync_interval_ms);

    (void)esp_sntp_set_time_sync_notification_cb(&Serv_sntp::_sdkSyncNotificationTrampoline);

    const char* servers[kMaxSntpServers] = {config.server_primary, config.server_secondary, config.server_tertiary};
    for (uint8_t i = 0U; i < kMaxSntpServers; ++i)
    {
        if (servers[i] != nullptr)
        {
            esp_sntp_setservername(i, servers[i]);
        }
    }

    _initialized = 1U;
    SYS_LOG_I(
        "Serv_sntp: initialized (interval=%ums, mode=%s)",
        static_cast<unsigned>(config.sync_interval_ms),
        (config.sync_mode == sntp_sync_mode_t::SMOOTH_SLEW) ? "SMOOTH_SLEW" : "STEP");
    return ERROR_SUCCESS;
}

sys_error_t Serv_sntp::deInit()
{
    if (_initialized == 0U)
    {
        return ERROR_SUCCESS;
    }

    if (_running != 0U)
    {
        (void)stop();
    }

    (void)esp_sntp_set_time_sync_notification_cb(nullptr);
    if (sntpInstance == this)
    {
        sntpInstance = nullptr;
    }
    _initialized = 0U;
    _status.store(sntp_sync_status_t::RESET);

    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }

    SYS_LOG_D("Serv_sntp: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t Serv_sntp::start()
{
    RETURN_IF_ERROR((_initialized == 0U), ERROR_NOT_INITIALIZED);

    if (_running != 0U)
    {
        return ERROR_SUCCESS;
    }

    esp_sntp_init();
    _running = 1U;

    _status.store(sntp_sync_status_t::IN_PROGRESS);

    SYS_LOG_I("Serv_sntp: polling started");
    return ERROR_SUCCESS;
}

sys_error_t Serv_sntp::stop()
{
    if (_running == 0U)
    {
        return ERROR_SUCCESS;
    }

    esp_sntp_stop();
    _running = 0U;
    SYS_LOG_D("Serv_sntp: polling stopped");
    return ERROR_SUCCESS;
}

sys_error_t Serv_sntp::triggerSync()
{
    RETURN_IF_ERROR((_initialized == 0U), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((_running == 0U), ERROR_INVALID_STATE);

    _status.store(sntp_sync_status_t::IN_PROGRESS);

    // sntp_restart() re-arms the polling timer immediately; it returns false
    // only when the SDK client was never initialized (guarded above).
    const bool restarted = sntp_restart();
    RETURN_IF_ERROR(
        (!restarted),                            // Expression
        ERROR_FAIL,                              // Error Code
        _status.store(sntp_sync_status_t::RESET) // Cleanup
    );

    SYS_LOG_D("Serv_sntp: manual sync triggered");
    return ERROR_SUCCESS;
}

sntp_sync_status_t Serv_sntp::getSyncStatus()
{
    if (_initialized == 0U)
    {
        return sntp_sync_status_t::RESET;
    }

    return _status.load();
}

sys_error_t Serv_sntp::setSyncCallback(sntp_sync_cb_t callback, void* userData)
{
    if (_mutex != nullptr)
    {
        RETURN_IF_ERROR((xSemaphoreTake(_mutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE), ERROR_DEVICE_BUSY);
        _callback         = callback;
        _callbackUserData = userData;
        (void)xSemaphoreGive(_mutex);
    }
    else
    {
        _callback         = callback;
        _callbackUserData = userData;
    }

    return ERROR_SUCCESS;
}

void Serv_sntp::_sdkSyncNotificationTrampoline(struct timeval* /*tv*/)
{
    // SDK notification callback: runs in the lwIP/TCPIP task context.
    // Resolve the owning instance registered at init() (one SNTP client per
    // process) and forward the event without blocking or allocating.
    if (sntpInstance != nullptr)
    {
        sntpInstance->_onSdkSyncNotification();
    }
}

void Serv_sntp::_onSdkSyncNotification()
{
    _status.store(sntp_sync_status_t::COMPLETED);

    sntp_sync_cb_t callback         = nullptr;
    void*          callbackUserData = nullptr;

    if (_mutex != nullptr)
    {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(kMutexTimeoutMs)) == pdTRUE)
        {
            callback         = _callback;
            callbackUserData = _callbackUserData;
            (void)xSemaphoreGive(_mutex);
        }
    }
    else
    {
        callback         = _callback;
        callbackUserData = _callbackUserData;
    }

    if (callback != nullptr)
    {
        callback(sntp_sync_status_t::COMPLETED, callbackUserData);
    }
}
