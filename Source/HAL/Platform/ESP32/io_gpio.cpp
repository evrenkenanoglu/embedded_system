/**
 * @file io_gpio.cpp
 * @brief Source file for io_gpio
 *
 * This file contains definitions for the io_gpio class and related data types and functions.
 */

#include "io_gpio.hpp"
#include "esp_log.h"

#define TAG "GPIO"

namespace
{
const uint32_t gpioIntBlockTime = 50; // ms
} // namespace

/**
 * @brief GPIO interrupt handler
 *  This function is called when a GPIO interrupt is triggered.
 *  It disables the GPIO interrupts, starts a timer to wait for GPIO_INTR_BLOCK_TIME_MS ms,
 * and then re-enables the GPIO interrupts.
 *  The GPIO number is added to the event queue when the timer expires.
 *  The event queue is then processed by the gpioTask function.
 *
 * @param arg Pointer to the io_gpio object
 */
static void IRAM_ATTR isrHandler(void* arg)
{
    io_gpio* gpioClass = static_cast<io_gpio*>(arg);
    if (gpioClass == nullptr)
    {
        ESP_LOGE(TAG, "gpioClass can't be null!");
    }
    gpio_intr_disable(gpioClass->getGpioNumber());

    // Start a timer to wait for GPIO_INTR_BLOCK_TIME_MS ms
    // After the timer expires, re-enable the GPIO interrupts
    static TimerHandle_t timerHandle = NULL;
    if (timerHandle == NULL)
    {
        timerHandle = xTimerCreate("gpio_intr_block", pdMS_TO_TICKS(gpioIntBlockTime), pdFALSE, (void*)gpioClass,
                                   [](TimerHandle_t xTimer)
                                   {
                                       io_gpio* gpioClass = static_cast<io_gpio*>(pvTimerGetTimerID(xTimer));
                                       //    printf("GPIO[%d] intr, val: %d\n", gpioClass->getGpioNumber(), gpio_get_level(gpioClass->getGpioNumber()));

                                       uint32_t gpioNumber = gpioClass->getGpioNumber();
                                       // Add the GPIO inputs to the queue as a single event
                                       xQueueSendFromISR(gpioClass->getEventQueue(), &gpioNumber, NULL);

                                       // Re-enable the GPIO interrupts
                                       gpio_intr_enable(gpioClass->getGpioNumber());
                                   });
    }

    // Start the timer
    if (xTimerStart(timerHandle, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to start timer!");
    }
}

io_gpio::io_gpio(gpio_num_t gpioNumber, void* config) : _gpioNumber(gpioNumber), _config((gpio_config_t*)config), _gpioEventQueue(xQueueCreate(5, sizeof(uint32_t))), _counter(0) {}

sys_error_t io_gpio::init()
{
    if (_config == nullptr)
    {
        ESP_LOGE(TAG, "Config can't be null!");
    }
    ESP_ERROR_CHECK(gpio_config(_config));

    if (_config->mode == GPIO_MODE_INPUT)
    {
        // install gpio isr service
        gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
        // hook isr handler for specific gpio pin
        gpio_isr_handler_add(_gpioNumber, isrHandler, (void*)this);
    }

    return ERROR_SUCCESS;
}

io_gpio::~io_gpio()
{
    // remove isr handler for gpio number.
    gpio_isr_handler_remove(_gpioNumber);
}

void io_gpio::get(void* data)
{
    if (data != nullptr)
    {
        *reinterpret_cast<int*>(data) = gpio_get_level(_gpioNumber);
    }
}

sys_error_t io_gpio::set(void* data)
{
    if (_config->mode == GPIO_MODE_INPUT)
        ESP_LOGW(TAG, "Gpio is an input type! Level can not be set!");

    gpio_set_level(_gpioNumber, *(uint8_t*)data);

    return ERROR_SUCCESS;
}

QueueHandle_t io_gpio::getEventQueue()
{
    return _gpioEventQueue;
}

gpio_num_t io_gpio::getGpioNumber()
{
    return _gpioNumber;
}
