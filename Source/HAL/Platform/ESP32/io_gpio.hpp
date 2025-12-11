/**
 * @file io_gpio.hpp
 * @brief Header file for io_gpio
 *
 * This file contains declarations for the io_gpio class and related data types and functions.
 */

#ifndef IO_GPIO_HPP
#define IO_GPIO_HPP

#include "HAL/IHAL/IHal_Io_Gpio.h"
#include "driver/gpio.h"

class io_gpio : public IHAL_IO_GPIO
{
private:
    gpio_hal_config_t& _halConfig;
    QueueHandle_t      _gpioEventQueue = NULL;
    uint32_t           _counter;
    int                _prevState;
    void (*_interruptHandler)(void*) = nullptr;
    void*         _handlerParams     = nullptr;
    TimerHandle_t timerHandle        = nullptr;

public:
    io_gpio(gpio_hal_config_t& config);
    ~io_gpio();

    // Base IHAL_IO methods
    sys_error_t init(void* params = nullptr) override;
    void        get(void* data) override;
    sys_error_t set(void* data) override;
    sys_error_t deInit() override;

    // IHAL_IO_GPIO specific methods
    sys_error_t setDirection(hal_gpio_direction_t direction) override;
    sys_error_t getDirection(hal_gpio_direction_t& direction) override;
    sys_error_t setPull(hal_gpio_pull_t pull) override;
    sys_error_t getPull(hal_gpio_pull_t& pull) override;
    sys_error_t setInterrupt(hal_gpio_interrupt_t interrupt) override;
    sys_error_t getInterrupt(hal_gpio_interrupt_t& interrupt) override;
    sys_error_t setInterruptHandler(void (*handler)(void* params), void* params) override;
    sys_error_t enableInterrupt() override;
    sys_error_t disableInterrupt() override;
    sys_error_t clearInterrupt() override;
    sys_error_t getLevel(hal_gpio_level_t& level) override;
    sys_error_t setLevel(hal_gpio_level_t level) override;
    sys_error_t toggleLevel() override;
    void*       getEventQueue() override;
    uint16_t    getGpioNumber() const override;
    uint8_t     getPortNumber() const override;
    bool        hasCapability(uint32_t capability) const override;
};

#endif /* IO_GPIO_HPP */