/**
 * @file Proc_Leds.cpp
 * @brief Source file for Proc_Leds
 *
 * This file contains definitions for the Proc_Leds class and related data types and functions.
 */

#include "Proc_Leds.hpp"
#include "HAL/Platform/ESP32/Library/logImpl.h"

namespace
{
constexpr uint16_t led_task_delay        = 100;                       // ms
constexpr uint16_t led_blink_slow_rate   = led_task_delay * 10;       // ms
constexpr uint16_t led_blink_rate        = led_task_delay * 5;        // ms
constexpr uint16_t led_blink_fast_rate   = led_task_delay;            // ms
constexpr uint16_t led_blink_toggle_rate = led_task_delay * 2;        // ms
constexpr uint16_t led_blink_once_rate   = led_blink_toggle_rate * 2; // ms
constexpr uint16_t led_blick_twice_rate  = led_blink_once_rate * 2;   // ms
constexpr uint16_t led_blick_thrice_rate = led_blink_once_rate * 3;   // ms
} // namespace

/**
 * @brief Task to handle the LEDs states
 * @param arg - vector of LED data
 */
static void procLedsTask(void* arg);
/**
 * @brief Toggle the LED
 *
 * @param led - LED data struct
 */
static void toggle(Proc_Leds::ledData* led);

/**
 * @brief Handle the LED blink state
 *
 * @param led - LED data struct
 * @param timeoutRate - the timeout rate for the LED
 */
static void handleBlink(Proc_Leds::ledData* led, uint32_t timeoutRate);

Proc_Leds::Proc_Leds(std::vector<ledData*>& leds, uint32_t stackSize, uint8_t taskPriority) : _leds(leds), _stackSize(stackSize), _taskPriority(taskPriority)
{
    // constructor implementation
}

Proc_Leds::~Proc_Leds()
{
    // destructor implementation
}

sys_error_t Proc_Leds::start()
{
    // start the LED task

    // Create the Button Listener
    BaseType_t result = xTaskCreate(procLedsTask,               // Task function
                                    "Leds_Task",                // Task name
                                    _stackSize,                 // Stack size
                                    static_cast<void*>(&_leds), // Task parameter
                                    _taskPriority,              // Task priority
                                    &_taskHandle);              // Task handle

    if (result != pdPASS)
    {
        logger().log(ILog::LogLevel::ERROR, "Proc_Leds: Failed to create task!");
        return ERROR_FAIL;
    }
    return ERROR_SUCCESS;
}

sys_error_t Proc_Leds::stop()
{
    // stop the LED task
    vTaskDelete(_taskHandle);
    return ERROR_SUCCESS;
}

sys_error_t Proc_Leds::pause()
{
    vTaskSuspend(_taskHandle);
    return ERROR_SUCCESS;
}

sys_error_t Proc_Leds::resume()
{
    vTaskResume(_taskHandle);
    return ERROR_SUCCESS;
}

sys_error_t Proc_Leds::setLedState(ledData& led, ledStateMachine state)
{
    // loop through the LEDs and find the corresponding LED to update its state
    for (ledData* _led : _leds)
    {
        if (_led == &led)
        {
            _led->counter = 0;
            _led->state   = state;

            return ERROR_SUCCESS;
        }
    }
    logger().log(ILog::LogLevel::ERROR, "Proc_Leds: LED not found!");
    return ERROR_INVALID_ARG;
}

static void toggle(Proc_Leds::ledData* led)
{
    led->onOff = !led->onOff;
    led->gpio.set((void*)&(led->onOff));
}

static void handleBlink(Proc_Leds::ledData* led, uint32_t timeoutRate)
{
    led->counter += led_task_delay;
    if (led->counter >= timeoutRate) // reset the counter and toggle the LED
    {
        led->counter = 0;
        toggle(led);
    }
}

static void handleBlinkTimesX(Proc_Leds::ledData* led, uint32_t timeoutRate)
{
    led->counter += led_task_delay;

    if (led->counter % led_blink_toggle_rate == 0) // toggle the LED
    {
        toggle(led);
    }

    if (led->counter >= timeoutRate) // reset the LED state
    {
        led->counter = 0;
        led->state   = LED_OFF;
    }
}

void procLedsTask(void* arg)
{
    // get the LED data
    std::vector<Proc_Leds::ledData*> leds = *static_cast<std::vector<Proc_Leds::ledData*>*>(arg);

    printf("Leds Task Started!\n");
    for (;;)
    {
        // loop through the LEDs and update their states
        for (auto led : leds)
        {

            switch (led->state)
            {
                case LED_OFF:
                {
                    uint8_t off = GPIO_LOW;
                    led->gpio.set((void*)&off);
                }
                break;
                case LED_ON:
                {
                    uint8_t on = GPIO_HIGH;
                    led->gpio.set((void*)&on);
                }
                break;

                case LED_BLINK_SLOW:
                    handleBlink(led, led_blink_slow_rate);
                    break;

                case LED_BLINK:
                    handleBlink(led, led_blink_rate);
                    break;

                case LED_BLINK_FAST:
                    handleBlink(led, led_blink_fast_rate);
                    break;

                case LED_BLINK_ONCE:
                    handleBlinkTimesX(led, led_blink_once_rate);
                    break;

                case LED_BLINK_TWICE:
                    handleBlinkTimesX(led, led_blick_twice_rate);
                    break;

                case LED_BLINK_THRICE:
                    handleBlinkTimesX(led, led_blick_thrice_rate);
                    break;
                default:
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(led_task_delay));
    }
}