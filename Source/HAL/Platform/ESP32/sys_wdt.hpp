/** @file       sys_wdt.hpp
 *  @brief      ESP32 platform implementation for IHal_Sys_Wdt.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *  @author     Evren Kenanoglu
 *  @date       27/09/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_Wdt.h"

class sys_wdt : public IHal_Sys_Wdt
{
private:
    uint8_t _initialized;

public:
    sys_wdt();
    ~sys_wdt() override;

    sys_wdt(const sys_wdt&)            = delete;
    sys_wdt& operator=(const sys_wdt&) = delete;
    sys_wdt(sys_wdt&&)                 = delete;
    sys_wdt& operator=(sys_wdt&&)      = delete;

public:
    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    sys_error_t registerCurrentTask() override;
    sys_error_t unregisterCurrentTask() override;
    void        yield(uint32_t delayTicks = 1) override;
};
