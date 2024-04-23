/**
 * @file Proc_Button.hpp
 * @brief Header file for Proc_Button
 *
 * This file contains declarations for the Proc_Button class and related data types and functions.
 */

#ifndef PROC_BUTTON_HPP
#define PROC_BUTTON_HPP

#include "HAL/Platform/ESP32/io_gpio.hpp"
#include "Process/IProcess.hpp"
#include <vector>

class Proc_Button : public IProcess
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

    void buttonDataClear(buttonData* button);

public:
    Proc_Button(std::vector<buttonData*>& button);
    ~Proc_Button();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;
};

#endif /* PROC_BUTTON_HPP */
