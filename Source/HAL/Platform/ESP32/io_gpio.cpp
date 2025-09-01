/**
 * @file io_gpio.cpp
 * @brief Source file for io_gpio
 *
 * This file contains definitions for the io_gpio class and related data types and functions.
 */

#include "io_gpio.hpp"
#include "System/LogHandler.h"
#include "esp_log.h"
#include <map>

static TimerHandle_t gpioTimers[GPIO_PIN_COUNT] = {nullptr};
namespace
{
const uint32_t gpioIntBlockTime = 50; // ms
// Helper functions to convert between enum types and ESP32 types

gpio_mode_t convertDirection(hal_gpio_direction_t direction)
{
    switch (direction)
    {
        case hal_gpio_direction_t::INPUT:
            return GPIO_MODE_INPUT;
        case hal_gpio_direction_t::OUTPUT:
            return GPIO_MODE_OUTPUT;
        case hal_gpio_direction_t::INPUT_OUTPUT:
            return GPIO_MODE_INPUT_OUTPUT;
        default:
            return GPIO_MODE_INPUT;
    }
}

hal_gpio_direction_t convertDirection(gpio_mode_t mode)
{
    switch (mode)
    {
        case GPIO_MODE_INPUT:
            return hal_gpio_direction_t::INPUT;
        case GPIO_MODE_OUTPUT:
            return hal_gpio_direction_t::OUTPUT;
        case GPIO_MODE_INPUT_OUTPUT:
            return hal_gpio_direction_t::INPUT_OUTPUT;
        case GPIO_MODE_OUTPUT_OD:
            return hal_gpio_direction_t::OUTPUT;
        default:
            return hal_gpio_direction_t::INPUT;
    }
}

gpio_pull_mode_t convertPull(hal_gpio_pull_t pull)
{
    switch (pull)
    {
        case hal_gpio_pull_t::NONE:
            return GPIO_FLOATING;
        case hal_gpio_pull_t::PULL_UP:
            return GPIO_PULLUP_ONLY;
        case hal_gpio_pull_t::PULL_DOWN:
            return GPIO_PULLDOWN_ONLY;
        default:
            return GPIO_FLOATING;
    }
}

hal_gpio_pull_t convertPull(gpio_pull_mode_t pullMode)
{
    switch (pullMode)
    {
        case GPIO_PULLUP_ONLY:
            return hal_gpio_pull_t::PULL_UP;
        case GPIO_PULLDOWN_ONLY:
            return hal_gpio_pull_t::PULL_DOWN;
        case GPIO_FLOATING:
            return hal_gpio_pull_t::NONE;
        default:
            return hal_gpio_pull_t::NONE;
    }
}

gpio_int_type_t convertInterrupt(hal_gpio_interrupt_t interrupt)
{
    switch (interrupt)
    {
        case hal_gpio_interrupt_t::DISABLED:
            return GPIO_INTR_DISABLE;
        case hal_gpio_interrupt_t::RISING_EDGE:
            return GPIO_INTR_POSEDGE;
        case hal_gpio_interrupt_t::FALLING_EDGE:
            return GPIO_INTR_NEGEDGE;
        case hal_gpio_interrupt_t::BOTH_EDGES:
            return GPIO_INTR_ANYEDGE;
        case hal_gpio_interrupt_t::LOW_LEVEL:
            return GPIO_INTR_LOW_LEVEL;
        case hal_gpio_interrupt_t::HIGH_LEVEL:
            return GPIO_INTR_HIGH_LEVEL;
        default:
            return GPIO_INTR_DISABLE;
    }
}

hal_gpio_interrupt_t convertInterrupt(gpio_int_type_t intType)
{
    switch (intType)
    {
        case GPIO_INTR_DISABLE:
            return hal_gpio_interrupt_t::DISABLED;
        case GPIO_INTR_POSEDGE:
            return hal_gpio_interrupt_t::RISING_EDGE;
        case GPIO_INTR_NEGEDGE:
            return hal_gpio_interrupt_t::FALLING_EDGE;
        case GPIO_INTR_ANYEDGE:
            return hal_gpio_interrupt_t::BOTH_EDGES;
        case GPIO_INTR_LOW_LEVEL:
            return hal_gpio_interrupt_t::LOW_LEVEL;
        case GPIO_INTR_HIGH_LEVEL:
            return hal_gpio_interrupt_t::HIGH_LEVEL;
        default:
            return hal_gpio_interrupt_t::DISABLED;
    }
}

gpio_config_t convertToESP32Config(const gpio_hal_config_t& halConfig)
{
    gpio_config_t esp32Config = {};

    esp32Config.pin_bit_mask = (1ULL << halConfig.pinNumber);
    esp32Config.mode         = convertDirection(halConfig.direction);
    esp32Config.intr_type    = convertInterrupt(halConfig.interrupt);

    // Handle pull resistor configuration
    switch (halConfig.pull)
    {
        case hal_gpio_pull_t::PULL_UP:
            esp32Config.pull_up_en   = GPIO_PULLUP_ENABLE;
            esp32Config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case hal_gpio_pull_t::PULL_DOWN:
            esp32Config.pull_up_en   = GPIO_PULLUP_DISABLE;
            esp32Config.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
        case hal_gpio_pull_t::NONE:
        default:
            esp32Config.pull_up_en   = GPIO_PULLUP_DISABLE;
            esp32Config.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
    }

    return esp32Config;
}
} // namespace

