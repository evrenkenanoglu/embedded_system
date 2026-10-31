/** @file       Serv_sntp.hpp
 *  @brief      ESP32 platform implementation of ISntpClient (SNTP time synchronization).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "PAL/Protocols/SNTP/ISntpClient.hpp"
#include "System/system.h"

#include <atomic>

struct timeval; // forward declaration: SDK sync-notification callback argument

/**
 * @class Serv_sntp
 * @brief ESP32 lwIP/esp_sntp based implementation of ISntpClient.
 *
 * Wraps the ESP-IDF SNTP client in poll mode with up to three configurable
 * servers, configurable re-sync interval and step/slew clock discipline.
 * Synchronization status notifications are translated from the SDK callback
 * (soft-IRQ/TCPIP context) and forwarded to the user callback without
 * blocking or allocating.
 *
 * @note Thread-Safety: init/deInit/start/stop/triggerSync must be called from a
 *       single owner task. The SDK notification path only reads atomics and takes
 *       the bounded callback mutex.
 */
class Serv_sntp : public ISntpClient
{
public:
    Serv_sntp();
    ~Serv_sntp() override;

    Serv_sntp(const Serv_sntp&)            = delete;
    Serv_sntp& operator=(const Serv_sntp&) = delete;
    Serv_sntp(Serv_sntp&&)                 = delete;
    Serv_sntp& operator=(Serv_sntp&&)      = delete;

    sys_error_t        init(const sntp_client_config_t& config) override;
    sys_error_t        deInit() override;
    sys_error_t        start() override;
    sys_error_t        stop() override;
    sys_error_t        triggerSync() override;
    sntp_sync_status_t getSyncStatus() override;
    sys_error_t        setSyncCallback(sntp_sync_cb_t callback, void* userData = nullptr) override;

private:
    static void _sdkSyncNotificationTrampoline(struct timeval* tv);
    void        _onSdkSyncNotification();

    sntp_client_config_t            _config;
    uint8_t                         _initialized;
    uint8_t                         _running;
    std::atomic<sntp_sync_status_t> _status;
    sntp_sync_cb_t                  _callback;
    void*                           _callbackUserData;
    SemaphoreHandle_t               _mutex;
};
