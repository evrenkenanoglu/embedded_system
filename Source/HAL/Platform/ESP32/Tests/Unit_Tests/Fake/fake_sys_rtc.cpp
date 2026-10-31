/** @file       fake_sys_rtc.cpp
 *  @brief      In-memory test double for IHal_Sys_Rtc (epoch and monotonic counters).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#include "fake_sys_rtc.hpp"

#include <cstring>

/*** CONSTANTS ***/
namespace
{
    /// @brief Mirrors sys_rtc::isTimeSet() plausibility threshold (2025-01-01T00:00:00Z).
    constexpr time_t kMinPlausibleEpoch = 1735689600;
} // namespace
/*** CONSTANTS ***/

/*** FUNCTIONS ***/
fake_sys_rtc::fake_sys_rtc()
    : _initialized(false)
    , _epochSeconds(0)
    , _fractionalUs(0U)
    , _monotonicUs(0U)
    , _tzString{}
{
}

fake_sys_rtc::~fake_sys_rtc()
{
    (void)deInit();
}

sys_error_t fake_sys_rtc::init(void* params)
{
    (void)params;
    _initialized = true;
    return ERROR_SUCCESS;
}

sys_error_t fake_sys_rtc::feed()
{
    return _initialized ? ERROR_SUCCESS : ERROR_NOT_INITIALIZED;
}

sys_error_t fake_sys_rtc::deInit()
{
    _initialized = false;
    return ERROR_SUCCESS;
}

sys_error_t fake_sys_rtc::setTime(time_t epochSeconds, uint32_t fractionalUs)
{
    if (!_initialized)
    {
        return ERROR_NOT_INITIALIZED;
    }
    if (fractionalUs > 999999U)
    {
        return ERROR_INVALID_ARG;
    }

    _epochSeconds = epochSeconds;
    _fractionalUs = fractionalUs;
    return ERROR_SUCCESS;
}

sys_error_t fake_sys_rtc::getTime(time_t& epochSeconds, uint32_t* fractionalUs)
{
    if (!_initialized)
    {
        return ERROR_NOT_INITIALIZED;
    }

    epochSeconds = _epochSeconds;
    if (fractionalUs != nullptr)
    {
        *fractionalUs = _fractionalUs;
    }
    return ERROR_SUCCESS;
}

uint64_t fake_sys_rtc::getMonotonicTimeUs()
{
    return _monotonicUs;
}

sys_error_t fake_sys_rtc::setTimezone(const char* tzString)
{
    if (!_initialized)
    {
        return ERROR_NOT_INITIALIZED;
    }
    if (tzString == nullptr)
    {
        return ERROR_NULL_POINTER;
    }
    if (std::strlen(tzString) >= sizeof(_tzString))
    {
        return ERROR_INVALID_ARG;
    }

    std::strncpy(_tzString, tzString, sizeof(_tzString) - 1U);
    _tzString[sizeof(_tzString) - 1U] = '\0';
    return ERROR_SUCCESS;
}

sys_error_t fake_sys_rtc::getTimezone(char* tzBuffer, size_t maxLen)
{
    if (!_initialized)
    {
        return ERROR_NOT_INITIALIZED;
    }
    if (tzBuffer == nullptr)
    {
        return ERROR_NULL_POINTER;
    }
    if (std::strlen(_tzString) + 1U > maxLen)
    {
        return ERROR_INVALID_ARG;
    }

    std::strcpy(tzBuffer, _tzString);
    return ERROR_SUCCESS;
}

bool fake_sys_rtc::isTimeSet()
{
    return _initialized && (_epochSeconds >= kMinPlausibleEpoch);
}

void fake_sys_rtc::advanceMonotonicUs(uint64_t deltaUs)
{
    _monotonicUs += deltaUs;
}
/*** FUNCTIONS ***/
