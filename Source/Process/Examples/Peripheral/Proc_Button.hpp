/**
 * @file Proc_Button.hpp
 * @brief Header file for Proc_Button
 *
 * This file contains declarations for the Proc_Button class and related data types and functions.
 */

#ifndef PROC_BUTTON_HPP
#define PROC_BUTTON_HPP

#include "HAL/IHal/IHal_Io_Gpio.h"
#include "HAL/Platform/ESP32/io_gpio.hpp"
#include "Library/Utility/button.hpp"
#include "Process/Process.hpp"
#include <functional>
#include <vector>

class Proc_Button : public Process
{
private:
    void buttonDataClear(BUTTON::Instance_t* button);

private:
    static void buttonListener(void* arg);

    std::unique_ptr<std::vector<BUTTON::Instance_t*>> _buttons;
    uint32_t                                          _stackSize;
    int                                               _taskPriority;
    TaskHandle_t                                      _taskHandle;

public:
    /**
     * @brief Construct a new Proc_Button object
     *
     * @param buttons A unique pointer to a vector of button instances
     */
    Proc_Button(std::unique_ptr<std::vector<BUTTON::Instance_t*>> buttons, uint32_t stackSize = 8192, int taskPriority = tskIDLE_PRIORITY + 1);
    ~Proc_Button();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

public:
    void buttonEventCallback(uint16_t buttonIndex, BUTTON::Event event, uint32_t duration);
    void processButtonEvent(BUTTON::Instance_t* button, hal_gpio_event_t* event);
};

#endif /* PROC_BUTTON_HPP */
