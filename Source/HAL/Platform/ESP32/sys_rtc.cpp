/** @file       sys_rtc.cpp
 *  @brief      ESP32 platform implementation of IHal_Sys_Rtc (system wall-clock & RTC).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#define ENABLE_SYS_LOG_D

/*** INCLUDES ***/
#include "sys_rtc.hpp"
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <esp_timer.h>
#include <sys/time.h>
/*** INCLUDES ***/

/*** CONSTANTS ***/
namespace
{
    /// @brief Minimum plausible POSIX epoch (2024-01-01T00:00:00Z); below this the
    ///        wall-clock is considered un-synchronized (boot counter epoch).
    constexpr time_t   kMinPlausibleEpoch = 1704067200;
    constexpr uint32_t kFractionalUsMax   = 999999U;
    constexpr uint32_t kMutexTimeoutMs    = 100U;
} // namespace
/*** CONSTANTS ***/

/*** LOCAL FUNCTIONS ***/
bool sys_rtc::_lock(TickType_t timeoutMs)
{
    if (_mutex == nullptr)
    {
        return false;
    }
    return xSemaphoreTake(_mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

void sys_rtc::_unlock()
{
    if (_mutex != nullptr)
    {
        (void)xSemaphoreGive(_mutex);
    }
}
/*** LOCAL FUNCTIONS ***/

/*** FUNCTIONS ***/
sys_rtc::sys_rtc()
    : _config{hal_rtc_clock_source_t::INTERNAL_RC, 0U}
    , _initialized(HAL_UNINITIALIZED)
    , _mutex(nullptr)
{
    std::memset(_tzString, 0, sizeof(_tzString));
}

sys_rtc::~sys_rtc()
{
    (void)deInit();
}

sys_error_t sys_rtc::init(void* params)
{
    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("sys_rtc: already initialized");
        return ERROR_SUCCESS;
    }

    if (params != nullptr)
    {
        const hal_rtc_config_t* config = static_cast<const hal_rtc_config_t*>(params);
        _config                        = *config;
    }
    else
    {
        _config.clk_src                 = hal_rtc_clock_source_t::INTERNAL_RC;
        _config.calibration_interval_ms = 0U;
    }

    if (_mutex == nullptr)
    {
        _mutex = xSemaphoreCreateMutex();
        RETURN_IF_ERROR(_mutex != nullptr, ERROR_OUT_OF_MEMORY);
    }

    _initialized = HAL_INITIALIZED;
    SYS_LOG_I("sys_rtc: initialized (clk_src=%u, cal_interval=%ums)", static_cast<unsigned>(_config.clk_src), static_cast<unsigned>(_config.calibration_interval_ms));
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::feed()
{
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

    // The ESP32 system clock requires no periodic hardware feed; calibration of
    // the selected clock source, when requested, is handled by the RTC driver.
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::deInit()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }

    _initialized = HAL_UNINITIALIZED;
    SYS_LOG_D("sys_rtc: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::setTime(time_t epochSeconds, uint32_t fractionalUs)
{
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(fractionalUs <= kFractionalUsMax, ERROR_INVALID_ARG);

    RETURN_IF_ERROR(_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);

    struct timeval tv;
    tv.tv_sec  = epochSeconds;
    tv.tv_usec = static_cast<suseconds_t>(fractionalUs);

    if (settimeofday(&tv, nullptr) != 0)
    {
        _unlock();
        SYS_LOG_E("sys_rtc: settimeofday failed (errno=%d)", errno);
        return ERROR_WRITE_FAILED;
    }

    _unlock();
    SYS_LOG_I("sys_rtc: time set to %lld.%06lu s", static_cast<long long>(epochSeconds), static_cast<unsigned long>(fractionalUs));
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::getTime(time_t& epochSeconds, uint32_t* fractionalUs)
{
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);

    struct timeval tv;
    if (gettimeofday(&tv, nullptr) != 0)
    {
        _unlock();
        return ERROR_READ_FAILED;
    }

    epochSeconds = tv.tv_sec;
    if (fractionalUs != nullptr)
    {
        *fractionalUs = static_cast<uint32_t>(tv.tv_usec);
    }

    _unlock();
    return ERROR_SUCCESS;
}

uint64_t sys_rtc::getMonotonicTimeUs()
{
    const int64_t nowUs = esp_timer_get_time();
    return (nowUs > 0) ? static_cast<uint64_t>(nowUs) : 0ULL;
}

sys_error_t sys_rtc::setTimezone(const char* tzString)
{
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(tzString != nullptr, ERROR_NULL_POINTER);

    const size_t len = std::strlen(tzString);
    RETURN_IF_ERROR(len < sizeof(_tzString), ERROR_INVALID_ARG);

    RETURN_IF_ERROR(_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);

    std::memcpy(_tzString, tzString, len + 1U);
    (void)setenv("TZ", _tzString, 1);
    tzset();

    _unlock();
    SYS_LOG_I("sys_rtc: timezone set to '%s'", _tzString);
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::getTimezone(char* tzBuffer, size_t maxLen)
{
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR(tzBuffer != nullptr, ERROR_NULL_POINTER);
    RETURN_IF_ERROR(maxLen > 0U, ERROR_INVALID_ARG);

    RETURN_IF_ERROR(_lock(kMutexTimeoutMs), ERROR_DEVICE_BUSY);

    const size_t len = std::strlen(_tzString);
    if (len + 1U > maxLen)
    {
        _unlock();
        return ERROR_BUFFER_OVERFLOW;
    }

    std::memcpy(tzBuffer, _tzString, len + 1U);
    _unlock();
    return ERROR_SUCCESS;
}

bool sys_rtc::isTimeSet()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return false;
    }

    time_t epochSeconds = 0;
    if (getTime(epochSeconds, nullptr) != ERROR_SUCCESS)
    {
        return false;
    }

    return epochSeconds >= kMinPlausibleEpoch;
}
/*** FUNCTIONS ***/
