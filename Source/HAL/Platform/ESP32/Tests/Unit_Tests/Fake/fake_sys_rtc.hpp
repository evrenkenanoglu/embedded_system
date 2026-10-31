/** @file       fake_sys_rtc.hpp
 *  @brief      In-memory test double for IHal_Sys_Rtc (epoch and monotonic counters).
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
#include "System/system.h"

/**
 * @class fake_sys_rtc
 * @brief Host-side IHal_Sys_Rtc simulating the epoch clock without touching the POSIX clock.
 */
class fake_sys_rtc : public IHal_Sys_Rtc
{
public:
    fake_sys_rtc();
    ~fake_sys_rtc() override;

    fake_sys_rtc(const fake_sys_rtc&)            = delete;
    fake_sys_rtc& operator=(const fake_sys_rtc&) = delete;
    fake_sys_rtc(fake_sys_rtc&&)                 = delete;
    fake_sys_rtc& operator=(fake_sys_rtc&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    sys_error_t setTime(time_t epochSeconds, uint32_t fractionalUs = 0) override;
    sys_error_t getTime(time_t& epochSeconds, uint32_t* fractionalUs = nullptr) override;
    uint64_t    getMonotonicTimeUs() override;
    sys_error_t setTimezone(const char* tzString) override;
    sys_error_t getTimezone(char* tzBuffer, size_t maxLen) override;
    bool        isTimeSet() override;

    /** @brief Advance the simulated monotonic counter. */
    void advanceMonotonicUs(uint64_t deltaUs);

private:
    bool     _initialized;
    time_t   _epochSeconds;
    uint32_t _fractionalUs;
    uint64_t _monotonicUs;
    char     _tzString[64];
};