/**
 * @brief Timer callback for GPIO debouncing
 */
static void gpioTimerCallback(TimerHandle_t xTimer)
{
    io_gpio* gpioClass = static_cast<io_gpio*>(pvTimerGetTimerID(xTimer));
    if (gpioClass == nullptr)
        return;

    // Get current level and timestamp
    hal_gpio_level_t currentLevel;
    gpioClass->getLevel(currentLevel);

    // Create event structure
    hal_gpio_event_t event = {
        .gpio_num     = gpioClass->getGpioNumber(),              // Gpio Number
        .level        = currentLevel,                            // Current Level
        .timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS // Timestamp in ms
    };

    // printf("GPIO[%d] intr, val: %d\n", static_cast<int>(event.gpio_num), static_cast<int>(currentLevel));

    // Send event to queue
    BaseType_t queueResult = xQueueSendFromISR(
        reinterpret_cast<QueueHandle_t>(gpioClass->getEventQueue()),
        &event,
        0 // No block time
    );

    // Re-enable the GPIO interrupt
    gpio_intr_enable(static_cast<gpio_num_t>(event.gpio_num));

    (void)queueResult; // Suppress unused variable warning
}

/**
 * @brief Internal GPIO interrupt handler for event queue
 * This function is called when a GPIO interrupt is triggered and no custom handler is set.
 * It handles debouncing and adds events to the event queue.
 *
 * @param arg Pointer to the io_gpio object
 */
static void IRAM_ATTR isrHandler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    io_gpio*   gpioClass                = static_cast<io_gpio*>(arg);

    if (gpioClass == nullptr)
        return;

    uint32_t gpioNumber = gpioClass->getGpioNumber();
    gpio_intr_disable(static_cast<gpio_num_t>(gpioNumber));

    // Get timer handle for this GPIO
    TimerHandle_t timerHandle = gpioTimers[gpioNumber];

    // Start or restart the timer from ISR
    if (timerHandle != nullptr)
    {
        if (xTimerStartFromISR(timerHandle, &xHigherPriorityTaskWoken) != pdPASS)
        {
            // Timer start failed - re-enable interrupt immediately
            gpio_intr_enable(static_cast<gpio_num_t>(gpioNumber));
        }
    }
    else
    {
        // No timer available - re-enable interrupt immediately
        gpio_intr_enable(static_cast<gpio_num_t>(gpioNumber));
    }

    // Yield to higher priority task if necessary
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

io_gpio::io_gpio(gpio_hal_config_t& config)
    : _halConfig(config)
    , _gpioEventQueue(xQueueCreate(IHAL_GPIO_EVENT_QUEUE_LENGTH, sizeof(hal_gpio_event_t)))
    , _counter(0)
{

    // Create timer during construction if interrupts are enabled
    if (_halConfig.enable_interrupt && _halConfig.interrupt != hal_gpio_interrupt_t::DISABLED && _halConfig.pinNumber < GPIO_PIN_COUNT)
    {
        // Create timer name with GPIO number
        char timerName[20];
        snprintf(timerName, sizeof(timerName), "gpio_tmr_%d", static_cast<int>(_halConfig.pinNumber));

        TimerHandle_t timerHandle = xTimerCreate(
            timerName,                       // Timer name
            pdMS_TO_TICKS(gpioIntBlockTime), // Timer period in ticks
            pdFALSE,                         // One-shot timer
            (void*)this,                     // Timer ID is pointer to this object
            gpioTimerCallback);              // Timer callback

        // Store timer in array for fast ISR access
        if (timerHandle != nullptr)
        {
            gpioTimers[_halConfig.pinNumber] = timerHandle;
        }
    }
}

