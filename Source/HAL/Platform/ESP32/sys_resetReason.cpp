/** @file       sys_resetReason.cpp
 *  @brief      ESP32 platform implementation of IHal_Sys_ResetReason (silicon reset diagnostics).
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
#include "sys_resetReason.hpp"
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

#include <esp_system.h>
/*** INCLUDES ***/

/*** LOCAL FUNCTIONS ***/
namespace
{
    /**
     * @brief Translates an ESP-IDF native reset code into the HAL classification.
     */
    hal_reset_reason_t translateResetReason(esp_reset_reason_t reason)
    {
        switch (reason)
        {
            case ESP_RST_POWERON:
                return hal_reset_reason_t::POWER_ON;
            case ESP_RST_EXT:
                return hal_reset_reason_t::EXTERNAL_PIN;
            case ESP_RST_SW:
                return hal_reset_reason_t::SOFTWARE;
            case ESP_RST_PANIC:
                return hal_reset_reason_t::PANIC_CRASH;
            case ESP_RST_INT_WDT:
                return hal_reset_reason_t::INTERRUPT_WATCHDOG;
            case ESP_RST_TASK_WDT:
                return hal_reset_reason_t::TASK_WATCHDOG;
            case ESP_RST_WDT:
                // Watchdog reset with unknown source: treat as interrupt watchdog,
                // the most conservative abnormal classification.
                return hal_reset_reason_t::INTERRUPT_WATCHDOG;
            case ESP_RST_DEEPSLEEP:
                return hal_reset_reason_t::DEEP_SLEEP;
            case ESP_RST_BROWNOUT:
                return hal_reset_reason_t::BROWNOUT;
            case ESP_RST_CPU_LOCKUP:
                return hal_reset_reason_t::CPU_LOCKUP;
            default:
                // ESP_RST_UNKNOWN, ESP_RST_SDIO, ESP_RST_USB, ESP_RST_JTAG,
                // ESP_RST_EFUSE, ESP_RST_PWR_GLITCH: no dedicated HAL vector.
                return hal_reset_reason_t::UNKNOWN;
        }
    }

    /**
     * @brief Provides a human-readable string for a HAL reset classification.
     */
    const char* resetReasonToString(hal_reset_reason_t reason)
    {
        switch (reason)
        {
            case hal_reset_reason_t::POWER_ON:
                return "POWER_ON";
            case hal_reset_reason_t::EXTERNAL_PIN:
                return "EXTERNAL_PIN";
            case hal_reset_reason_t::SOFTWARE:
                return "SOFTWARE";
            case hal_reset_reason_t::PANIC_CRASH:
                return "PANIC_CRASH";
            case hal_reset_reason_t::TASK_WATCHDOG:
                return "TASK_WATCHDOG";
            case hal_reset_reason_t::INTERRUPT_WATCHDOG:
                return "INTERRUPT_WATCHDOG";
            case hal_reset_reason_t::BROWNOUT:
                return "BROWNOUT";
            case hal_reset_reason_t::DEEP_SLEEP:
                return "DEEP_SLEEP";
            case hal_reset_reason_t::CPU_LOCKUP:
                return "CPU_LOCKUP";
            case hal_reset_reason_t::UNKNOWN:
            default:
                return "UNKNOWN";
        }
    }
} // namespace
/*** LOCAL FUNCTIONS ***/

/*** FUNCTIONS ***/
sys_resetReason::sys_resetReason()
    : _initialized(HAL_UNINITIALIZED)
{
}

sys_resetReason::~sys_resetReason()
{
    (void)deInit();
}

sys_error_t sys_resetReason::init(void* params)
{
    (void)params;

    if (_initialized == HAL_INITIALIZED)
    {
        SYS_LOG_W("sys_resetReason: already initialized");
        return ERROR_SUCCESS;
    }

    _initialized = HAL_INITIALIZED;
    SYS_LOG_I("sys_resetReason: initialized (reason=%s)", resetReasonToString(getResetReason()));
    return ERROR_SUCCESS;
}

sys_error_t sys_resetReason::feed()
{
    RETURN_IF_ERROR(_initialized == HAL_INITIALIZED, ERROR_NOT_INITIALIZED);

    // The reset reason is latched by the silicon at power-up; no periodic
    // hardware feed is required.
    return ERROR_SUCCESS;
}

sys_error_t sys_resetReason::deInit()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return ERROR_SUCCESS;
    }

    _initialized = HAL_UNINITIALIZED;
    SYS_LOG_D("sys_resetReason: de-initialized");
    return ERROR_SUCCESS;
}

hal_reset_reason_t sys_resetReason::getResetReason()
{
    if (_initialized != HAL_INITIALIZED)
    {
        return hal_reset_reason_t::UNKNOWN;
    }

    return translateResetReason(esp_reset_reason());
}

const char* sys_resetReason::getResetReasonString()
{
    return resetReasonToString(getResetReason());
}

bool sys_resetReason::isAbnormalReset()
{
    switch (getResetReason())
    {
        case hal_reset_reason_t::PANIC_CRASH:
        case hal_reset_reason_t::TASK_WATCHDOG:
        case hal_reset_reason_t::INTERRUPT_WATCHDOG:
        case hal_reset_reason_t::BROWNOUT:
        case hal_reset_reason_t::CPU_LOCKUP:
            return true;
        default:
            return false;
    }
}
/*** FUNCTIONS ***/
