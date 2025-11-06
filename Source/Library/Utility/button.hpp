#pragma once

#include "HAL/IHAL/IHal_Io_Gpio.h"
#include "System/system.h"

namespace BUTTON
{

constexpr uint32_t LONG_PRESS_THRESHOLD_MS  = 1000; // 2 seconds
constexpr uint32_t SHORT_PRESS_THRESHOLD_MS = 500;  // 500 milliseconds
constexpr uint32_t DOUBLE_CLICK_WINDOW_MS   = 500;  // 500 milliseconds
constexpr uint32_t EVENT_QUEUE_LENGTH       = 10;   // Length of the event queue

enum class Event
{
    PRESSED,
    RELEASED,
    SHORT_PRESS,  // less than 500 ms
    LONG_PRESS,   // more than 500 ms and less than 2 seconds
    DOUBLE_CLICK, // two presses within 500 ms
    MAX
};

typedef struct
{
    uint16_t index;             // Button index
    Event    event;             // Event type
    uint32_t pressedDurationMs; // Duration the button was pressed (for PRESS and RELEASE events)
} EventData_t;

typedef struct
{
    uint32_t     stackSize    = 2048;    // Stack size for the button listener task
    uint8_t      taskPriority = 10;      // Task priority
    TaskHandle_t taskHandle   = nullptr; // Task handle for the button listener task
} TaskConfig_t;

typedef struct
{
    uint16_t         longPressThresholdMs  = LONG_PRESS_THRESHOLD_MS;
    uint16_t         shortPressThresholdMs = SHORT_PRESS_THRESHOLD_MS;
    uint16_t         doubleClickWindowMs   = DOUBLE_CLICK_WINDOW_MS;
    hal_gpio_level_t pressedState          = hal_gpio_level_t::LOW; // Active low by default
} Config_t;

typedef struct
{
    uint32_t pressTime             = 0;
    uint32_t releaseTime           = 0;
    bool     isPressed             = false;
    bool     waitingForSecondClick = false;
    uint32_t firstClickTime        = 0;
} State_t;

typedef struct
{
    uint16_t      index;
    IHAL_IO_GPIO& gpio;
    Config_t      config;
    State_t       state;
    TaskConfig_t  taskConfig;
    QueueHandle_t eventQueue = nullptr;
} Instance_t;

// Create a configuration with default values
static constexpr Config_t DEFAULT_CONFIG(
    uint16_t longPressThresholdMs = LONG_PRESS_THRESHOLD_MS, uint16_t shortPressThresholdMs = SHORT_PRESS_THRESHOLD_MS, uint16_t doubleClickWindowMs = DOUBLE_CLICK_WINDOW_MS,
    hal_gpio_level_t pressedState = hal_gpio_level_t::LOW)
{
    return Config_t{
        .longPressThresholdMs  = LONG_PRESS_THRESHOLD_MS,
        .shortPressThresholdMs = SHORT_PRESS_THRESHOLD_MS,
        .doubleClickWindowMs   = DOUBLE_CLICK_WINDOW_MS,
        .pressedState          = hal_gpio_level_t::LOW, // Active low by default
    };
}

/**
 * @brief Create a button instance with internal event queue
 *
 * Creates a button instance with its own event queue.
 *
 * @param index Unique index for the button
 * @param gpio Reference to the GPIO interface
 * @param config Button configuration
 * @param taskConfig Task configuration
 * @param eventQueueLength Length of the event queue
 * @return constexpr Instance_t
 */
inline Instance_t
CREATE_INSTANCE(uint16_t index, IHAL_IO_GPIO& gpio, Config_t config = Config_t(), TaskConfig_t taskConfig = TaskConfig_t(), uint32_t eventQueueLength = EVENT_QUEUE_LENGTH)
{
    return Instance_t{
        .index      = index,                                               // Unique index for the button
        .gpio       = gpio,                                                // Reference to the GPIO interface
        .config     = config,                                              // Button configuration
        .state      = State_t{},                                           // Initial state
        .taskConfig = taskConfig,                                          // Task configuration
        .eventQueue = xQueueCreate(eventQueueLength, sizeof(EventData_t)), // Event queue
    };
}

/**
 * @brief Create a instance ext que object with external event queue
 *
 * @param index Unique index for the button
 * @param gpio Reference to the GPIO interface
 * @param config Button configuration
 * @param taskConfig Task configuration
 * @param eventQueue External event queue handle or nullptr to create internal queue
 * @return constexpr Instance_t
 */
inline Instance_t
CREATE_INSTANCE_EXT_QUE(uint16_t index, IHAL_IO_GPIO& gpio, QueueHandle_t eventQueue = nullptr, Config_t config = Config_t(), TaskConfig_t taskConfig = TaskConfig_t())
{
    return Instance_t{
        .index      = index,      // Unique index for the butFton
        .gpio       = gpio,       // Reference to the GPIO interface
        .config     = config,     // Button configuration
        .state      = State_t{},  // Initial state
        .taskConfig = taskConfig, // Task configuration
        .eventQueue = eventQueue, // Event queue
    };
}
} // namespace BUTTON