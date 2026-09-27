/** @file       IHal_Sys_Wdt.h
 *  @brief      Specialized interface for Task Watchdog Timer operations.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       27/09/2026
 */

#pragma once

#include "IHal.h"

/**
 * @class IHal_Sys_Wdt
 * @brief Specialization of IHAL_SYS providing task subscription and cooperative yielding.
 */
class IHal_Sys_Wdt : public IHAL_SYS
{
public:
    /**
     * @brief Subscribes the calling task to watchdog monitoring.
     *
     * @return sys_error_t ERROR_SUCCESS on successful registration.
     */
    virtual sys_error_t registerCurrentTask() = 0;

    /**
     * @brief Unsubscribes the calling task from watchdog monitoring.
     *
     * @return sys_error_t ERROR_SUCCESS on successful unregistration.
     */
    virtual sys_error_t unregisterCurrentTask() = 0;

    /**
     * @brief Yields the current task to prevent CPU starvation of lower-priority tasks.
     *
     * @param[in] delayTicks Number of scheduler ticks to yield/delay (defaults to 1).
     */
    virtual void yield(uint32_t delayTicks = 1) = 0;

    virtual ~IHal_Sys_Wdt() = default;
};
