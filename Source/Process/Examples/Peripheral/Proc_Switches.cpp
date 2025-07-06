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
constexpr uint16_t programRoutineTaskDelay = 20; // milliseconds
constexpr uint8_t  SwitchesQueueSize       = sizeof(Proc_Switches::SwitchQueue_t);
} // namespace

static void SwitchesTask(void* pvParameters);

Proc_Switches::Proc_Switches(std::unique_ptr<std::vector<Switch_t>> Switches)
    : _xHandleSwitches(nullptr)
    , _switches(std::move(Switches))
    , _SwitchesQueue(nullptr)
{
}

Proc_Switches::~Proc_Switches() {}

sys_error_t Proc_Switches::start()
{
    if (_switches == nullptr)
    {
        logger().log(ILog::LogLevel::ERROR, "Invalid Switches");
        return ERROR_FAIL;
    }

    BaseType_t result = pdFAIL;

    // create a queue to handle the switches
    _SwitchesQueue = xQueueCreate(SwitchesQueueSize, SwitchesQueueSize);

    // create a task to handle the switches
    result = xTaskCreate(SwitchesTask,          // Task function
                         SwitchesTaskName,      // Task name
                         SwitchesTaskStackSize, // Stack size
                         this,                  // Task parameters
                         SwitchesTaskPriority,  // Task priority
                         &_xHandleSwitches);    // Task handle

    if (result != pdPASS)
    {
        logger().log(ILog::LogLevel::INFO, " Task Failed to Start");
        return ERROR_FAIL;
    }

    logger().log(ILog::LogLevel::INFO, " Task Started");
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

    return ERROR_SUCCESS;
}

sys_error_t Proc_Switches::pause()
{
    if (_xHandleSwitches == nullptr)
    {
        return ERROR_FAIL;
    }
    vTaskSuspend(_xHandleSwitches);
    return ERROR_SUCCESS;
}

sys_error_t Proc_Switches::resume()
{
    if (_xHandleSwitches == nullptr)
    {
        return ERROR_FAIL;
    }
    vTaskResume(_xHandleSwitches);
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
        logger().log(ILog::LogLevel::ERROR, "Invalid switch number");
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

static void SwitchesTask(void* pvParameters)
{
    Proc_Switches* proc = static_cast<Proc_Switches*>(pvParameters);

    if (proc == nullptr)
    {
        logger().log(ILog::LogLevel::ERROR, " Task: Invalid parameters");
        return;
    }

    for (;;)
    {
        // Check if there is anything in the queue
        Proc_Switches::SwitchQueue_t Switch;
        if (xQueueReceive(proc->getSwitchesQueue(), &Switch, portMAX_DELAY))
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