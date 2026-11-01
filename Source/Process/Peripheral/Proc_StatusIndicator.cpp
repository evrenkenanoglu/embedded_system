/** @file       Proc_StatusIndicator.cpp
 *  @brief      Prioritized monochrome status indication process.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       31/10/2026
 */

#define ENABLE_SYS_LOG_D

/** INCLUDES ******************************************************************/
#include "Proc_StatusIndicator.hpp"

#include "System/LogHandler.h"
#include "System/error_macros.h"

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr uint32_t    kWorkerStackSize        = 3072U;
    constexpr UBaseType_t kWorkerPriority         = 3U;
    constexpr uint32_t    kWorkerStoppedBit       = (1U << 0U);
    constexpr uint32_t    kWorkerPausedBit        = (1U << 1U);
    constexpr uint32_t    kWorkerWakeNotification = (1U << 0U);
    constexpr uint8_t     kIndicationStateCount   = 6U;

    struct indication_frame_t
    {
        hal_gpio_level_t level;
        uint16_t         durationMs;
    };

    struct indication_pattern_t
    {
        const indication_frame_t* frames;
        uint8_t                   frameCount;
    };

    constexpr indication_frame_t kCriticalErrorFrames[]  = {{hal_gpio_level_t::HIGH, 50U}, {hal_gpio_level_t::LOW, 50U}};
    constexpr indication_frame_t kOtaUpdatingFrames[]    = {{hal_gpio_level_t::HIGH, 100U}, {hal_gpio_level_t::LOW, 100U}};
    constexpr indication_frame_t kMatterPairingFrames[]  = {{hal_gpio_level_t::HIGH, 500U}, {hal_gpio_level_t::LOW, 500U}};
    constexpr indication_frame_t kWifiConnectingFrames[] = {{hal_gpio_level_t::HIGH, 250U}, {hal_gpio_level_t::LOW, 250U}};
    constexpr indication_frame_t kApProvisioningFrames[] = {
        {hal_gpio_level_t::HIGH, 100U}, {hal_gpio_level_t::LOW, 100U}, {hal_gpio_level_t::HIGH, 100U}, {hal_gpio_level_t::LOW, 700U}};
    constexpr indication_frame_t kNormalIdleFrames[] = {{hal_gpio_level_t::HIGH, 0U}};

    constexpr indication_pattern_t kPatterns[] = {
        {kCriticalErrorFrames, static_cast<uint8_t>(sizeof(kCriticalErrorFrames) / sizeof(kCriticalErrorFrames[0]))},
        {kOtaUpdatingFrames, static_cast<uint8_t>(sizeof(kOtaUpdatingFrames) / sizeof(kOtaUpdatingFrames[0]))},
        {kMatterPairingFrames, static_cast<uint8_t>(sizeof(kMatterPairingFrames) / sizeof(kMatterPairingFrames[0]))},
        {kWifiConnectingFrames, static_cast<uint8_t>(sizeof(kWifiConnectingFrames) / sizeof(kWifiConnectingFrames[0]))},
        {kApProvisioningFrames, static_cast<uint8_t>(sizeof(kApProvisioningFrames) / sizeof(kApProvisioningFrames[0]))},
        {kNormalIdleFrames, static_cast<uint8_t>(sizeof(kNormalIdleFrames) / sizeof(kNormalIdleFrames[0]))}};
} // namespace

/** LOCAL FUNCTIONS ***********************************************************/
indication_state_t Proc_StatusIndicator::_resolveState(uint32_t activeStateMask) const
{
    for (uint8_t stateIndex = 0U; stateIndex < kIndicationStateCount; ++stateIndex)
    {
        if ((activeStateMask & (1U << stateIndex)) != 0U)
        {
            return static_cast<indication_state_t>(stateIndex);
        }
    }

    return indication_state_t::NORMAL_IDLE;
}

void Proc_StatusIndicator::_recordOutputError(sys_error_t error)
{
    sys_error_t expected = ERROR_SUCCESS;
    if (_outputError.compare_exchange_strong(expected, error, std::memory_order_acq_rel))
    {
        SYS_LOG_E("Proc_StatusIndicator: GPIO output update failed (%d)", static_cast<int>(error));
    }
}

sys_error_t Proc_StatusIndicator::_setOutputLevel(hal_gpio_level_t level)
{
    RETURN_ON_ERROR(
        (_outputGpio.setLevel(level)), // Expression
        _recordOutputError(err)        // Cleanup
    );

    return ERROR_SUCCESS;
}

