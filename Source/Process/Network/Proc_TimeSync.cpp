/** @file       Proc_TimeSync.cpp
 *  @brief      Network-synchronized time process (SNTP + RTC + WDT supervision).
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
#include "Proc_TimeSync.hpp"
#include "System/LogHandler.h"
#include "System/error_macros.h"

#include <cstring>
#include <ctime>

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr uint32_t    kWorkerStackSize = 4096U;
    constexpr UBaseType_t kWorkerPriority  = 3U;

    /// @brief EventGroup bits signalling connectivity state to the worker loop.
    constexpr EventBits_t kNetworkUpBit      = (1U << 0);
    constexpr EventBits_t kStopRequestBit    = (1U << 1);
    constexpr EventBits_t kWorkerExitedBit   = (1U << 2);
    constexpr EventBits_t kNetworkChangedBit = (1U << 3);

    /// @brief Default timing, overridable via setTiming() (mirrors SSoT "time:" block).
    constexpr uint32_t kDefaultSyncIntervalSec    = 3600U;
    constexpr uint32_t kDefaultRetryIntervalSec   = 15U;
    constexpr uint32_t kDefaultMaxRetryBackoffSec = 300U;
    constexpr uint32_t kDefaultSyncTimeoutMs      = 10000U;

    /// @brief WDT feed period while sleeping (ms).
    constexpr uint32_t kWdtFeedPeriodMs = 1000U;
} // namespace

/** LOCAL FUNCTIONS ***********************************************************/
uint32_t Proc_TimeSync::_computeBackoffSec(uint32_t retryCount) const
{
    // T_retry = min(R_base * 2^n, R_max), with saturation-safe exponentiation.
    uint32_t delaySec = _retryIntervalSec;
    for (uint32_t i = 0U; (i < retryCount) && (delaySec < _maxRetryBackoffSec); ++i)
    {
        if (delaySec > (_maxRetryBackoffSec / 2U))
        {
            delaySec = _maxRetryBackoffSec;
        }
        else
        {
            delaySec *= 2U;
        }
    }

    return (delaySec > _maxRetryBackoffSec) ? _maxRetryBackoffSec : delaySec;
}

void Proc_TimeSync::_sleepSec(uint32_t seconds)
{
    if (!_networkUp.load())
    {
        return;
    }

    (void)xEventGroupClearBits(_eventGroup, kNetworkChangedBit);

    uint32_t remainingMs = seconds * 1000U;
    while (remainingMs > 0U)
    {
        if ((xEventGroupGetBits(_eventGroup) & kStopRequestBit) != 0U)
        {
            return;
        }

        const uint32_t sliceMs = (remainingMs > kWdtFeedPeriodMs) ? kWdtFeedPeriodMs : remainingMs;

        if (_wdt != nullptr)
        {
            (void)_wdt->feed();
        }

        const EventBits_t bits = xEventGroupWaitBits(_eventGroup, kNetworkChangedBit, pdTRUE, pdFALSE, pdMS_TO_TICKS(sliceMs));
        if (((bits | xEventGroupGetBits(_eventGroup)) & kStopRequestBit) != 0U)
        {
            return; // stop requested: abandon the remaining sleep
        }
        if ((bits & kNetworkChangedBit) != 0U)
        {
            return;
        }

        remainingMs -= sliceMs;
    }
}

void Proc_TimeSync::_waitForNetwork()
{
    SYS_LOG_I("Proc_TimeSync: waiting for network connectivity");
    for (;;)
    {
        // Bounded slices so the WDT is fed even during long network outages.
        if (_wdt != nullptr)
        {
            (void)_wdt->feed();
        }

        const EventBits_t bits = xEventGroupWaitBits(_eventGroup, kNetworkUpBit | kStopRequestBit, pdFALSE, pdFALSE, pdMS_TO_TICKS(kWdtFeedPeriodMs));
        if ((bits & kStopRequestBit) != 0U)
        {
            return;
        }
        if ((bits & kNetworkUpBit) != 0U)
        {
            SYS_LOG_I("Proc_TimeSync: network is up");
            return;
        }
    }
}

