/** @file       sys_reset.hpp
 *  @brief      ESP32 platform implementation of IHal_Sys_Reset (software system restart).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_Reset.h"
#include "System/system.h"

/**
 * @class sys_reset
 * @brief ESP32 implementation of IHal_Sys_Reset.
 */
class sys_reset : public IHal_Sys_Reset
{
public:
    sys_reset();
    ~sys_reset() override;

    sys_reset(const sys_reset&)            = delete;
    sys_reset& operator=(const sys_reset&) = delete;
    sys_reset(sys_reset&&)                 = delete;
    sys_reset& operator=(sys_reset&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    [[noreturn]] void restartSystem() override;

private:
    uint8_t _initialized;
};