void Proc_StatusIndicator::_notifyTask()
{
    if (_taskHandle != nullptr)
    {
        (void)xTaskNotify(_taskHandle, kWorkerWakeNotification, eSetBits);
    }
}

void Proc_StatusIndicator::_taskEntry(void* context)
{
    static_cast<Proc_StatusIndicator*>(context)->_run();
}

void Proc_StatusIndicator::_run()
{
    (void)_setOutputLevel(hal_gpio_level_t::LOW);

    for (;;)
    {
        if (_stopRequested.load(std::memory_order_acquire))
        {
            (void)_setOutputLevel(hal_gpio_level_t::LOW);
            (void)xEventGroupSetBits(_lifecycleEvents, kWorkerStoppedBit);

            for (;;)
            {
                (void)xTaskNotifyWait(0U, kWorkerWakeNotification, nullptr, portMAX_DELAY);
            }
        }

        if (_pauseRequested.load(std::memory_order_acquire))
        {
            (void)_setOutputLevel(hal_gpio_level_t::LOW);
            (void)xEventGroupSetBits(_lifecycleEvents, kWorkerPausedBit);
            (void)xTaskNotifyWait(0U, kWorkerWakeNotification, nullptr, portMAX_DELAY);
            continue;
        }

        (void)xEventGroupClearBits(_lifecycleEvents, kWorkerPausedBit);

        const indication_state_t    currentState = _resolveState(_activeStateMask.load(std::memory_order_acquire));
        const indication_pattern_t& pattern      = kPatterns[static_cast<uint8_t>(currentState)];

        for (uint8_t frameIndex = 0U; frameIndex < pattern.frameCount; ++frameIndex)
        {
            if (_stopRequested.load(std::memory_order_acquire) || _pauseRequested.load(std::memory_order_acquire))
            {
                break;
            }

            if (_resolveState(_activeStateMask.load(std::memory_order_acquire)) != currentState)
            {
                break;
            }

            const indication_frame_t& frame = pattern.frames[frameIndex];
            (void)_setOutputLevel(frame.level);

            TickType_t frameDurationTicks = portMAX_DELAY;
            if (frame.durationMs != 0U)
            {
                frameDurationTicks = pdMS_TO_TICKS(frame.durationMs);
                if (frameDurationTicks == 0U)
                {
                    frameDurationTicks = 1U;
                }
            }

            const TickType_t frameStartTicks = xTaskGetTickCount();
            TickType_t       remainingTicks  = frameDurationTicks;
            for (;;)
            {
                if (_stopRequested.load(std::memory_order_acquire) || _pauseRequested.load(std::memory_order_acquire) ||
                    (_resolveState(_activeStateMask.load(std::memory_order_acquire)) != currentState))
                {
                    break;
                }

                const BaseType_t notified = xTaskNotifyWait(0U, kWorkerWakeNotification, nullptr, remainingTicks);
                if (notified != pdTRUE || frame.durationMs == 0U)
                {
                    break;
                }

                const TickType_t elapsedTicks = xTaskGetTickCount() - frameStartTicks;
                if (elapsedTicks >= frameDurationTicks)
                {
                    break;
                }
                remainingTicks = frameDurationTicks - elapsedTicks;
            }

            if (_stopRequested.load(std::memory_order_acquire) || _pauseRequested.load(std::memory_order_acquire) ||
                (_resolveState(_activeStateMask.load(std::memory_order_acquire)) != currentState))
            {
                break;
            }
        }
    }
}

/** FUNCTIONS *****************************************************************/
Proc_StatusIndicator::Proc_StatusIndicator(IHAL_IO_GPIO& outputGpio)
    : _outputGpio(outputGpio)
    , _taskHandle(nullptr)
    , _lifecycleEvents(nullptr)
    , _activeStateMask(0U)
    , _notificationsEnabled(false)
    , _notificationUsers(0U)
    , _pauseRequested(false)
    , _stopRequested(false)
    , _outputError(ERROR_SUCCESS)
{
    Process::setState(State::INITIALIZED);
}

Proc_StatusIndicator::~Proc_StatusIndicator()
{
    (void)stop();

    if (_lifecycleEvents != nullptr)
    {
        vEventGroupDelete(_lifecycleEvents);
        _lifecycleEvents = nullptr;
    }
}

