#pragma once
#include "HAL\IHAL\IHal_Io_Gpio.h"

/**
 * @brief GPIO configuration presets for common use cases
 */
namespace GPIO_Presets
{
// ========== Basic Input Configurations ==========

static constexpr gpio_hal_config_t INPUT_FLOATING(uint16_t pin, uint8_t port = 0)
{
    return {
        .pinNumber        = pin,
        .portNumber       = port,
        .direction        = hal_gpio_direction_t::INPUT,
        .pull             = hal_gpio_pull_t::NONE,
        .interrupt        = hal_gpio_interrupt_t::DISABLED,
        .initial_level    = hal_gpio_level_t::LOW,
        .enable_interrupt = false};
}

static constexpr gpio_hal_config_t INPUT_PULLUP(uint16_t pin, uint8_t port = 0)
{
    return {
        .pinNumber        = pin,
        .portNumber       = port,
        .direction        = hal_gpio_direction_t::INPUT,
        .pull             = hal_gpio_pull_t::PULL_UP,
        .interrupt        = hal_gpio_interrupt_t::DISABLED,
        .initial_level    = hal_gpio_level_t::HIGH,
        .enable_interrupt = false};
}

static constexpr gpio_hal_config_t INPUT_PULLDOWN(uint16_t pin, uint8_t port = 0)
{
    return {
        .pinNumber        = pin,
        .portNumber       = port,
        .direction        = hal_gpio_direction_t::INPUT,
        .pull             = hal_gpio_pull_t::PULL_DOWN,
        .interrupt        = hal_gpio_interrupt_t::DISABLED,
        .initial_level    = hal_gpio_level_t::LOW,
        .enable_interrupt = false};
}

// ========== Basic Output Configurations ==========

static constexpr gpio_hal_config_t OUTPUT_LOW(uint16_t pin, uint8_t port = 0)
{
    return {
        .pinNumber        = pin,
        .portNumber       = port,
        .direction        = hal_gpio_direction_t::OUTPUT,
        .pull             = hal_gpio_pull_t::NONE,
        .interrupt        = hal_gpio_interrupt_t::DISABLED,
        .initial_level    = hal_gpio_level_t::LOW,
        .enable_interrupt = false};
}

static constexpr gpio_hal_config_t OUTPUT_HIGH(uint16_t pin, uint8_t port = 0)
{
    return {
        .pinNumber        = pin,
        .portNumber       = port,
        .direction        = hal_gpio_direction_t::OUTPUT,
        .pull             = hal_gpio_pull_t::NONE,
        .interrupt        = hal_gpio_interrupt_t::DISABLED,
        .initial_level    = hal_gpio_level_t::HIGH,
        .enable_interrupt = false};
}

// ========== Interrupt Configurations ==========

static constexpr gpio_hal_config_t INPUT_INTERRUPT(uint16_t pin, uint8_t port, hal_gpio_interrupt_t int_type, hal_gpio_pull_t pull = hal_gpio_pull_t::NONE)
{
    return {
        .pinNumber        = pin,
        .portNumber       = port,
        .direction        = hal_gpio_direction_t::INPUT,
        .pull             = pull,
        .interrupt        = int_type,
        .initial_level    = (pull == hal_gpio_pull_t::PULL_UP) ? hal_gpio_level_t::HIGH : hal_gpio_level_t::LOW,
        .enable_interrupt = true};
}

static constexpr gpio_hal_config_t INPUT_INTERRUPT_PULLUP(uint16_t pin, uint8_t port, hal_gpio_interrupt_t int_type, hal_gpio_pull_t pull = hal_gpio_pull_t::PULL_UP)
{
    return INPUT_INTERRUPT(pin, port, int_type, pull);
}

static constexpr gpio_hal_config_t INPUT_INTERRUPT_PULLDOWN(uint16_t pin, uint8_t port, hal_gpio_interrupt_t int_type, hal_gpio_pull_t pull = hal_gpio_pull_t::PULL_DOWN)
{
    return INPUT_INTERRUPT(pin, port, int_type, pull);
}

static constexpr gpio_hal_config_t INPUT_INTERRUPT_EXTERNAL_PULLUP(uint16_t pin, uint8_t port, hal_gpio_interrupt_t int_type)
{
    return INPUT_INTERRUPT(pin, port, int_type);
}

static constexpr gpio_hal_config_t INPUT_INTERRUPT_DETECT_BOTH(uint16_t pin, uint8_t port, hal_gpio_pull_t pull = hal_gpio_pull_t::PULL_UP)
{
    return INPUT_INTERRUPT(pin, port, hal_gpio_interrupt_t::BOTH_EDGES, pull);
}

// ========== Button Configurations ==========
static constexpr gpio_hal_config_t BUTTON_PULLUP(uint16_t pin, uint8_t port = 0)
{
    return INPUT_INTERRUPT_PULLUP(pin, port, hal_gpio_interrupt_t::FALLING_EDGE);
}

static constexpr gpio_hal_config_t BUTTON_PULLDOWN(uint16_t pin, uint8_t port = 0)
{
    return INPUT_INTERRUPT_PULLDOWN(pin, port, hal_gpio_interrupt_t::RISING_EDGE);
}

static constexpr gpio_hal_config_t BUTTON_EXTERNAL_PULLUP(uint16_t pin, uint8_t port = 0)
{
    return INPUT_INTERRUPT_EXTERNAL_PULLUP(pin, port, hal_gpio_interrupt_t::FALLING_EDGE);
}

static constexpr gpio_hal_config_t SWITCH_DETECT(uint16_t pin, uint8_t port = 0, hal_gpio_pull_t pull = hal_gpio_pull_t::PULL_UP)
{
    return INPUT_INTERRUPT(pin, port, hal_gpio_interrupt_t::BOTH_EDGES, pull);
}

// ========== LED Configurations ==========

static constexpr gpio_hal_config_t LED_OFF(uint16_t pin, uint8_t port = 0)
{
    return OUTPUT_LOW(pin);
}

static constexpr gpio_hal_config_t LED_ON(uint16_t pin, uint8_t port = 0)
{
    return OUTPUT_HIGH(pin);
}

static constexpr gpio_hal_config_t LED_ACTIVE_LOW(uint16_t pin, uint8_t port = 0, bool initially_on = false)
{
    return {
        .pinNumber        = pin,
        .portNumber       = port,
        .direction        = hal_gpio_direction_t::OUTPUT,
        .pull             = hal_gpio_pull_t::NONE,
        .interrupt        = hal_gpio_interrupt_t::DISABLED,
        .initial_level    = initially_on ? hal_gpio_level_t::LOW : hal_gpio_level_t::HIGH,
        .enable_interrupt = false};
}

// ========== Relay/Switch Configurations ==========

static constexpr gpio_hal_config_t RELAY_NORMALLY_OPEN(uint16_t pin, uint8_t port = 0)
{
    return OUTPUT_LOW(pin);
}

static constexpr gpio_hal_config_t RELAY_NORMALLY_CLOSED(uint16_t pin, uint8_t port = 0)
{
    return OUTPUT_HIGH(pin);
}

static constexpr gpio_hal_config_t MOSFET_SWITCH(uint16_t pin, uint8_t port = 0, bool initially_on = false)
{
    return {
        .pinNumber        = pin,
        .portNumber       = port,
        .direction        = hal_gpio_direction_t::OUTPUT,
        .pull             = hal_gpio_pull_t::PULL_DOWN, // Ensure clean off state
        .interrupt        = hal_gpio_interrupt_t::DISABLED,
        .initial_level    = initially_on ? hal_gpio_level_t::HIGH : hal_gpio_level_t::LOW,
        .enable_interrupt = false};
}

// ========== Sensor Configurations ==========

static constexpr gpio_hal_config_t MOTION_SENSOR(uint16_t pin, uint8_t port = 0)
{
    return INPUT_INTERRUPT(pin, port, hal_gpio_interrupt_t::RISING_EDGE, hal_gpio_pull_t::PULL_DOWN);
}

static constexpr gpio_hal_config_t DOOR_SENSOR(uint16_t pin, uint8_t port = 0)
{
    return INPUT_INTERRUPT(pin, port, hal_gpio_interrupt_t::BOTH_EDGES, hal_gpio_pull_t::PULL_UP);
}

static constexpr gpio_hal_config_t LIMIT_SWITCH(uint16_t pin, uint8_t port = 0)
{
    return INPUT_INTERRUPT(pin, port, hal_gpio_interrupt_t::BOTH_EDGES, hal_gpio_pull_t::PULL_UP);
}

static constexpr gpio_hal_config_t ENCODER_CHANNEL(uint16_t pin, uint8_t port = 0)
{
    return INPUT_INTERRUPT(pin, port, hal_gpio_interrupt_t::BOTH_EDGES, hal_gpio_pull_t::PULL_UP);
}

} // namespace GPIO_Presets