sys_error_t Proc_TimeSync::_runSyncAttempt(uint32_t& retryCount)
{
    RETURN_IF_ERROR((_sntpClient.triggerSync() != ERROR_SUCCESS), ERROR_NETWORK_UNAVAILABLE);

    /// Poll for completion up to syncTimeoutMs.
    const uint32_t pollSliceMs = 100U;
    uint32_t       waitedMs    = 0U;
    while (waitedMs < _syncTimeoutMs)
    {
        if (_wdt != nullptr)
        {
            (void)_wdt->feed();
        }

        if (_sntpClient.getSyncStatus() == sntp_sync_status_t::COMPLETED)
        {
            /// Propagate the synchronized time into the RTC.
            struct timespec ts;
            RETURN_IF_ERROR_WITH_LOG(
                (clock_gettime(CLOCK_REALTIME, &ts) != 0),               // Expression
                ERROR_READ_FAILED,                                       // Error Code
                "Proc_TimeSync: failed to read synchronized system time" // Log Message
            );

            const time_t   epochSeconds = ts.tv_sec;
            const uint32_t fractionalUs = static_cast<uint32_t>(ts.tv_nsec / 1000);
            RETURN_ON_ERROR(
                (_sysRtc.setTime(epochSeconds, fractionalUs)),                                                   // Expression
                SYS_LOG_E("Proc_TimeSync: failed to apply synchronized time to RTC (%d)", static_cast<int>(err)) // Log Message
            );

            if (_tzString[0] != '\0')
            {
                RETURN_ON_ERROR(
                    (_sysRtc.setTimezone(_tzString)),                                                // Expression
                    SYS_LOG_E("Proc_TimeSync: failed to apply timezone (%d)", static_cast<int>(err)) // Log Message
                );
            }

            retryCount = 0U;
            SYS_LOG_I("Proc_TimeSync: synchronized (epoch=%lld)", static_cast<long long>(epochSeconds));
            return ERROR_SUCCESS;
        }

        RETURN_IF_ERROR(((xEventGroupGetBits(_eventGroup) & kStopRequestBit) != 0U), ERROR_INVALID_STATE);

        vTaskDelay(pdMS_TO_TICKS(pollSliceMs));
        waitedMs += pollSliceMs;
    }

    SYS_LOG_W("Proc_TimeSync: sync attempt timed out (%ums)", static_cast<unsigned>(_syncTimeoutMs));
    return ERROR_TIMEOUT;
}

void Proc_TimeSync::_worker()
{
    SYS_LOG_I("Proc_TimeSync: worker started");

    for (;;)
    {
        _waitForNetwork();

        if ((xEventGroupGetBits(_eventGroup) & kStopRequestBit) != 0U)
        {
            SYS_LOG_I("Proc_TimeSync: stop requested, worker exiting");
            break;
        }

        uint32_t retryCount = 0U;
        bool     synced     = false;

        while (!synced)
        {
            const sys_error_t err = _runSyncAttempt(retryCount);
            if (err == ERROR_SUCCESS)
            {
                synced = true;
                break;
            }

            if ((xEventGroupGetBits(_eventGroup) & kStopRequestBit) != 0U)
            {
                break;
            }

            const uint32_t backoffSec = _computeBackoffSec(retryCount);
            ++retryCount;
            SYS_LOG_W("Proc_TimeSync: sync failed (%d), retry %u in %us", static_cast<int>(err), static_cast<unsigned>(retryCount), static_cast<unsigned>(backoffSec));
            _sleepSec(backoffSec);

            if ((xEventGroupGetBits(_eventGroup) & kNetworkUpBit) == 0U)
            {
                break; // connectivity lost: re-enter network wait
            }
        }

        if (synced)
        {
            _sleepSec(_syncIntervalSec);
        }
    }

    /// Signal the supervisor that the worker is gone before returning from the task function.
    // FreeRTOS deletes the task on return, so the handle is never dereferenced after task death.
    (void)xEventGroupSetBits(_eventGroup, kWorkerExitedBit);
}

void Proc_TimeSync::_workerTrampoline(void* param)
{
    static_cast<Proc_TimeSync*>(param)->_worker();
}

/** FUNCTIONS *****************************************************************/
Proc_TimeSync::Proc_TimeSync(ISntpClient& sntpClient, IHal_Sys_Rtc& sysRtc, IHal_Sys_Wdt* wdt)
    : Process()
    , _sntpClient(sntpClient)
    , _sysRtc(sysRtc)
    , _wdt(wdt)
    , _xHandle(nullptr)
    , _eventGroup(nullptr)
    , _networkUp(false)
    , _syncIntervalSec(kDefaultSyncIntervalSec)
    , _retryIntervalSec(kDefaultRetryIntervalSec)
    , _maxRetryBackoffSec(kDefaultMaxRetryBackoffSec)
    , _syncTimeoutMs(kDefaultSyncTimeoutMs)
    , _tzString{}
{
}

Proc_TimeSync::~Proc_TimeSync()
{
    (void)stop();

    if (_eventGroup != nullptr)
    {
        vEventGroupDelete(_eventGroup);
        _eventGroup = nullptr;
    }
}

sys_error_t Proc_TimeSync::start()
{
    RETURN_IF_ERROR_WITH_LOG(
        (getState() == State::RUNNING),  // Expression
        ERROR_INVALID_STATE,             // Error Code
        "Proc_TimeSync: already running" // Log Message
    );

    if (_eventGroup == nullptr)
    {
        _eventGroup = xEventGroupCreate();
        RETURN_IF_ERROR((_eventGroup == nullptr), ERROR_OUT_OF_MEMORY);
    }

    (void)xEventGroupClearBits(_eventGroup, kNetworkUpBit | kStopRequestBit | kWorkerExitedBit | kNetworkChangedBit);
    if (_networkUp.load())
    {
        (void)xEventGroupSetBits(_eventGroup, kNetworkUpBit);
    }

    RETURN_ON_ERROR((_sntpClient.start()));

    const BaseType_t created = xTaskCreate(_workerTrampoline, "Proc_TimeSync", kWorkerStackSize, this, kWorkerPriority, &_xHandle);
    if (created != pdPASS)
    {
        const sys_error_t stopError = _sntpClient.stop();
        if (stopError != ERROR_SUCCESS)
        {
            SYS_LOG_E("Proc_TimeSync: failed to stop SNTP client after task creation failure (%d)", static_cast<int>(stopError));
        }
        return ERROR_OUT_OF_MEMORY;
    }

    setState(State::RUNNING);
    SYS_LOG_I("Proc_TimeSync: started");
    return ERROR_SUCCESS;
}

