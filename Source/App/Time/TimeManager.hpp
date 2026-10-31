/** @file       TimeManager.hpp
 *  @brief      Application-level time services (ISO-8601, windows, certificate validity).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_Rtc.h"
#include "PAL/Protocols/SNTP/ISntpClient.hpp"
#include "System/system.h"

/**
 * @class TimeManager
 * @brief High-level time facade for the application layer.
 *
 * Combines the RTC wall-clock with the SNTP synchronization state to answer
 * application questions: is time trustworthy, what is the local ISO-8601
 * timestamp, are we inside a maintenance window, is an X.509 validity period
 * currently valid. All trust decisions fail closed when time is not
 * synchronized (ERROR_INVALID_STATE / false).
 *
 * @note Thread-Safety: methods are const and only read the RTC and SNTP client;
 *       thread safety is inherited from those collaborators.
 */
class TimeManager
{
public:
    /**
     * @brief Construct the time manager.
     * @param[in] sysRtc     RTC providing wall-clock time.
     * @param[in] sntpClient SNTP client providing synchronization state.
     */
    TimeManager(IHal_Sys_Rtc& sysRtc, ISntpClient& sntpClient);
    ~TimeManager() = default;

    TimeManager(const TimeManager&)            = delete;
    TimeManager& operator=(const TimeManager&) = delete;
    TimeManager(TimeManager&&)                 = delete;
    TimeManager& operator=(TimeManager&&)      = delete;

    /**
     * @brief Check whether the wall-clock is considered synchronized.
     * @return true when the SNTP client reports COMPLETED and the RTC is time-set.
     */
    bool isTimeSynchronized() const;

    /**
     * @brief Format the current local time as an ISO-8601 string ("YYYY-MM-DDTHH:MM:SS").
     * @param[out] buffer Destination buffer, must not be nullptr.
     * @param[in]  maxLen Size of the destination buffer in bytes (including NUL), >= 20.
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_INVALID_STATE when unsynchronized.
     */
    sys_error_t getIso8601String(char* buffer, size_t maxLen) const;

    /**
     * @brief Get the current local hour of day.
     * @param[out] hour Local hour in the range 0..23.
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_INVALID_STATE when unsynchronized.
     */
    sys_error_t getLocalHour(uint8_t& hour) const;

    /**
     * @brief Check whether the current local hour lies within a maintenance window.
     *
     * Supports wrap-around windows (e.g. start=23, end=03 spans midnight).
     * A window where start == end covers the full 24 hours.
     *
     * @param[in] startHour Window start hour (0..23).
     * @param[in] endHour   Window end hour (0..23).
     * @return true if the current local time is inside the window.
     * @return false when unsynchronized, arguments are out of range, or outside the window.
     */
    bool isWithinMaintenanceWindow(uint8_t startHour, uint8_t endHour) const;

    /**
     * @brief Check whether an X.509 certificate validity period covers the current time.
     * @param[in] notBefore Validity start (POSIX epoch seconds).
     * @param[in] notAfter  Validity end (POSIX epoch seconds).
     * @return true if notBefore <= now <= notAfter and time is synchronized.
     * @return false otherwise (fails closed when unsynchronized).
     */
    bool isCertificateValid(time_t notBefore, time_t notAfter) const;

private:
    IHal_Sys_Rtc& _sysRtc;
    ISntpClient&  _sntpClient;
};