sys_error_t io_gpio::init(void* params)
{
    UNUSED(params);

    // Convert HAL config to ESP32 config
    gpio_config_t esp32Config = convertToESP32Config(_halConfig);

    esp_err_t result = gpio_config(&esp32Config);
    if (result != ESP_OK)
    {
        SYS_LOG_E("GPIO config failed: %s", esp_err_to_name(result));
        return ERROR_INIT_FAILED;
    }

    SYS_LOG_D("GPIO %d initialized", _halConfig.pinNumber);
    // Set initial level for output pins
    if (_halConfig.direction == hal_gpio_direction_t::OUTPUT)
    {
        gpio_set_level(static_cast<gpio_num_t>(_halConfig.pinNumber), (_halConfig.initial_level == hal_gpio_level_t::HIGH) ? 1 : 0);
    }

    SYS_LOG_D("Initial level set for GPIO %d", _halConfig.pinNumber);

    // Configure interrupts if needed
    if (_halConfig.enable_interrupt && _halConfig.interrupt != hal_gpio_interrupt_t::DISABLED)
    {

        SYS_LOG_D("Configuring interrupt for GPIO %d", _halConfig.pinNumber);
        gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
        if (_interruptHandler != nullptr)
        {
            gpio_isr_handler_add(static_cast<gpio_num_t>(_halConfig.pinNumber), _interruptHandler, _handlerParams);
        }
        else
        {
            gpio_isr_handler_add(static_cast<gpio_num_t>(_halConfig.pinNumber), isrHandler, (void*)this);
        }
    }

    SYS_LOG_D("GPIO %d configured", _halConfig.pinNumber);

    return ERROR_SUCCESS;
}

io_gpio::~io_gpio()
{
    deInit();
}

void io_gpio::get(void* data)
{
    RETURN_IF_ERROR(
        data == nullptr, // Expression
        void()           // Return value (void)
    );

    *reinterpret_cast<int*>(data) = gpio_get_level(static_cast<gpio_num_t>(_halConfig.pinNumber));
}

sys_error_t io_gpio::set(void* data)
{
    if (_halConfig.direction == hal_gpio_direction_t::INPUT)
    {
        SYS_LOG_W("Gpio is an input type! Level can not be set!");
        return ERROR_FAIL;
    }

    if (data == nullptr)
    {
        return ERROR_INVALID_ARG;
    }

    esp_err_t result = gpio_set_level(static_cast<gpio_num_t>(_halConfig.pinNumber), *(uint8_t*)data);
    return (result == ESP_OK) ? ERROR_SUCCESS : ERROR_FAIL;
}

sys_error_t io_gpio::deInit()
{
    if (hal_gpio_direction_t::INPUT == _halConfig.direction)
    {
        // Remove ISR handler for the GPIO number
        gpio_isr_handler_remove(static_cast<gpio_num_t>(_halConfig.pinNumber));
    }

    // Delete the event queue
    if (_gpioEventQueue != NULL)
    {
        vQueueDelete(_gpioEventQueue);
        _gpioEventQueue = NULL;
    }

    return ERROR_SUCCESS;
}

// IHAL_IO_GPIO specific methods implementation
sys_error_t io_gpio::getDirection(hal_gpio_direction_t& direction)
{
    direction = _halConfig.direction; // Instead of _currentDirection
    return ERROR_SUCCESS;
}

sys_error_t io_gpio::setDirection(hal_gpio_direction_t direction)
{
    gpio_mode_t mode = convertDirection(direction);

    RETURN_IF_ERROR_WITH_LOG(
        gpio_set_direction(static_cast<gpio_num_t>(_halConfig.pinNumber), mode) != ESP_OK, // Expression
        ERROR_FAIL,                                                                        // Error code
        "Failed to set GPIO direction"                                                     // Log message
    );

    _halConfig.direction = direction; // Update HAL config directly

    return ERROR_SUCCESS;
}

sys_error_t io_gpio::setPull(hal_gpio_pull_t pull)
{
    gpio_pull_mode_t platformPull = convertPull(pull);

    RETURN_IF_ERROR_WITH_LOG(
        gpio_set_pull_mode(static_cast<gpio_num_t>(_halConfig.pinNumber), platformPull) != ESP_OK, // Expression
        ERROR_FAIL,                                                                                // Error code
        "Failed to set GPIO pull mode"                                                             // Log message
    );

    _halConfig.pull = pull; // Update HAL config, not cached variable

    return ERROR_SUCCESS;
}

sys_error_t io_gpio::getPull(hal_gpio_pull_t& pull)
{
    pull = _halConfig.pull; // Not _currentPull
    return ERROR_SUCCESS;
}

