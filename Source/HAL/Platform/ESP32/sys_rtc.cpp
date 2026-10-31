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

/** INCLUDES ******************************************************************/
#include "sys_rtc.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/time.h>

/** CONSTANTS *****************************************************************/
namespace
{
    /// @brief Minimum plausible POSIX epoch (2024-01-01T00:00:00Z); below this the
    ///        wall-clock is considered un-synchronized (boot counter epoch).
    constexpr time_t   kMinPlausibleEpoch = 1704067200;
    constexpr uint32_t kFractionalUsMax   = 999999U;
    constexpr uint32_t kMutexTimeoutMs    = 100U;
} // namespace

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/
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

/** FUNCTIONS *****************************************************************/
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
    /// Check whether the RTC subsystem is already initialized.
    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("sys_rtc: already initialized");
        return ERROR_SUCCESS;
    }

    /// Apply custom configuration parameters if provided, otherwise default to internal RC.
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

    /// Allocate FreeRTOS synchronization mutex if not already created.
    if (_mutex == nullptr)
    {
        _mutex = xSemaphoreCreateMutex();
        RETURN_IF_ERROR((_mutex == nullptr), ERROR_OUT_OF_MEMORY);
    }

    /// Mark driver state as initialized and log startup parameters.
    _initialized = HAL_INITIALIZED;
    SYS_LOG_I("sys_rtc: initialized (clk_src=%u, cal_interval=%ums)", static_cast<unsigned>(_config.clk_src), static_cast<unsigned>(_config.calibration_interval_ms));
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::feed()
{
    /// Validate driver initialization state.
    RETURN_IF_ERROR((_initialized != HAL_INITIALIZED), ERROR_NOT_INITIALIZED);

    // The ESP32 system clock requires no periodic hardware feed; calibration of
    // the selected clock source, when requested, is handled by the RTC driver.
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::deInit()
{
    /// Return early if already deinitialized.
    if (_initialized != HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    /// Delete FreeRTOS synchronization mutex.
    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }

    /// Reset initialized lifecycle flag.
    _initialized = HAL_UNINITIALIZED;
    SYS_LOG_D("sys_rtc: de-initialized");
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::setTime(time_t epochSeconds, uint32_t fractionalUs)
{
    /// Validate initialization state and fractional microsecond bounds.
    RETURN_IF_ERROR((_initialized != HAL_INITIALIZED), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((fractionalUs > kFractionalUsMax), ERROR_INVALID_ARG);

    /// Acquire serialization mutex before updating system clock.
    RETURN_IF_ERROR((!_lock(kMutexTimeoutMs)), ERROR_DEVICE_BUSY);

    // Hardware note: settimeofday expects struct timeval with microseconds (Ignored by Doxygen)
    struct timeval tv;
    tv.tv_sec  = epochSeconds;
    tv.tv_usec = static_cast<suseconds_t>(fractionalUs);

    /// Commit the wall-clock timestamp to the POSIX system time base.
    RETURN_IF_ERROR(
        (settimeofday(&tv, nullptr) != 0),                           // Expression
        ERROR_WRITE_FAILED,                                          // Error Code
        SYS_LOG_E("sys_rtc: settimeofday failed (errno=%d)", errno), // Log Message
        _unlock()                                                    // Cleanup
    );

    /// Release serialization mutex.
    _unlock();
    SYS_LOG_I("sys_rtc: time set to %lld.%06lu s", static_cast<long long>(epochSeconds), static_cast<unsigned long>(fractionalUs));
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::getTime(time_t& epochSeconds, uint32_t* fractionalUs)
{
    /// Validate initialization state and acquire serialization mutex.
    RETURN_IF_ERROR((_initialized != HAL_INITIALIZED), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((!_lock(kMutexTimeoutMs)), ERROR_DEVICE_BUSY);

    /// Read current timestamp from POSIX clock.
    struct timeval tv;
    RETURN_IF_ERROR(
        (gettimeofday(&tv, nullptr) != 0), // Expression
        ERROR_READ_FAILED,                 // Error Code
        _unlock()                          // Cleanup
    );

    /// Populate output epoch seconds and optional fractional microseconds.
    epochSeconds = tv.tv_sec;
    if (fractionalUs != nullptr)
    {
        *fractionalUs = static_cast<uint32_t>(tv.tv_usec);
    }

    /// Release serialization mutex.
    _unlock();
    return ERROR_SUCCESS;
}

uint64_t sys_rtc::getMonotonicTimeUs()
{
    /// Read continuous monotonic microseconds elapsed since boot from esp_timer.
    const int64_t nowUs = esp_timer_get_time();
    return (nowUs > 0) ? static_cast<uint64_t>(nowUs) : 0ULL;
}

sys_error_t sys_rtc::setTimezone(const char* tzString)
{
    /// Validate initialization state, timezone string pointer, and buffer capacity.
    RETURN_IF_ERROR((_initialized != HAL_INITIALIZED), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((tzString == nullptr), ERROR_NULL_POINTER);

    const size_t len = std::strlen(tzString);
    RETURN_IF_ERROR((len >= sizeof(_tzString)), ERROR_INVALID_ARG);

    /// Acquire serialization mutex before updating environment variables.
    RETURN_IF_ERROR((!_lock(kMutexTimeoutMs)), ERROR_DEVICE_BUSY);

    /// Update the process timezone and report environment allocation failures.
    RETURN_IF_ERROR(
        (setenv("TZ", tzString, 1) != 0),                                             // Expression
        ERROR_WRITE_FAILED,                                                           // Error Code
        SYS_LOG_E("sys_rtc: setenv failed while setting timezone (errno=%d)", errno), // Log Message
        _unlock()                                                                     // Cleanup
    );

    /// Store the configured timezone and apply it to local-time conversions.
    std::memcpy(_tzString, tzString, len + 1U);
    tzset();

    /// Release serialization mutex.
    _unlock();
    SYS_LOG_I("sys_rtc: timezone set to '%s'", _tzString);
    return ERROR_SUCCESS;
}

sys_error_t sys_rtc::getTimezone(char* tzBuffer, size_t maxLen)
{
    /// Validate initialization state, output buffer pointer, and length constraint.
    RETURN_IF_ERROR((_initialized != HAL_INITIALIZED), ERROR_NOT_INITIALIZED);
    RETURN_IF_ERROR((tzBuffer == nullptr), ERROR_NULL_POINTER);
    RETURN_IF_ERROR((maxLen == 0U), ERROR_INVALID_ARG);

    /// Acquire serialization mutex.
    RETURN_IF_ERROR((!_lock(kMutexTimeoutMs)), ERROR_DEVICE_BUSY);

    /// Verify output buffer capacity accommodates stored timezone string with NUL terminator.
    const size_t len = std::strlen(_tzString);
    RETURN_IF_ERROR(
        (len + 1U > maxLen),   // Expression
        ERROR_BUFFER_OVERFLOW, // Error Code
        _unlock()              // Cleanup
    );

    /// Copy timezone string to destination buffer.
    std::memcpy(tzBuffer, _tzString, len + 1U);

    /// Release serialization mutex.
    _unlock();
    return ERROR_SUCCESS;
}

bool sys_rtc::isTimeSet()
{
    /// Return false immediately if driver is uninitialized.
    if (_initialized != HAL_INITIALIZED)
    {
        return false;
    }

    /// Retrieve current epoch timestamp.
    time_t epochSeconds = 0;
    if (getTime(epochSeconds, nullptr) != ERROR_SUCCESS)
    {
        return false;
    }

    /// Verify timestamp exceeds minimum plausible real-time epoch threshold.
    return epochSeconds >= kMinPlausibleEpoch;
}
