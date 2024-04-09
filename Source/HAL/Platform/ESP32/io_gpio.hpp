/**
 * @file io_gpio.hpp
 * @brief Header file for io_gpio
 *
 * This file contains declarations for the io_gpio class and related data types and functions.
 */

#ifndef IO_GPIO_HPP
#define IO_GPIO_HPP

#include "HAL/IHal.h"
#include "driver/gpio.h"

#define GPIO_HIGH 1
#define GPIO_LOW  0


class io_gpio : public IHAL_IO
{
private:
    gpio_num_t     _gpioNumber;
    gpio_config_t* _config;
    QueueHandle_t  _gpioEventQueue = NULL;
    uint32_t       _counter;
    int            _prevState;

public:
    io_gpio(gpio_num_t gpioNo, void* config);
    ~io_gpio();

    sys_error_t init();
    void        get(void* data) override;
    sys_error_t set(void* data) override;

    QueueHandle_t getEventQueue();
    gpio_num_t    getGpioNumber();
};

#endif /* IO_GPIO_HPP */