sys_error_t io_gpio::setInterruptHandler(void (*handler)(void* params), void* params)
{
    _interruptHandler = handler;
    _handlerParams    = params;

    // If GPIO is already initialized and configured for interrupts, update the ISR handler
    if (_halConfig.enable_interrupt && _halConfig.interrupt != hal_gpio_interrupt_t::DISABLED)
    {
        gpio_isr_handler_remove(static_cast<gpio_num_t>(_halConfig.pinNumber));

        esp_err_t result;
        if (_interruptHandler != nullptr)
        {
            result = gpio_isr_handler_add(static_cast<gpio_num_t>(_halConfig.pinNumber), _interruptHandler, _handlerParams);
        }
        else
        {
            result = gpio_isr_handler_add(static_cast<gpio_num_t>(_halConfig.pinNumber), isrHandler, (void*)this);
        }

        return (result == ESP_OK) ? ERROR_SUCCESS : ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t io_gpio::getInterrupt(hal_gpio_interrupt_t& interrupt)
{
    interrupt = _halConfig.interrupt; // Not _currentInterrupt
    return ERROR_SUCCESS;
}

sys_error_t io_gpio::enableInterrupt()
{
    esp_err_t result = gpio_intr_enable(static_cast<gpio_num_t>(_halConfig.pinNumber));
    return (result == ESP_OK) ? ERROR_SUCCESS : ERROR_FAIL;
}

sys_error_t io_gpio::disableInterrupt()
{
    esp_err_t result = gpio_intr_disable(static_cast<gpio_num_t>(_halConfig.pinNumber));
    return (result == ESP_OK) ? ERROR_SUCCESS : ERROR_FAIL;
}

sys_error_t io_gpio::clearInterrupt()
{
    // ESP32 doesn't have a direct clear interrupt function for GPIO
    // The interrupt flag is automatically cleared when the ISR is serviced
    return ERROR_SUCCESS;
}

sys_error_t io_gpio::getLevel(hal_gpio_level_t& level)
{
    int gpioLevel = gpio_get_level(static_cast<gpio_num_t>(_halConfig.pinNumber));
    level         = (gpioLevel == 1) ? hal_gpio_level_t::HIGH : hal_gpio_level_t::LOW;
    return ERROR_SUCCESS;
}

sys_error_t io_gpio::setLevel(hal_gpio_level_t level)
{
    RETURN_IF_ERROR_WITH_LOG(
        (_halConfig.direction == hal_gpio_direction_t::INPUT), // Expression
        ERROR_NOT_SUPPORTED,                                   // Error code
        "Cannot set level on input GPIO"                       // Log message
    );

    int       gpioLevel = (level == hal_gpio_level_t::HIGH) ? 1 : 0;
    esp_err_t result    = gpio_set_level(static_cast<gpio_num_t>(_halConfig.pinNumber), gpioLevel);
    return (result == ESP_OK) ? ERROR_SUCCESS : ERROR_FAIL;
}

sys_error_t io_gpio::toggleLevel()
{
    RETURN_IF_ERROR_WITH_LOG(
        (_halConfig.direction == hal_gpio_direction_t::INPUT), // Expression
        ERROR_NOT_SUPPORTED,                                   // Error code
        "Cannot toggle level on input GPIO!"                   // Log message
    );

    hal_gpio_level_t currentLevel;
    RETURN_ON_ERROR(getLevel(currentLevel));

    hal_gpio_level_t newLevel = (currentLevel == hal_gpio_level_t::HIGH) ? hal_gpio_level_t::LOW : hal_gpio_level_t::HIGH;
    return setLevel(newLevel);
}

sys_error_t io_gpio::setInterrupt(hal_gpio_interrupt_t interrupt)
{
    gpio_int_type_t intType = convertInterrupt(interrupt);

    RETURN_IF_ERROR_WITH_LOG(
        gpio_set_intr_type(static_cast<gpio_num_t>(_halConfig.pinNumber), intType) != ESP_OK, // Expression
        ERROR_FAIL,                                                                           // Error code
        "Failed to set GPIO interrupt type"                                                   // Log message
    );

    _halConfig.interrupt = interrupt;

    return ERROR_SUCCESS;
}

void* io_gpio::getEventQueue()
{
    return (void*)_gpioEventQueue;
}

uint32_t io_gpio::getGpioNumber() const
{
    return static_cast<uint32_t>(_halConfig.pinNumber);
}

bool io_gpio::hasCapability(uint32_t capability) const
{
    // Not implemented yet
    return false;
}