sys_error_t Proc_TimeSync::stop()
{
    if (getState() != State::RUNNING && getState() != State::PAUSED)
    {
        return ERROR_SUCCESS;
    }

    if (_eventGroup != nullptr)
    {
        (void)xEventGroupSetBits(_eventGroup, kStopRequestBit);
    }

    /// Wait (bounded) for the worker to acknowledge shutdown via kWorkerExitedBit.
    // The handle is never probed after the task may have deleted itself, keeping this race-free.
    // Only a worker still alive after the bound is force-deleted.
    if (_eventGroup != nullptr)
    {
        const EventBits_t bits = xEventGroupWaitBits(_eventGroup, kWorkerExitedBit, pdFALSE, pdFALSE, pdMS_TO_TICKS(2000U));
        if ((bits & kWorkerExitedBit) != 0U)
        {
            _xHandle = nullptr;
        }
        else if (_xHandle != nullptr)
        {
            /// Worker is unresponsive: force-delete as a last resort.
            vTaskDelete(_xHandle);
            _xHandle = nullptr;
        }
    }
    else
    {
        _xHandle = nullptr;
    }

    setState(State::STOPPED);
    SYS_LOG_I("Proc_TimeSync: stopped");
    return _sntpClient.stop();
}

sys_error_t Proc_TimeSync::pause()
{
    RETURN_IF_ERROR((getState() != State::RUNNING), ERROR_INVALID_STATE);

    if (_xHandle != nullptr)
    {
        vTaskSuspend(_xHandle);
    }

    setState(State::PAUSED);
    SYS_LOG_I("Proc_TimeSync: paused");
    return ERROR_SUCCESS;
}

sys_error_t Proc_TimeSync::resume()
{
    RETURN_IF_ERROR((getState() != State::PAUSED), ERROR_INVALID_STATE);

    if (_xHandle != nullptr)
    {
        vTaskResume(_xHandle);
    }

    setState(State::RUNNING);
    SYS_LOG_I("Proc_TimeSync: resumed");
    return ERROR_SUCCESS;
}

void Proc_TimeSync::notifyNetworkEvent(hal_wifi_event_type_t event)
{
    switch (event)
    {
        case HAL_WIFI_EVENT_STA_GOT_IP:
            _networkUp.store(true);
            if (_eventGroup != nullptr)
            {
                (void)xEventGroupSetBits(_eventGroup, kNetworkUpBit | kNetworkChangedBit);
            }
            break;

        case HAL_WIFI_EVENT_STA_LOST_IP:
        case HAL_WIFI_EVENT_STA_DISCONNECTED:
            _networkUp.store(false);
            if (_eventGroup != nullptr)
            {
                (void)xEventGroupClearBits(_eventGroup, kNetworkUpBit);
                (void)xEventGroupSetBits(_eventGroup, kNetworkChangedBit);
            }
            break;

        default:
            break;
    }
}

sys_error_t Proc_TimeSync::setTiming(uint32_t syncIntervalSec, uint32_t retryIntervalSec, uint32_t maxRetryBackoffSec, uint32_t syncTimeoutMs)
{
    RETURN_IF_ERROR((getState() == State::RUNNING || getState() == State::PAUSED), ERROR_INVALID_STATE);

    _syncIntervalSec    = (syncIntervalSec > 0U) ? syncIntervalSec : kDefaultSyncIntervalSec;
    _retryIntervalSec   = (retryIntervalSec > 0U) ? retryIntervalSec : kDefaultRetryIntervalSec;
    _maxRetryBackoffSec = (maxRetryBackoffSec > 0U) ? maxRetryBackoffSec : kDefaultMaxRetryBackoffSec;
    _syncTimeoutMs      = (syncTimeoutMs > 0U) ? syncTimeoutMs : kDefaultSyncTimeoutMs;
    return ERROR_SUCCESS;
}

sys_error_t Proc_TimeSync::setTimezone(const char* tzString)
{
    RETURN_IF_ERROR((getState() == State::RUNNING || getState() == State::PAUSED), ERROR_INVALID_STATE);

    if (tzString == nullptr)
    {
        _tzString[0] = '\0';
        return ERROR_SUCCESS;
    }

    const size_t len = std::strlen(tzString);
    RETURN_IF_ERROR((len >= sizeof(_tzString)), ERROR_INVALID_ARG);

    std::memcpy(_tzString, tzString, len + 1U);
    return ERROR_SUCCESS;
}
