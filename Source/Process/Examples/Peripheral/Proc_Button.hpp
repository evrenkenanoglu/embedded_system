/**
 * @file Proc_Button.hpp
 * @brief Header file for Proc_Button
 *
 * This file contains declarations for the Proc_Button class and related data types and functions.
 */

#ifndef PROC_BUTTON_HPP
#define PROC_BUTTON_HPP

#include "HAL/Platform/ESP32/io_gpio.hpp"
#include "Process/Process.hpp"
#include <vector>

class Proc_Button : public Process
{
public:
    typedef struct
    {
        io_gpio&     gpio;
        int          prevState;
        int          currentState;
        const int    pressedState;
        uint32_t     changeTime;
        TaskHandle_t taskHandle;
    } buttonData;

private:
    std::vector<buttonData*>& _buttons;
    uint32_t                  _stackSize;
    uint8_t                   _taskPriority;

    /**
     * @brief Button Data Clear
     * 
     * @param button button data struct to clear
     */
    void buttonDataClear(buttonData* button);

public:
    /**
     * @brief Construct a new Proc_Button object
     *
     * @param button - vector of button data
     * @param stackSize - stack size (default 2048) of each button task
     * @param taskPriority - task priority (default 10)
     */
    Proc_Button(std::vector<buttonData*>& button, uint32_t stackSize = 2048, uint8_t taskPriority = 10);
    ~Proc_Button();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;
};

#endif /* PROC_BUTTON_HPP */
