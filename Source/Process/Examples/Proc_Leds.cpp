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

const uint32_t stackSize    = 10000;
const uint8_t  taskPriority = 10;
} // namespace

static void LedsTask(void* arg);
static void toggle(Proc_Leds::ledData* led);

Proc_Leds::Proc_Leds(std::vector<ledData*>& leds) : _leds(leds)
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
    xTaskCreate(LedsTask,                   // Task function
                "Leds_Task",                // Task name
                stackSize,                  // Stack size
                static_cast<void*>(&_leds), // Task parameter
                taskPriority,               // Task priority
                &_taskHandle);              // Task handle

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
    // set the LED state
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

static void handleBlink(Proc_Leds::ledData* led, uint32_t rate)
{
    led->counter += led_task_delay;
    if (led->counter >= rate)
    {
        led->counter = 0;
        toggle(led);
    }
}

void LedsTask(void* arg)
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
                {
                    led->counter += led_task_delay;
                    printf("Led State: %d\n", led->state);
                    printf("Led Counter: %d\n", (int)led->counter);

                    if (led->counter % led_blink_toggle_rate == 0)
                    {
                        printf("Toggling\n");
                        toggle(led);
                    }

                    if (led->counter >= led_blink_once_rate)
                    {
                        led->counter = 0;
                        led->state   = LED_OFF;
                    }
                }
                break;

                case LED_BLINK_TWICE:
                {
                    led->counter += led_task_delay;
                    printf("Led State: %d\n", led->state);
                    printf("Led Counter: %d\n", (int)led->counter);

                    if (led->counter % led_blink_toggle_rate == 0)
                    {
                        printf("Toggling\n");
                        toggle(led);
                    }

                    if (led->counter >= led_blick_twice_rate)
                    {
                        led->counter = 0;
                        led->state   = LED_OFF;
                    }
                }
                break;

                case LED_BLINK_THRICE:
                {
                    led->counter += led_task_delay;
                    printf("Led State: %d\n", led->state);
                    printf("Led Counter: %d\n", (int)led->counter);

                    if (led->counter % led_blink_toggle_rate == 0)
                    {

                        printf("Toggling\n");
                        toggle(led);
                    }

                    if (led->counter >= led_blick_thrice_rate)
                    {
                        led->counter = 0;
                        led->state   = LED_OFF;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(led_task_delay));
    }
}