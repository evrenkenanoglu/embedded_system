#include "Proc_Button.hpp"
#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

namespace
{
} // namespace

/**
 * @brief Clear the queue for unwanted events
 *
 * @param xQueue
 */
static void clearQueue(QueueHandle_t xQueue);

Proc_Button::Proc_Button(std::unique_ptr<std::vector<BUTTON::Instance_t*>> buttons)
    : _buttons(std::move(buttons))
{
    setState(State::INITIALIZED);
}

Proc_Button::~Proc_Button()
{
    // destructor implementation
}

sys_error_t Proc_Button::start()
{
    SYS_LOG_D("STATE: %d", static_cast<int>(getState()));

    RETURN_IF_ERROR_WITH_LOG(
        getState() != State::INITIALIZED && getState() != State::STOPPED, // Expression
        ERROR_INVALID_STATE,                                              // Error Code
        "Process not in INITIALIZED state");                              // Error Message

    // Start the Button Listener
    for (BUTTON::Instance_t* button : *_buttons)
    {
        // Create the Button Listener
        BaseType_t result = xTaskCreate(
            buttonListener,                    // Task function
            "button_listener_task",            // Task name
            button->taskConfig.stackSize,      // Stack size
            static_cast<void*>(button),        // Task parameter
            button->taskConfig.taskPriority,   // Task priority
            &(button->taskConfig.taskHandle)); // Task handle

        RETURN_IF_ERROR_WITH_LOG(result != pdPASS, ERROR_FAIL, "Failed to create button listener task");
    }

    return ERROR_SUCCESS;
}
sys_error_t Proc_Button::stop()
{
    // Stop the Button Listener
    for (BUTTON::Instance_t* button : *_buttons)
    {
        // Delete the Button Listener
        vTaskDelete(button->taskConfig.taskHandle);
        button->taskConfig.taskHandle = nullptr;
        buttonDataClear(button);
    }

    return ERROR_SUCCESS;
}

sys_error_t Proc_Button::pause()
{
    RETURN_IF_ERROR_WITH_LOG(getState() != State::RUNNING, ERROR_INVALID_STATE, "Process not in RUNNING state");

    // Pause the Button Listener
    for (BUTTON::Instance_t* button : *_buttons)
    {
        // Suspend the Button Listener
        vTaskSuspend(button->taskConfig.taskHandle);
    }
    return ERROR_SUCCESS;
}

sys_error_t Proc_Button::resume()
{
    RETURN_IF_ERROR_WITH_LOG(getState() != State::PAUSED, ERROR_INVALID_STATE, "Process not in PAUSED state");

    for (BUTTON::Instance_t* button : *_buttons)
    {
        // Clear the Button Data
        buttonDataClear(button);

        // Resume the Button Listener
        vTaskResume(button->taskConfig.taskHandle);
    }
    return ERROR_SUCCESS;
}

void Proc_Button::buttonListener(void* arg)
{
    RETURN_IF_ERROR(arg == nullptr, );

    BUTTON::Instance_t* button           = static_cast<BUTTON::Instance_t*>(arg);
    QueueHandle_t       gpioEventQueue   = reinterpret_cast<QueueHandle_t>(button->gpio.getEventQueue());
    QueueHandle_t       buttonEventQueue = reinterpret_cast<QueueHandle_t>(button->eventQueue);
    hal_gpio_event_t    event;
    BUTTON::EventData_t eventData;
    eventData.index = button->index;
    uint32_t now    = 0;

    RETURN_IF_ERROR(gpioEventQueue == nullptr || buttonEventQueue == nullptr, );

    SYS_LOG_D("Waiting for button to be pressed!\n");
    for (;;)
    {
        if (xQueueReceive(gpioEventQueue, &event, portMAX_DELAY))
        {
            now = xTaskGetTickCount() * portTICK_PERIOD_MS;

            // Button pressed and was not already pressed
            if (event.level == button->config.pressedState && !button->state.isPressed)
            {
                button->state.isPressed = true;
                button->state.pressTime = now;

                // Double click detection
                if (button->state.waitingForSecondClick && (now - button->state.firstClickTime <= button->config.doubleClickWindowMs))
                {
                    eventData.event             = BUTTON::Event::DOUBLE_CLICK;
                    eventData.pressedDurationMs = 0; // Duration is not relevant for double click
                    xQueueSend(buttonEventQueue, &eventData, 0);
                    button->state.waitingForSecondClick = false;
                }
                else // First click
                {
                    button->state.firstClickTime        = now;
                    button->state.waitingForSecondClick = true;
                }

                eventData.event             = BUTTON::Event::PRESSED;
                eventData.pressedDurationMs = 0; // Duration is not relevant for press event
                xQueueSend(buttonEventQueue, &eventData, 0);
            }
            // Button released and was previously pressed
            else if (event.level != button->config.pressedState && button->state.isPressed)
            {
                button->state.isPressed   = false;
                button->state.releaseTime = now;
                uint32_t duration         = button->state.releaseTime - button->state.pressTime;

                eventData.event             = BUTTON::Event::RELEASED;
                eventData.pressedDurationMs = duration;
                xQueueSend(buttonEventQueue, &eventData, 0);

                if (duration < button->config.shortPressThresholdMs)
                {

                    eventData.event = BUTTON::Event::SHORT_PRESS;
                    xQueueSend(buttonEventQueue, &eventData, 0);
                }
                else if (duration < button->config.longPressThresholdMs)
                {
                    eventData.event = BUTTON::Event::LONG_PRESS;
                    xQueueSend(buttonEventQueue, &eventData, 0);
                }
                // else: ignore very long presses

                // If no double click detected after window, reset
                if (button->state.waitingForSecondClick && (now - button->state.firstClickTime > button->config.doubleClickWindowMs))
                {
                    button->state.waitingForSecondClick = false;
                }
            }
        }
    }
}

void Proc_Button::buttonDataClear(BUTTON::Instance_t* button)
{
    RETURN_IF_ERROR(button == nullptr, );

    button->state = BUTTON::State_t{}; // Reset the button state

    // Clear the queue for unwanted events
    clearQueue(reinterpret_cast<QueueHandle_t>(button->gpio.getEventQueue()));
}

static void clearQueue(QueueHandle_t xQueue)
{
    BaseType_t xQueueStatus;
    uint32_t   buffer;

    // Continue receiving items from the queue until it's empty
    do
    {
        xQueueStatus = xQueueReceive(xQueue, &buffer, 0);
    } while (xQueueStatus == pdPASS);
}