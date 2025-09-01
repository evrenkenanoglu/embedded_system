/** @file       ihal.h
 *  @brief      Hardware Abstraction Layer Interface
 *  @copyright  (c) 2023- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       12/02/2023
 */
#ifndef FILE_IHAL_EXTENSION_H
#define FILE_IHAL_EXTENSION_H

#include "IHal.h"
#include "System/system.h"

#define IHAL_GPIO_EVENT_QUEUE_LENGTH 4

/**
 * @brief GPIO direction enumeration
 */
enum class hal_gpio_direction_t : uint8_t
{
    INPUT        = 0,
    OUTPUT       = 1,
    INPUT_OUTPUT = 2 // For bidirectional pins
};

/**
 * @brief GPIO pull-up/pull-down configuration
 */
enum class hal_gpio_pull_t : uint8_t
{
    NONE      = 0,
    PULL_UP   = 1,
    PULL_DOWN = 2
};

/**
 * @brief GPIO interrupt trigger types
 */
enum class hal_gpio_interrupt_t : uint8_t
{
    DISABLED     = 0,
    RISING_EDGE  = 1,
    FALLING_EDGE = 2,
    BOTH_EDGES   = 3,
    LOW_LEVEL    = 4,
    HIGH_LEVEL   = 5
};

/**
 * @brief GPIO logic levels
 */
enum class hal_gpio_level_t : uint8_t
{
    LOW  = 0,
    HIGH = 1
};

/**
 * @brief GPIO event structure for interrupt handling
 */
typedef struct
{
    uint32_t         gpio_num;
    hal_gpio_level_t level;
    uint32_t         timestamp_ms;
} hal_gpio_event_t;

/**
 * @brief Platform-independent GPIO configuration structure
 */
typedef struct
{
    uint32_t             pinNumber;        // GPIO pin number
    hal_gpio_direction_t direction;        // GPIO direction (input/output/bidirectional)
    hal_gpio_pull_t      pull;             // Pull resistor configuration
    hal_gpio_interrupt_t interrupt;        // Interrupt trigger type
    hal_gpio_level_t     initial_level;    // Initial level for output pins
    bool                 enable_interrupt; // Enable interrupt on initialization
} gpio_hal_config_t;

/**
 * @class IHAL_IO_GPIO
 * @brief Interface for Hardware Abstraction Layer (HAL) GPIO operations.
 */
class IHAL_IO_GPIO : public IHAL_IO
{
public:
    /**
     * @brief Set GPIO pin direction
     * @param direction GPIO direction (input/output/bidirectional)
     * @return sys_error_t Error code
     */
    virtual sys_error_t setDirection(hal_gpio_direction_t direction) = 0;

    /**
     * @brief Get GPIO pin direction
     * @param direction Reference to store current direction
     * @return sys_error_t Error code
     */
    virtual sys_error_t getDirection(hal_gpio_direction_t& direction) = 0;

    /**
     * @brief Set GPIO pull resistor configuration
     * @param pull Pull resistor configuration
     * @return sys_error_t Error code
     */
    virtual sys_error_t setPull(hal_gpio_pull_t pull) = 0;

    /**
     * @brief Get GPIO pull resistor configuration
     * @param pull Reference to store current pull configuration
     * @return sys_error_t Error code
     */
    virtual sys_error_t getPull(hal_gpio_pull_t& pull) = 0;

    /**
     * @brief Set GPIO interrupt configuration
     * @param interrupt Interrupt trigger type
     * @return sys_error_t Error code
     */
    virtual sys_error_t setInterrupt(hal_gpio_interrupt_t interrupt) = 0;

    /**
     * @brief Get GPIO interrupt configuration
     * @param interrupt Reference to store current interrupt configuration
     * @return sys_error_t Error code
     */
    virtual sys_error_t getInterrupt(hal_gpio_interrupt_t& interrupt) = 0;

    /**
     * @brief Register interrupt handler callback
     * @param handler Function pointer to interrupt handler
     * @param params Parameters to pass to the handler
     * @return sys_error_t Error code
     */
    virtual sys_error_t setInterruptHandler(void (*handler)(void* params), void* params) = 0;

    /**
     * @brief Enable GPIO interrupt
     * @return sys_error_t Error code
     */
    virtual sys_error_t enableInterrupt() = 0;

    /**
     * @brief Disable GPIO interrupt
     * @return sys_error_t Error code
     */
    virtual sys_error_t disableInterrupt() = 0;

    /**
     * @brief Clear pending interrupt flag
     * @return sys_error_t Error code
     */
    virtual sys_error_t clearInterrupt() = 0;

    /**
     * @brief Get GPIO pin logic level
     * @param level Reference to store current level
     * @return sys_error_t Error code
     */
    virtual sys_error_t getLevel(hal_gpio_level_t& level) = 0;

    /**
     * @brief Set GPIO pin logic level (for output pins)
     * @param level Logic level to set
     * @return sys_error_t Error code
     */
    virtual sys_error_t setLevel(hal_gpio_level_t level) = 0;

    /**
     * @brief Toggle GPIO pin logic level (for output pins)
     * @return sys_error_t Error code
     */
    virtual sys_error_t toggleLevel() = 0;

    /**
     * @brief Get event queue handle for interrupt events
     * @return void* Pointer to event queue handle
     */
    virtual void* getEventQueue() = 0;

    /**
     * @brief Get GPIO number/identifier
     * @return uint32_t GPIO number
     */
    virtual uint32_t getGpioNumber() const = 0;

    /**
     * @brief Check if GPIO supports specific capability
     * @param capability Capability to check (platform-specific)
     * @return bool True if supported, false otherwise
     */
    virtual bool hasCapability(uint32_t capability) const = 0;

    // /**
    //  * @brief Get GPIO configuration as a HAL configuration structure
    //  * @param config Pointer to gpio_hal_config_t structure to store current configuration
    //  * @return sys_error_t Error code
    //  */
    // virtual sys_error_t getConfiguration(gpio_hal_config_t* config) const = 0;

    // /**
    //  * @brief Set GPIO configuration from a HAL configuration structure
    //  * @param config Pointer to gpio_hal_config_t structure containing new configuration
    //  * @return sys_error_t Error code
    //  */
    // virtual sys_error_t setConfiguration(const gpio_hal_config_t* config) = 0;

    /**
     * @brief Virtual destructor
     */
    virtual ~IHAL_IO_GPIO() = default;
};

#undef INTERFACE // Should not let this roam free

#endif // FILE_IHAL_H
