/** @file       TimeManager.cpp
 *  @brief      Application-level time services (ISO-8601, windows, certificate validity).
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
#include "TimeManager.hpp"
#include "System/LogHandler.h"
#include "System/error_macros.h"

#include <cstring>
#include <ctime>
/*** INCLUDES ***/

/*** CONSTANTS ***/
namespace
{
    /// @brief Minimum ISO-8601 string length "YYYY-MM-DDTHH:MM:SS" + NUL.
    constexpr size_t kIso8601MinLen = 20U;
    constexpr size_t kIso8601FmtLen = 19U; // strlen("YYYY-MM-DDTHH:MM:SS")
} // namespace
/*** CONSTANTS ***/

/*** FUNCTIONS ***/
TimeManager::TimeManager(IHal_Sys_Rtc& sysRtc, ISntpClient& sntpClient)
    : _sysRtc(sysRtc)
    , _sntpClient(sntpClient)
{
}

bool TimeManager::isTimeSynchronized() const
{
    return (_sntpClient.getSyncStatus() == sntp_sync_status_t::COMPLETED) && _sysRtc.isTimeSet();
}

sys_error_t TimeManager::getIso8601String(char* buffer, size_t maxLen) const
{
    RETURN_IF_ERROR(buffer != nullptr, ERROR_NULL_POINTER);
    RETURN_IF_ERROR(maxLen >= kIso8601MinLen, ERROR_INVALID_ARG);
    RETURN_IF_ERROR(isTimeSynchronized(), ERROR_INVALID_STATE);

    time_t epochSeconds = 0;
    RETURN_ON_ERROR(_sysRtc.getTime(epochSeconds, nullptr));

    struct tm localTm;
    if (localtime_r(&epochSeconds, &localTm) == nullptr)
    {
        return ERROR_FAIL;
    }

    if (strftime(buffer, maxLen, "%Y-%m-%dT%H:%M:%S", &localTm) != kIso8601FmtLen)
    {
        buffer[0] = '\0';
        return ERROR_WRITE_FAILED;
    }

    return ERROR_SUCCESS;
}

bool TimeManager::isWithinMaintenanceWindow(uint8_t startHour, uint8_t endHour) const
{
    if ((startHour > 23U) || (endHour > 23U) || !isTimeSynchronized())
    {
        return false;
    }

    time_t epochSeconds = 0;
    if (_sysRtc.getTime(epochSeconds, nullptr) != ERROR_SUCCESS)
    {
        return false;
    }

    struct tm localTm;
    if (localtime_r(&epochSeconds, &localTm) == nullptr)
    {
        return false;
    }

    const uint8_t currentHour = static_cast<uint8_t>(localTm.tm_hour);

    /* start == end covers the full day; start < end is a plain range;
     * start > end wraps across midnight (e.g. 23 -> 03). */
    if (startHour == endHour)
    {
        return true;
    }

    if (startHour < endHour)
    {
        return (currentHour >= startHour) && (currentHour < endHour);
    }

    return (currentHour >= startHour) || (currentHour < endHour);
}

bool TimeManager::isCertificateValid(time_t notBefore, time_t notAfter) const
{
    if (!isTimeSynchronized() || (notBefore > notAfter))
    {
        return false;
    }

    time_t now = 0;
    if (_sysRtc.getTime(now, nullptr) != ERROR_SUCCESS)
    {
        return false;
    }

    return (now >= notBefore) && (now <= notAfter);
}
/*** FUNCTIONS ***/
