/** @file       IHal_Sys_ResetReason.h
 *  @brief      Specialized interface for silicon reset reason diagnostics.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/10/2026
 */

#pragma once

#include "IHal.h"

#include <cstdint>

/**
 * @enum hal_reset_reason_t
 * @brief Platform-independent classification of silicon reset vectors.
 */
enum class hal_reset_reason_t : uint8_t
{
    UNKNOWN = 0,        /**< Reason could not be determined. */
    POWER_ON,           /**< Power-on reset or reset caused by power supply glitch. */
    EXTERNAL_PIN,       /**< Reset asserted through the external chip-pads pin. */
    SOFTWARE,           /**< Reset requested by the software (e.g. esp_restart). */
    PANIC_CRASH,        /**< Software panic or exception (fatal crash). */
    TASK_WATCHDOG,      /**< Task watchdog timer triggered. */
    INTERRUPT_WATCHDOG, /**< Interrupt watchdog timer triggered. */
    BROWNOUT,           /**< Brownout detector triggered. */
    DEEP_SLEEP,         /**< Wake-up from deep-sleep sleep mode. */
    CPU_LOCKUP          /**< CPU lock-up detected by the watchdog. */
};

/**
 * @class IHal_Sys_ResetReason
 * @brief Specialization of IHAL_SYS providing silicon startup/reset diagnostics.
 *
 * Decodes low-level hardware reset registers into a unified contract consumed
 * by the application layer to drive self-healing OTA rollbacks and diagnostic
 * telemetry.
 */
class IHal_Sys_ResetReason : public IHAL_SYS
{
public:
    /**
     * @brief Reads the reason of the most recent silicon reset.
     *
     * @return hal_reset_reason_t Platform-independent reset classification.
     */
    virtual hal_reset_reason_t getResetReason() = 0;

    /**
     * @brief Provides a human-readable representation of the reset reason.
     *
     * @return const char* Null-terminated string describing the reset reason.
     */
    virtual const char* getResetReasonString() = 0;

    /**
     * @brief Indicates whether the reset stems from an abnormal event.
     *
     * Abnormal resets include panic crashes, watchdog triggers, brownout and
     * CPU lock-up; these are the vectors relevant for OTA rollback decisions.
     *
     * @return true When the reset is considered abnormal, false otherwise.
     */
    virtual bool isAbnormalReset() = 0;

    virtual ~IHal_Sys_ResetReason() = default;
};
