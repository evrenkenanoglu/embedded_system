/**
 * @file Proc_Leds.hpp
 * @brief Header file for Proc_Leds
 *
 * This file contains declarations for the Proc_Leds class and related data types and functions.
 */

#ifndef PROC_LEDS_HPP
#define PROC_LEDS_HPP

#include "HAL/Platform/ESP32/io_gpio.hpp"
#include "Process/IProcess.hpp"
#include <stdbool.h>
#include <vector>

/**
 * @brief LED state machine
 */
typedef enum : uint8_t
{
    LED_OFF          = 0, // LED is off
    LED_ON           = 1, // LED is on
    LED_BLINK_SLOW   = 2, // LED is blinking at a slow rate (1000ms on, 1000ms off)
    LED_BLINK        = 3, // LED is blinking at a constant rate (250ms on, 250ms off)
    LED_BLINK_FAST   = 4, // LED is blinking at a fast rate (100ms on, 100ms off)
    LED_BLINK_ONCE   = 5, // LED is blinking once
    LED_BLINK_TWICE  = 6, // LED is blinking twice
    LED_BLINK_THRICE = 7, // LED is blinking thrice
} ledStateMachine;

class Proc_Leds : public IProcess
{
public:
    // private members
    typedef struct
    {
        io_gpio&        gpio;
        ledStateMachine state;
        uint8_t         onOff;
        uint32_t        counter;
    } ledData;

private:
    std::vector<ledData*>& _leds;
    TaskHandle_t           _taskHandle;
    uint32_t               _stackSize;
    uint8_t                _taskPriority;

public:
    /**
     * @brief Construct a new Proc_Leds object
     *
     * @param leds - vector of LED data
     * @param stackSize - stack size (default 10000)
     * @param taskPriority - task priority (default 10)
     */
    Proc_Leds(std::vector<ledData*>& leds, uint32_t stackSize = 10000, uint8_t taskPriority = 10);
    ~Proc_Leds();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

    /**
     * @brief Set the Led State object
     *
     * @param led
     * @param state
     * @return sys_error_t
     */
    sys_error_t setLedState(ledData& led, ledStateMachine state);
};
// which one makes more sense

// Attach one task for each led and control their state depending on the blink state or
// One task should handle all the leds state ?
#endif /* PROC_LEDS_HPP */
