/** @file       Proc_StatusIndicator.hpp
 *  @brief      Prioritized monochrome status indication process.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       31/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Io_Gpio.h"
#include "Process/Process.hpp"

#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <atomic>
#include <cstdint>

/**
 * @brief Monochrome status indications, ordered from highest to lowest priority.
 */
enum class indication_state_t : uint8_t
{
    CRITICAL_ERROR  = 0U,
    OTA_UPDATING    = 1U,
    MATTER_PAIRING  = 2U,
    WIFI_CONNECTING = 3U,
    AP_PROVISIONING = 4U,
    NORMAL_IDLE     = 5U
};

/**
 * @class Proc_StatusIndicator
 * @brief Resolves prioritized status indications and drives one injected GPIO.
 *
 * A single worker task owns all GPIO writes. State updates are atomic and wake
 * the task with a coalescing notification; they must be made from task context.
 *
 * @note Thread-Safety: setState() may be called concurrently from task contexts.
 *       Lifecycle methods must be called from one owner task and not from an ISR.
 */
class Proc_StatusIndicator : public Process
{
public:
    /**
     * @brief Construct the status-indication process.
     * @param[in] outputGpio Initialized active-high GPIO output owned by the composition root.
     */
    explicit Proc_StatusIndicator(IHAL_IO_GPIO& outputGpio);
    ~Proc_StatusIndicator() override;

    Proc_StatusIndicator(const Proc_StatusIndicator&)            = delete;
    Proc_StatusIndicator& operator=(const Proc_StatusIndicator&) = delete;
    Proc_StatusIndicator(Proc_StatusIndicator&&)                 = delete;
    Proc_StatusIndicator& operator=(Proc_StatusIndicator&&)      = delete;

    sys_error_t start() override;
    sys_error_t stop() override;
    sys_error_t pause() override;
    sys_error_t resume() override;

    /**
     * @brief Set or clear one active indication.
     *
     * The highest-priority active indication is rendered. When no indication is
     * active, NORMAL_IDLE is rendered as the fallback. This API is task-context
     * only and does not perform GPIO I/O.
     *
     * @param[in] state Indication to activate or clear.
     * @param[in] active True to activate the indication; false to clear it.
     * @return sys_error_t ERROR_SUCCESS when accepted, or ERROR_INVALID_ARG for an invalid state.
     */
    sys_error_t setState(indication_state_t state, bool active);

private:
    static void        _taskEntry(void* context);
    void               _run();
    void               _notifyTask();
    sys_error_t        _setOutputLevel(hal_gpio_level_t level);
    void               _recordOutputError(sys_error_t error);
    indication_state_t _resolveState(uint32_t activeStateMask) const;

    IHAL_IO_GPIO& _outputGpio;

    TaskHandle_t       _taskHandle;
    EventGroupHandle_t _lifecycleEvents;

    std::atomic<uint32_t>    _activeStateMask;
    std::atomic<bool>        _notificationsEnabled;
    std::atomic<uint32_t>    _notificationUsers;
    std::atomic<bool>        _pauseRequested;
    std::atomic<bool>        _stopRequested;
    std::atomic<sys_error_t> _outputError;
};
