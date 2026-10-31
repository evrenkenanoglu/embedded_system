/** @file       Proc_TimeSync.hpp
 *  @brief      Network-synchronized time process (SNTP + RTC + WDT supervision).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Cpx_Wifi.h"
#include "HAL/IHAL/IHal_Sys_Rtc.h"
#include "HAL/IHAL/IHal_Sys_Wdt.h"
#include "PAL/Protocols/SNTP/ISntpClient.hpp"
#include "Process/Process.hpp"
#include "System/system.h"

#include <atomic>

/**
 * @class Proc_TimeSync
 * @brief Keeps the RTC wall-clock synchronized via SNTP.
 *
 * Waits for a network-connectivity signal (HAL_WIFI_EVENT_STA_GOT_IP), triggers
 * an on-demand SNTP sync, polls for completion with timeout, applies the result
 * to the RTC, and re-synchronizes periodically. Failed attempts are retried with
 * exponential backoff: T_retry = min(R_base * 2^n, R_max). The task loop never
 * returns; lifecycle follows IProcess (init/start/stop/pause/resume) and the
 * optional watchdog is fed on every sleep cycle.
 *
 * @note Thread-Safety: notifyNetworkEvent() may be called from the Wi-Fi event
 *       context (it uses an EventGroup and atomics). start/stop/pause/resume,
 *       setTiming() and setTimezone() must be called from a single owner task.
 */
class Proc_TimeSync : public Process
{
public:
    /**
     * @brief Construct the time-sync process.
     *
     * @param[in] sntpClient SNTP client to drive.
     * @param[in] sysRtc     RTC to update on successful synchronization.
     * @param[in] wdt        Optional watchdog to feed (nullptr = no supervision).
     */
    Proc_TimeSync(ISntpClient& sntpClient, IHal_Sys_Rtc& sysRtc, IHal_Sys_Wdt* wdt = nullptr);
    ~Proc_TimeSync() override;

    Proc_TimeSync(const Proc_TimeSync&)            = delete;
    Proc_TimeSync& operator=(const Proc_TimeSync&) = delete;
    Proc_TimeSync(Proc_TimeSync&&)                 = delete;
    Proc_TimeSync& operator=(Proc_TimeSync&&)      = delete;

    sys_error_t start() override;
    sys_error_t stop() override;
    sys_error_t pause() override;
    sys_error_t resume() override;

    /**
     * @brief Deliver a network event from the connectivity source.
     *
     * The owner of the WiFi stack must forward HAL_WIFI_EVENT_STA_GOT_IP /
     * HAL_WIFI_EVENT_STA_LOST_IP here so the worker can gate synchronization
     * on connectivity. Safe to call from ISR-free event contexts.
     *
     * @param[in] event Network event type.
     */
    void notifyNetworkEvent(hal_wifi_event_type_t event);

    /**
     * @brief Configure the synchronization cadence and retry policy.
     *
     * @param[in] syncIntervalSec       Re-sync period after a successful sync.
     * @param[in] retryIntervalSec      Base retry delay (R_base) on failure.
     * @param[in] maxRetryBackoffSec    Backoff ceiling (R_max).
     * @param[in] syncTimeoutMs         Max wait for one sync attempt to complete.
     * @return sys_error_t ERROR_SUCCESS if applied, ERROR_INVALID_STATE if running.
     */
    sys_error_t setTiming(uint32_t syncIntervalSec, uint32_t retryIntervalSec, uint32_t maxRetryBackoffSec, uint32_t syncTimeoutMs);

    /**
     * @brief Set the POSIX TZ applied to the RTC after a successful sync.
     * @param[in] tzString POSIX TZ string (e.g. "CET-1CEST,M3.5.0,M10.5.0/3"), nullptr = no change.
     * @return sys_error_t ERROR_SUCCESS if applied, ERROR_INVALID_ARG for an oversized string,
     *                     or ERROR_INVALID_STATE if running.
     */
    sys_error_t setTimezone(const char* tzString);

private:
    static void _workerTrampoline(void* param);
    void        _worker();
    void        _waitForNetwork();
    sys_error_t _runSyncAttempt(uint32_t& retryCount);
    void        _sleepSec(uint32_t seconds);
    uint32_t    _computeBackoffSec(uint32_t retryCount) const;

    ISntpClient&  _sntpClient;
    IHal_Sys_Rtc& _sysRtc;
    IHal_Sys_Wdt* _wdt;

    TaskHandle_t       _xHandle;
    EventGroupHandle_t _eventGroup;
    std::atomic<bool>  _networkUp;

    uint32_t _syncIntervalSec;
    uint32_t _retryIntervalSec;
    uint32_t _maxRetryBackoffSec;
    uint32_t _syncTimeoutMs;
    char     _tzString[64];
};
