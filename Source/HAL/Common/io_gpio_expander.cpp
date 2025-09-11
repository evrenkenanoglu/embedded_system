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
    , _userInterruptParams(nullptr)
    , _userInterruptHandler(nullptr)
{
}

io_gpio_expander::~io_gpio_expander() {}

sys_error_t io_gpio_expander::init(void* params)
{
    UNUSED(params);
    RETURN_ON_ERROR(_expander.init(nullptr));

    // Configure pin based on halConfig
    if (_halConfig.direction == hal_gpio_direction_t::OUTPUT)
    {
        setDirection(hal_gpio_direction_t::OUTPUT);
        setLevel(_halConfig.initial_level);
    }
    else if (_halConfig.direction == hal_gpio_direction_t::INPUT)
    {
        SYS_LOG_D("Configuring GPIO Expander Pin: %d on Port: %d as INPUT", _halConfig.pinNumber, _halConfig.portNumber);
        setDirection(hal_gpio_direction_t::INPUT);
        setPull(_halConfig.pull);
        if (_halConfig.enable_interrupt)
        {
            SYS_LOG_D("Enabling interrupt for GPIO Expander Pin: %d on Port: %d", _halConfig.pinNumber, _halConfig.portNumber);
            setInterrupt(_halConfig.interrupt);
            enableInterrupt();
        }
    }
    else
    {
        return ERROR_INVALID_CONFIG; // Invalid direction
    }

    // Set polarity to normal (not inverted)
    RETURN_ON_ERROR(
        _expander.setPolarityNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, POLARITY_NORMAL), // Set polarity
    );

    SYS_LOG_I("Initialized GPIO Expander Pin: %d on Port: %d", _halConfig.pinNumber, _halConfig.portNumber);

    _initialized = true;
    return ERROR_SUCCESS;
}

sys_error_t io_gpio_expander::deInit()
{
    if (!_initialized)
    {
        return ERROR_SUCCESS;
    }

    // Disable interrupt
    RETURN_ON_ERROR(disableInterrupt());

    // Reset configuration to safe defaults
    RETURN_ON_ERROR(setDirection(hal_gpio_direction_t::INPUT));
    RETURN_ON_ERROR(setPull(hal_gpio_pull_t::NONE));

    _userInterruptHandler = nullptr;
    _userInterruptParams  = nullptr;
    _initialized          = false;

    SYS_LOG_I("Deinitialized GPIO Expander Pin: %d on Port: %d", _halConfig.pinNumber, _halConfig.portNumber);

    return ERROR_SUCCESS;
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
        _expander.getDirectionNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, dir), // Get direction
        "Failed to get direction",                                                                                   // Error Message
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
        _expander.getPullUpResistorNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, p), // Get pull
        "Failed to get pull",                                                                                           // Error Message
    );
    pull = convertPull(p ? 1 : 0);
    return ERROR_SUCCESS;
}

sys_error_t io_gpio_expander::setInterrupt(hal_gpio_interrupt_t interrupt)
{
    // Store the interrupt type in config
    _halConfig.interrupt = interrupt;

    bool setDefaultValue     = LOGIC_LOW;
    bool setInterruptControl = COMPARE_AGAINST_DEFVAL_STATE;

    switch (interrupt)
    {
        case hal_gpio_interrupt_t::LOW_LEVEL:
        {
            setDefaultValue     = LOGIC_HIGH;                   // Default value HIGH
            setInterruptControl = COMPARE_AGAINST_DEFVAL_STATE; // Interrupt on change from DEFVAL
            SYS_LOG_I("Configuring for LOW_LEVEL interrupt");
        }
        break;

        case hal_gpio_interrupt_t::HIGH_LEVEL:
        {
            setDefaultValue     = LOGIC_LOW;                    // Default value LOW
            setInterruptControl = COMPARE_AGAINST_DEFVAL_STATE; // Interrupt on change from DEFVAL
            SYS_LOG_I("Configuring for HIGH_LEVEL interrupt");
        }
        break;

        case hal_gpio_interrupt_t::BOTH_EDGES:
        {
            setInterruptControl = COMPARE_AGAINST_PREVIOUS_STATE; // Interrupt on any change
            SYS_LOG_I("Configuring for BOTH_EDGES interrupt");
        }
        break;

        // Doesn't support edge interrupts directly, so configure for change detection and should be handled in the interrupt handler
        case hal_gpio_interrupt_t::RISING_EDGE:
        {
            SYS_LOG_W(
                "RISING_EDGE interrupts are not directly supported by MCP23X17! Interrupts will be triggered on both edges, internal interrupt handler will filter the "
                "edges.");

            setInterruptControl = COMPARE_AGAINST_PREVIOUS_STATE; // Interrupt on any change
        }
        break;
        case hal_gpio_interrupt_t::FALLING_EDGE:
        {
            SYS_LOG_W(
                "FALLING_EDGE interrupts are not directly supported by MCP23X17! Interrupts will be triggered on both edges, internal interrupt handler will filter the "
                "edges.");

            setInterruptControl = COMPARE_AGAINST_PREVIOUS_STATE; // Interrupt on any change
        }
        break;
        case hal_gpio_interrupt_t::DISABLED:
        {
            SYS_LOG_I("Disabling interrupt");
            return ERROR_SUCCESS; // No interrupt to set
        }
        default:
            SYS_LOG_E("Invalid interrupt type");
            return ERROR_INVALID_ARG; // Invalid argument
            break;
    }

    // Set default value
    RETURN_ON_ERROR(
        _expander.setDefaultValueNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, setDefaultValue), // Set default value
    );

    // Configure interrupt
    RETURN_ON_ERROR(
        _expander.setInterruptControlNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, setInterruptControl), // Interrupt on change from DEFVAL
    );

    SYS_LOG_I("Configured interrupt for GPIO Expander Pin: %d on Port: %d", _halConfig.pinNumber, _halConfig.portNumber);

    return ERROR_SUCCESS;
}

