/** @file       io_gpio_expander.hpp
 *  @brief      Io gpio expander for mcp23x17
 *  @copyright  (c) 2023- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       04/09/2025
 */
#ifndef IO_GPIO_EXPANDER_HPP
#define IO_GPIO_EXPANDER_HPP

#include "HAL/IHAL/IHal_Io_Gpio.h"
#include "cpx_mcp23x17.hpp"

/** INCLUDES ******************************************************************/

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/**
 * @class io_gpio_expander
 * @brief Io gpio expander for mcp23x17
 */
class io_gpio_expander : public IHAL_IO_GPIO
{
private:
    /** VARIABLES *************************************************************/
    cpx_mcp23x17&      _expander;
    gpio_hal_config_t& _halConfig;
    bool               _initialized;
    QueueHandle_t      _eventQueue;

private:
    // user interrupt handler parameters
    void* _userInterruptParams;

private:
    // user interrupt handler function pointer
    void (*_userInterruptHandler)(void* params);

    /**
     * @brief Internal interrupt handler
     *
     * @param context
     */
    static void internalInterruptHandler(void* context);

public:
    io_gpio_expander(cpx_mcp23x17& expander, gpio_hal_config_t& config);
    ~io_gpio_expander();

    /** INTERFACE METHODS *****************************************************/
    sys_error_t init(void* params = nullptr) override;
    sys_error_t deInit() override;
    void        get(void* data) override;
    sys_error_t set(void* data) override;
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

public:
    /** USER METHODS *******************************************************/
};

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** FUNCTIONS *****************************************************************/

#endif // IO_GPIO_EXPANDER_HPP
