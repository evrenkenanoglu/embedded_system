/**
 * @file Proc_Switches.cpp
 * @brief Source file for Proc_Switches
 *
 * This file contains definitions for the Proc_Switches class and related data types and functions.
 */

#include "Proc_Switches.hpp"
#include "System/LogHandler.h"

namespace
{
constexpr uint16_t SwitchesTaskStackSize   = 4096; // bytes
constexpr uint8_t  SwitchesTaskPriority    = 5;
constexpr char     SwitchesTaskName[]      = "SwitchesTask";
constexpr uint16_t programRoutineTaskDelay = 20;                                   // milliseconds
constexpr uint8_t  SwitchesQueueSize       = sizeof(Proc_Switches::SwitchQueue_t); // Size of each item in the queue
constexpr uint8_t  SwitchesQueueLength     = 32;                                   // Number of items in the queue
} // namespace

Proc_Switches::Proc_Switches(std::unique_ptr<std::vector<Switch_t>> Switches, QueueHandle_t SwitchesQueue)
    : _xHandleSwitches(nullptr)
    , _switches(std::move(Switches))
    , _SwitchesQueue(SwitchesQueue)
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

    RETURN_IF_ERROR(_switches == nullptr, ERROR_INVALID_ARG);

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

void Proc_Switches::setSwitchState(uint8_t switchNo, bool state)
{
    if (switchNo == ALL_SWITCHES)
    {
        for (auto& switchData : *_switches)
        {
            switchData.ioGpio.set(reinterpret_cast<void*>(&state));
            switchData.state = state;
        }
        return;
    }

    if (switchNo >= _switches->size())
    {
        SYS_LOG_E("Invalid switch number");
        return;
    }

    _switches->at(switchNo).ioGpio.set(reinterpret_cast<void*>(&state));
    _switches->at(switchNo).state = state;
    _switches->at(switchNo).state = state;

    // Notify the switch state change
    notifySwitchStateChange(switchNo, state);
}

bool Proc_Switches::getSwitchState(uint8_t switchNo)
{
    return _switches->at(switchNo).state;
}

QueueHandle_t Proc_Switches::getSwitchesQueue()
{
    return _SwitchesQueue;
}

void Proc_Switches::registerSwitchStateChangeCb(std::function<void(uint8_t, bool)> cb)
{
    _switchStateChangeCbs.push_back(cb);
}

void Proc_Switches::notifySwitchStateChange(uint8_t switchNo, bool state)
{
    for (auto& cb : _switchStateChangeCbs)
    {
        cb(switchNo, state);
    }
}

void Proc_Switches::SwitchesTask(void* pvParameters)
{
    Proc_Switches* proc = static_cast<Proc_Switches*>(pvParameters);

    RETURN_IF_ERROR(proc == nullptr, );

    for (;;)
    {
        // Check if there is anything in the queue
        Proc_Switches::SwitchQueue_t Switch;
        if (xQueueReceive(proc->_SwitchesQueue, &Switch, portMAX_DELAY))
        {
            // Set the switch state
            proc->setSwitchState(Switch.switchNo, Switch.state);
        }
        // Safety delay
        std::this_thread::sleep_for(std::chrono::milliseconds(programRoutineTaskDelay));
    }
}

std::vector<Proc_Switches::Switch_t>* Proc_Switches::getSwitches()
{
    return _switches.get();
}