sys_error_t io_gpio_expander::getInterrupt(hal_gpio_interrupt_t& interrupt)
{
    interrupt = _halConfig.interrupt;
    return ERROR_SUCCESS;
}

sys_error_t io_gpio_expander::setInterruptHandler(void (*handler)(void* params), void* params)
{
    _userInterruptHandler = handler;
    _userInterruptParams  = params;

    // Register with the expander's interrupt handler system
    return _expander.registerInputPinInterruptHandler(
        static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, &io_gpio_expander::internalInterruptHandler);
}

// Internal interrupt handler using INTCAP for edge detection
void io_gpio_expander::internalInterruptHandler(void* context)
{
    io_gpio_expander* _io_gpio = static_cast<io_gpio_expander*>(context);

    if (!_io_gpio)
    {
        return;
    }

    bool shouldTrigger = false;

    switch (_io_gpio->_halConfig.interrupt)
    {
        case hal_gpio_interrupt_t::RISING_EDGE:
        {
            // Read the captured value when interrupt occurred
            bool capturedValue = false;
            if (_io_gpio->_expander.getInterruptCapturedNo(static_cast<cpx_mcp23x17::PORT>(_io_gpio->_halConfig.portNumber), _io_gpio->_halConfig.pinNumber, capturedValue) ==
                ERROR_SUCCESS)
            {
                // Rising edge: captured value should be HIGH
                shouldTrigger = capturedValue;
                SYS_LOG_D("RISING_EDGE: captured=%d, trigger=%d", capturedValue, shouldTrigger);
            }
        }
        break;

        case hal_gpio_interrupt_t::FALLING_EDGE:
        {
            // Read the captured value when interrupt occurred
            bool capturedValue = false;
            if (_io_gpio->_expander.getInterruptCapturedNo(static_cast<cpx_mcp23x17::PORT>(_io_gpio->_halConfig.portNumber), _io_gpio->_halConfig.pinNumber, capturedValue) ==
                ERROR_SUCCESS)
            {
                // Falling edge: captured value should be LOW
                shouldTrigger = !capturedValue;
                SYS_LOG_D("FALLING_EDGE: captured=%d, trigger=%d", capturedValue, shouldTrigger);
            }
        }
        break;

        case hal_gpio_interrupt_t::BOTH_EDGES:
        case hal_gpio_interrupt_t::LOW_LEVEL:
        case hal_gpio_interrupt_t::HIGH_LEVEL:
        {
            SYS_LOG_D("Triggering for BOTH_EDGES or LEVEL interrupt");
            shouldTrigger = true;
        }
        break;

        default:
            break;
    }

    // Call user handler only if it's the desired edge/condition
    if (shouldTrigger)
    {
        SYS_LOG_D("Triggering user interrupt handler for pin %d", _io_gpio->_halConfig.pinNumber);

        if (_io_gpio->_userInterruptHandler)
            _io_gpio->_userInterruptHandler(_io_gpio->_userInterruptParams);
    }

    // Clear the interrupt by reading INTCAP (this is done automatically by the read above)
    // or read GPIO register to clear it
    _io_gpio->clearInterrupt();
}

sys_error_t io_gpio_expander::enableInterrupt()
{
    return _expander.setInterruptEnableNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, INTERRUPT_ON_CHANGE_ENABLED);
}

sys_error_t io_gpio_expander::disableInterrupt()
{
    return _expander.setInterruptEnableNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, INTERRUPT_ON_CHANGE_DISABLED);
}

sys_error_t io_gpio_expander::clearInterrupt()
{
    // Reading INTCAP clears the interrupt
    uint8_t dummy;
    return _expander.getInterruptCaptured(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), dummy);
}

sys_error_t io_gpio_expander::getLevel(hal_gpio_level_t& level)
{
    bool lvl = false;
    RETURN_ON_ERROR_WITH_LOG(
        _expander.getGpioNo(static_cast<cpx_mcp23x17::PORT>(_halConfig.portNumber), _halConfig.pinNumber, lvl), // Get level
        "Failed to get level",                                                                                  // Error Message
    );
    level = convertLevel(lvl ? 1 : 0);
    return ERROR_SUCCESS;
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
