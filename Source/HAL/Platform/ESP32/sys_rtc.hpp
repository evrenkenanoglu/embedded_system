/** @file       sys_rtc.hpp
 *  @brief      ESP32 platform implementation of IHal_Sys_Rtc (system wall-clock & RTC).
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
 * @class sys_rtc
 * @brief ESP32 implementation of IHal_Sys_Rtc.
 *
 * Backed by the POSIX system clock (settimeofday/gettimeofday), the esp_timer
 * monotonic base and the C runtime TZ database. Thread-safe: all public
 * methods serialize through an internal FreeRTOS mutex.
 */
class sys_rtc : public IHal_Sys_Rtc
{
public:
    sys_rtc();
    ~sys_rtc() override;

    sys_rtc(const sys_rtc&)            = delete;
    sys_rtc& operator=(const sys_rtc&) = delete;
    sys_rtc(sys_rtc&&)                 = delete;
    sys_rtc& operator=(sys_rtc&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    sys_error_t setTime(time_t epochSeconds, uint32_t fractionalUs = 0) override;
    sys_error_t getTime(time_t& epochSeconds, uint32_t* fractionalUs = nullptr) override;
    uint64_t    getMonotonicTimeUs() override;
    sys_error_t setTimezone(const char* tzString) override;
    sys_error_t getTimezone(char* tzBuffer, size_t maxLen) override;
    bool        isTimeSet() override;

private:
    bool _lock(TickType_t timeoutMs);
    void _unlock();

    hal_rtc_config_t  _config;
    uint8_t           _initialized;
    SemaphoreHandle_t _mutex;
    char              _tzString[64];
};
