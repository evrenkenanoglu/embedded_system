/**
 * @file Proc_Switches.hpp
 * @brief Header file for Proc_Switches
 *
 * This file contains declarations for the Proc_Switches class and related data types and functions.
 */

#ifndef PROC_SWITCHES_HPP
#define PROC_SWITCHES_HPP

#include "HAL/IHal/IHal.h"
#include "Library/Utility/switch.hpp"
#include "Process/Process.hpp"
#include <functional>
#include <map>

#define ALL_SWITCHES 0xFF

class Proc_Switches : public Process
{
private:
    TaskHandle_t                            _xHandleSwitches; // Task handle for the switches
    std::map<uint16_t, SWITCH::Instance_t>& _switches;        // Reference to the map of switch instances
    QueueHandle_t                           _switchesQueue;   // Queue to handle the switches

    // Vector of Register cbs for notification of switch state changes
    std::vector<std::function<void(uint16_t, SWITCH::State)>> _switchStateChangeCbs;

private:
    // Private member functions
    static void SwitchesTask(void* pvParameters);

    /**
     * @brief Set the state of the switch
     *
     * @param switchIdx Index of the switch
     * @param state State to set
     * @return sys_error_t
     */
    sys_error_t setSwitchState(uint16_t switchIdx, SWITCH::State state);

    /**
     * @brief Process the switch event
     *
     * @param switchData Switch event data
     * @return sys_error_t
     */
    sys_error_t processSwitchEvent(SWITCH::EventData_t switchData);

    /**
     * @brief Notify the switch state change
     *
     * @param switchIdx Index of the switch
     * @param state State of the switch
     */
    void notifySwitchStateChange(uint16_t switchIdx, SWITCH::State state);

public:
    Proc_Switches(std::map<uint16_t, SWITCH::Instance_t>& switches, QueueHandle_t switchesQueue = nullptr);
    ~Proc_Switches();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

    QueueHandle_t getSwitchesQueue() const;

    /**
     * @brief Get the state of the switch
     *
     * @param switchIdx Switch number
     * @return bool State of the switch
     */
    SWITCH::State getSwitchState(uint16_t switchIdx);

    /**
     * @brief Register a callback function for switch state change
     *
     * @param cb Callback function
     */
    void registerSwitchStateChangeCb(std::function<void(uint16_t, SWITCH::State)> cb);
};

#endif /* PROC_SWITCHES_HPP */
