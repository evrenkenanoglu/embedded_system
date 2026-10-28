/** @file       sys_resetReason.hpp
 *  @brief      ESP32 platform implementation of IHal_Sys_ResetReason (silicon reset diagnostics).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sys_ResetReason.h"
#include "System/system.h"

/**
 * @class sys_resetReason
 * @brief ESP32 implementation of IHal_Sys_ResetReason.
 *
 * Decodes the ESP32-C6 silicon reset registers (esp_reset_reason) into the
 * platform-independent hal_reset_reason_t classification. Stateless: the
 * hardware latches the reset reason at power-up and it is read on demand.
 */
class sys_resetReason : public IHal_Sys_ResetReason
{
public:
    sys_resetReason();
    ~sys_resetReason() override;

    sys_resetReason(const sys_resetReason&)            = delete;
    sys_resetReason& operator=(const sys_resetReason&) = delete;
    sys_resetReason(sys_resetReason&&)                 = delete;
    sys_resetReason& operator=(sys_resetReason&&)      = delete;

    sys_error_t init(void* params = nullptr) override;
    sys_error_t feed() override;
    sys_error_t deInit() override;

    hal_reset_reason_t getResetReason() override;
    const char*        getResetReasonString() override;
    bool               isAbnormalReset() override;

private:
    uint8_t _initialized;
};