sys_error_t Proc_StatusIndicator::start()
{
    RETURN_IF_ERROR(
        (getState() != State::INITIALIZED && getState() != State::STOPPED), // Expression
        ERROR_INVALID_STATE                                                 // Error Code
    );

    if (_lifecycleEvents == nullptr)
    {
        _lifecycleEvents = xEventGroupCreate();
        RETURN_IF_ERROR((_lifecycleEvents == nullptr), ERROR_OUT_OF_MEMORY);
    }

    (void)xEventGroupClearBits(_lifecycleEvents, kWorkerStoppedBit | kWorkerPausedBit);
    _pauseRequested.store(false, std::memory_order_release);
    _stopRequested.store(false, std::memory_order_release);
    _outputError.store(ERROR_SUCCESS, std::memory_order_release);
    _taskHandle = nullptr;

    const BaseType_t created = xTaskCreate(_taskEntry, "StatusIndicator", kWorkerStackSize, this, kWorkerPriority, &_taskHandle);
    RETURN_IF_ERROR(
        (created != pdPASS), // Expression
        ERROR_OUT_OF_MEMORY  // Error Code
    );

    Process::setState(State::RUNNING);
    _notificationsEnabled.store(true, std::memory_order_seq_cst);
    _notifyTask();
    return ERROR_SUCCESS;
}

sys_error_t Proc_StatusIndicator::stop()
{
    if (getState() == State::INITIALIZED)
    {
        return ERROR_SUCCESS;
    }
    if (getState() == State::STOPPED)
    {
        return _outputError.load(std::memory_order_acquire);
    }

    RETURN_IF_ERROR(
        (getState() != State::RUNNING && getState() != State::PAUSED), // Expression
        ERROR_INVALID_STATE                                            // Error Code
    );

    _notificationsEnabled.store(false, std::memory_order_seq_cst);
    _stopRequested.store(true, std::memory_order_release);
    _notifyTask();

    (void)xEventGroupWaitBits(_lifecycleEvents, kWorkerStoppedBit, pdFALSE, pdTRUE, portMAX_DELAY);
    while (_notificationUsers.load(std::memory_order_seq_cst) != 0U)
    {
        vTaskDelay(1U);
    }

    const TaskHandle_t taskHandle = _taskHandle;
    _taskHandle                   = nullptr;
    if (taskHandle != nullptr)
    {
        vTaskDelete(taskHandle);
    }

    Process::setState(State::STOPPED);
    return _outputError.load(std::memory_order_acquire);
}

sys_error_t Proc_StatusIndicator::pause()
{
    RETURN_IF_ERROR((getState() != State::RUNNING), ERROR_INVALID_STATE);

    (void)xEventGroupClearBits(_lifecycleEvents, kWorkerPausedBit);
    _pauseRequested.store(true, std::memory_order_release);
    _notifyTask();
    (void)xEventGroupWaitBits(_lifecycleEvents, kWorkerPausedBit, pdFALSE, pdTRUE, portMAX_DELAY);

    Process::setState(State::PAUSED);
    return _outputError.load(std::memory_order_acquire);
}

sys_error_t Proc_StatusIndicator::resume()
{
    RETURN_IF_ERROR((getState() != State::PAUSED), ERROR_INVALID_STATE);

    (void)xEventGroupClearBits(_lifecycleEvents, kWorkerPausedBit);
    _pauseRequested.store(false, std::memory_order_release);
    Process::setState(State::RUNNING);
    _notifyTask();
    return ERROR_SUCCESS;
}

sys_error_t Proc_StatusIndicator::setState(indication_state_t state, bool active)
{
    const uint8_t stateIndex = static_cast<uint8_t>(state);
    RETURN_IF_ERROR_WITH_LOG(
        (stateIndex >= kIndicationStateCount), // Expression
        ERROR_INVALID_ARG,                     // Error Code
        "Proc_StatusIndicator: invalid state"  // Log Message
    );

    const uint32_t stateBit     = (1U << stateIndex);
    const uint32_t previousMask = active ? _activeStateMask.fetch_or(stateBit, std::memory_order_acq_rel) : _activeStateMask.fetch_and(~stateBit, std::memory_order_acq_rel);
    if (((previousMask & stateBit) != 0U) == active)
    {
        return ERROR_SUCCESS;
    }

    _notificationUsers.fetch_add(1U, std::memory_order_seq_cst);
    if (_notificationsEnabled.load(std::memory_order_seq_cst) && (_taskHandle != nullptr))
    {
        (void)xTaskNotify(_taskHandle, kWorkerWakeNotification, eSetBits);
    }
    _notificationUsers.fetch_sub(1U, std::memory_order_seq_cst);
    return ERROR_SUCCESS;
}
