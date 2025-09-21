/**
 * @file Proc_Switches.hpp
 * @brief Header file for Proc_Switches
 *
 * This file contains declarations for the Proc_Switches class and related data types and functions.
 */

#ifndef PROC_SWITCHES_HPP
#define PROC_SWITCHES_HPP

#include "HAL/IHal/IHal.h"
#include "Process/Process.hpp"
#include <functional>
#include <vector>

#define ALL_SWITCHES 0xFF

class Proc_Switches : public Process
{

public:
    // Structure for the switches
    typedef struct
    {
        IHAL_IO& ioGpio;
        bool     state;
    } Switch_t;

    // Queue structure for the switches
    typedef struct
    {
        uint8_t switchNo;
        bool    state;
    } SwitchQueue_t;

private:
    TaskHandle_t                           _xHandleSwitches; // Task handle for the switches
    std::unique_ptr<std::vector<Switch_t>> _switches;        // States of the switches
    QueueHandle_t                          _SwitchesQueue;   // Queue to handle the switches

    // Vector of Register cbs for notification of switch state changes
    std::vector<std::function<void(uint8_t, bool)>> _switchStateChangeCbs;

private:
    // Private member functions
    static void SwitchesTask(void* pvParameters);

public:
    Proc_Switches(std::unique_ptr<std::vector<Switch_t>> Switches, QueueHandle_t SwitchesQueue);
    ~Proc_Switches();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

    QueueHandle_t getSwitchesQueue();

    /**
     * @brief Set the state of the switch
     *
     * @param switchNo Switch number
     * @param state State of the switch
     */
    void setSwitchState(uint8_t switchNo, bool state);

    /**
     * @brief Get the state of the switch
     *
     * @param switchNo Switch number
     * @return bool State of the switch
     */
    bool getSwitchState(uint8_t switchNo);

    /**
     * @brief Register a callback function for switch state change
     *
     * @param cb Callback function
     */
    void registerSwitchStateChangeCb(std::function<void(uint8_t, bool)> cb);

    /**
     * @brief Notify the switch state change
     *
     * @param switchNo Switch number
     * @param state State of the switch
     */
    void notifySwitchStateChange(uint8_t switchNo, bool state);

    /**
     * @brief Get the list of switches
     * @return std::vector<Switch_t>* Pointer to the vector of switches
     */
    std::vector<Switch_t>* getSwitches();
};

#endif /* PROC_SWITCHES_HPP */
