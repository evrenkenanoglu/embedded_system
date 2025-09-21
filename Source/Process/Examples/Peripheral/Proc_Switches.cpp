/**
 * @file Proc_Switches.cpp
 * @brief Source file for Proc_Switches
 *
 * This file contains definitions for the Proc_Switches class and related data types and functions.
 */

#include "Proc_Switches.hpp"
// #define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

namespace
{
constexpr uint16_t SwitchesTaskStackSize   = 4096; // bytes
constexpr uint8_t  SwitchesTaskPriority    = 5;
constexpr char     SwitchesTaskName[]      = "SwitchesTask";
constexpr uint16_t programRoutineTaskDelay = 20;                          // milliseconds
constexpr uint8_t  SwitchesQueueSize       = sizeof(SWITCH::EventData_t); // Size of each item in the queue
constexpr uint8_t  SwitchesQueueLength     = 32;                          // Number of items in the queue
} // namespace

Proc_Switches::Proc_Switches(std::map<uint16_t, SWITCH::Instance_t>& switches, QueueHandle_t switchesQueue)
    : _xHandleSwitches(nullptr)
    , _switches(switches)
    , _SwitchesQueue(switchesQueue)
{
    setState(State::INITIALIZED);
}

Proc_Switches::~Proc_Switches() {}

sys_error_t Proc_Switches::start()
{
    RETURN_IF_ERROR_WITH_LOG(
        getState() != State::INITIALIZED && getState() != State::STOPPED, // Expression
        ERROR_INVALID_STATE,                                              // Error Code
        "Process not in INITIALIZED state");                              // Error Message

    BaseType_t result = pdFAIL;

    if (_SwitchesQueue == nullptr)
    {
        // create a queue to handle the switches
        _SwitchesQueue = xQueueCreate(SwitchesQueueLength, SwitchesQueueSize);
        RETURN_IF_ERROR(_SwitchesQueue == nullptr, ERROR_FAIL);
    }

    // create a task to handle the switches
    result = xTaskCreate(
        SwitchesTask,          // Task function
        SwitchesTaskName,      // Task name
        SwitchesTaskStackSize, // Stack size
        this,                  // Task parameters
        SwitchesTaskPriority,  // Task priority
        &_xHandleSwitches);    // Task handle

    RETURN_IF_ERROR_WITH_LOG(result != pdPASS, ERROR_FAIL, "Failed to create Switches task");

    SYS_LOG_I("Switches task created successfully");

    setState(State::RUNNING);

    return ERROR_SUCCESS;
}

sys_error_t Proc_Switches::stop()
{
    if (_xHandleSwitches == nullptr)
    {
        return ERROR_FAIL;
    }

    vTaskDelete(_xHandleSwitches);
    _xHandleSwitches = nullptr;
    vQueueDelete(_SwitchesQueue);
    _SwitchesQueue = nullptr;

    setState(State::STOPPED);

    return ERROR_SUCCESS;
}

sys_error_t Proc_Switches::pause()
{
    if (_xHandleSwitches == nullptr)
    {
        return ERROR_FAIL;
    }
    vTaskSuspend(_xHandleSwitches);

    setState(State::PAUSED);

    return ERROR_SUCCESS;
}

sys_error_t Proc_Switches::resume()
{
    if (_xHandleSwitches == nullptr)
    {
        return ERROR_FAIL;
    }
    vTaskResume(_xHandleSwitches);

    setState(State::RUNNING);

    return ERROR_SUCCESS;
}

sys_error_t Proc_Switches::processSwitchEvent(SWITCH::EventData_t eventData)
{
    RETURN_IF_ERROR(eventData.event != SWITCH::Event::STATE_CHANGED, ERROR_NOT_SUPPORTED); // Unsupported event

    if (eventData.index == ALL_SWITCHES) // All switches event
    {
        SYS_LOG_D("Processing event for all switches");
        for (auto const& [key, val] : _switches)
        {
            RETURN_ON_ERROR(setSwitchState(key, eventData.state));
        }
    }
    else // Single switch event
        RETURN_ON_ERROR(setSwitchState(eventData.index, eventData.state));

    // Notify the switch state change
    notifySwitchStateChange(static_cast<uint8_t>(eventData.index), eventData.state);

    return ERROR_SUCCESS;
}

sys_error_t Proc_Switches::setSwitchState(uint16_t switchIdx, SWITCH::State state)
{
    SYS_LOG_D("Setting switch %d to state %d", switchIdx, static_cast<int>(state));

    RETURN_IF_ERROR(_switches.find(switchIdx) == _switches.end(), ERROR_INVALID_ARG); // Switch index not found

    auto& switchInstance = _switches.at(switchIdx);

    bool             isInverted = switchInstance.isInverted;
    hal_gpio_level_t newState   = isInverted ? (state == SWITCH::State::ON ? hal_gpio_level_t::LOW : hal_gpio_level_t::HIGH)
                                             : (state == SWITCH::State::ON ? hal_gpio_level_t::HIGH : hal_gpio_level_t::LOW);

    RETURN_ON_ERROR(switchInstance.gpio.set(reinterpret_cast<void*>(&newState)));

    switchInstance.state = state;

    return ERROR_SUCCESS;
}

SWITCH::State Proc_Switches::getSwitchState(uint16_t switchIdx)
{
    RETURN_IF_ERROR(_switches.find(switchIdx) == _switches.end(), SWITCH::State::MAX); // Switch index not found

    return _switches.at(switchIdx).state;
}

void Proc_Switches::registerSwitchStateChangeCb(std::function<void(uint16_t, SWITCH::State)> cb)
{
    _switchStateChangeCbs.push_back(cb);
}

void Proc_Switches::notifySwitchStateChange(uint16_t switchIdx, SWITCH::State state)
{
    for (auto& cb : _switchStateChangeCbs)
    {
        cb(switchIdx, state);
    }
}

void Proc_Switches::SwitchesTask(void* pvParameters)
{
    Proc_Switches* proc = static_cast<Proc_Switches*>(pvParameters);

    RETURN_IF_ERROR(proc == nullptr, );

    for (;;)
    {
        // Check if there is anything in the queue
        SWITCH::EventData_t eventData;
        if (xQueueReceive(proc->_SwitchesQueue, &eventData, portMAX_DELAY))
        {
            SYS_LOG_D("\nSwitch Event: Index=%d\n, Event=%d\n, State=%d\n", eventData.index, static_cast<int>(eventData.event), static_cast<int>(eventData.state));

            // Process the switch event
            proc->processSwitchEvent(eventData);

            // Safety delay
            std::this_thread::sleep_for(std::chrono::milliseconds(programRoutineTaskDelay));
        }
    }
}
