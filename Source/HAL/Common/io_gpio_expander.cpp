/** @file       io_gpio_expander.cpp
 *  @brief      Io gpio expander for mcp23x17
 *  @copyright  (c) 2023- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       04/09/2025
 */

/** INCLUDES ******************************************************************/
#include "io_gpio_expander.hpp"
#include "System/LogHandler.h"

namespace
{
uint8_t convertDirection(hal_gpio_direction_t direction)
{
    switch (direction)
    {
        case hal_gpio_direction_t::INPUT:
            return INPUT_MODE;
        case hal_gpio_direction_t::OUTPUT:
            return OUTPUT_MODE;
        default:
            return OUTPUT_MODE; // Default to output
    }
}

hal_gpio_direction_t convertDirection(uint8_t direction)
{
    switch (direction)
    {
        case INPUT_MODE:
            return hal_gpio_direction_t::INPUT;
        case OUTPUT_MODE:
            return hal_gpio_direction_t::OUTPUT;
        default:
            return hal_gpio_direction_t::OUTPUT; // Default to output
    }
}

uint8_t convertPull(hal_gpio_pull_t pull)
{
    switch (pull)
    {
        case hal_gpio_pull_t::NONE:
            return PULL_UP_RESISTOR_DISABLED;
        case hal_gpio_pull_t::PULL_UP:
            return PULL_UP_RESISTOR_ENABLED;
        case hal_gpio_pull_t::PULL_DOWN:
            return PULL_UP_RESISTOR_DISABLED;
        default:
            return PULL_UP_RESISTOR_DISABLED; // Default to no pull
    }
}

hal_gpio_pull_t convertPull(uint8_t pull)
{
    switch (pull)
    {
        case PULL_UP_RESISTOR_DISABLED:
            return hal_gpio_pull_t::NONE;
        case PULL_UP_RESISTOR_ENABLED:
            return hal_gpio_pull_t::PULL_UP;
        default:
            return hal_gpio_pull_t::NONE; // Default to no pull
    }
}

uint8_t convertLevel(hal_gpio_level_t level)
{
    switch (level)
    {
        case hal_gpio_level_t::LOW:
            return LOGIC_LOW;
        case hal_gpio_level_t::HIGH:
            return LOGIC_HIGH;
        default:
            return LOGIC_LOW; // Default to low
    }
}

hal_gpio_level_t convertLevel(uint8_t level)
{
    switch (level)
    {
        case LOGIC_LOW:
            return hal_gpio_level_t::LOW;
        case LOGIC_HIGH:
            return hal_gpio_level_t::HIGH;
        default:
            return hal_gpio_level_t::LOW; // Default to low
    }
}

} // namespace

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/

io_gpio_expander::io_gpio_expander(cpx_mcp23x17& expander, gpio_hal_config_t& config)
    : _expander(expander)
    , _halConfig(config)
    , _initialized(false)
    , _eventQueue(nullptr)
{
}

io_gpio_expander::~io_gpio_expander()
{
    // TODO: Cleanup if needed
}

sys_error_t io_gpio_expander::init(void* params)
{
    UNUSED(params);
    RETURN_ON_ERROR(_expander.init(nullptr));

    if (_halConfig.direction == hal_gpio_direction_t::OUTPUT)
    {
        setDirection(hal_gpio_direction_t::OUTPUT);
        setLevel(_halConfig.initial_level);
    }
    else if (_halConfig.direction == hal_gpio_direction_t::INPUT)
    {
        setDirection(hal_gpio_direction_t::INPUT);
        setPull(_halConfig.pull);
        if (_halConfig.enable_interrupt)
        {
            setInterrupt(_halConfig.interrupt);
            enableInterrupt();
        }
    }
    else
    {
        return ERROR_INVALID_CONFIG; // Invalid direction
    }

    _initialized = true;
    return ERROR_SUCCESS;
}

sys_error_t io_gpio_expander::deInit()
{
    // TODO: Implement deInit
    return;
}

void io_gpio_expander::get(void* data)
{
    if (data == nullptr)
    {
        return;
    }
    hal_gpio_level_t* level = static_cast<hal_gpio_level_t*>(data);
    getLevel(*level);
}

sys_error_t io_gpio_expander::set(void* data)
{
    if (data == nullptr)
    {
        return ERROR_INVALID_ARG;
    }

    hal_gpio_level_t* level = static_cast<hal_gpio_level_t*>(data);
    return setLevel(*level);
}

sys_error_t io_gpio_expander::setDirection(hal_gpio_direction_t direction)
{
    return _expander.setDirectionNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, convertDirection(direction));
}

sys_error_t io_gpio_expander::getDirection(hal_gpio_direction_t& direction)
{
    bool dir = false;
    RETURN_ON_ERROR_WITH_LOG(
        _expander.getDirectionNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, &dir), // Get direction
        "Failed to get direction",                                                                                    // Error Message
    );

    direction = convertDirection(dir ? 1 : 0);
    return ERROR_SUCCESS;
}

sys_error_t io_gpio_expander::setPull(hal_gpio_pull_t pull)
{
    return _expander.setPullUpResistorNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, convertPull(pull));
}

sys_error_t io_gpio_expander::getPull(hal_gpio_pull_t& pull)
{
    bool p = false;
    RETURN_ON_ERROR_WITH_LOG(
        _expander.getPullUpResistorNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, &p), // Get pull
        "Failed to get pull",                                                                                            // Error Message
    );
    pull = convertPull(p ? 1 : 0);
    return ERROR_SUCCESS;
}

sys_error_t io_gpio_expander::setInterrupt(hal_gpio_interrupt_t interrupt)
{
    // TODO: Implement setInterrupt
    return;
}

sys_error_t io_gpio_expander::getInterrupt(hal_gpio_interrupt_t& interrupt)
{
    // TODO: Implement getInterrupt
    return;
}

sys_error_t io_gpio_expander::setInterruptHandler(void (*handler)(void* params), void* params)
{
    // TODO: Implement setInterruptHandler
    return;
}

sys_error_t io_gpio_expander::enableInterrupt()
{
    return _expander.setInterruptEnableNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, true);
}

sys_error_t io_gpio_expander::disableInterrupt()
{
    return _expander.setInterruptEnableNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, false);
}

sys_error_t io_gpio_expander::clearInterrupt()
{
    // TODO: Implement clearInterrupt
    return;
}

sys_error_t io_gpio_expander::getLevel(hal_gpio_level_t& level)
{
    bool lvl = false;
    RETURN_ON_ERROR_WITH_LOG(
        _expander.getGpioNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, &lvl), // Get level
        "Failed to get level",                                                                                   // Error Message
    );
    level = convertLevel(lvl ? 1 : 0);
    return;
}

sys_error_t io_gpio_expander::setLevel(hal_gpio_level_t level)
{
    return _expander.setGpioNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, convertLevel(level));
}

sys_error_t io_gpio_expander::toggleLevel()
{
    hal_gpio_level_t currentLevel;
    RETURN_ON_ERROR(getLevel(currentLevel));
    return setLevel(currentLevel == hal_gpio_level_t::HIGH ? hal_gpio_level_t::LOW : hal_gpio_level_t::HIGH);
}

void* io_gpio_expander::getEventQueue()
{
    return (void*)_eventQueue;
}

uint16_t io_gpio_expander::getGpioNumber() const
{
    return _halConfig.pinNumber;
}

uint8_t io_gpio_expander::getPortNumber() const
{
    return _halConfig.portNumber;
}

bool io_gpio_expander::hasCapability(uint32_t capability) const
{
    return false;
}
