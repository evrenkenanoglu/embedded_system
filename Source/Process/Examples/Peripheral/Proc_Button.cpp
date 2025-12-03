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

Proc_Button::Proc_Button(std::unique_ptr<std::vector<BUTTON::Instance_t*>> buttons, uint32_t stackSize, int taskPriority)
    : _buttons(std::move(buttons)) // Move the unique pointer to the member variable
    , _stackSize(stackSize)        // Set stack size
    , _taskPriority(taskPriority)  // Set task priority
    , _taskHandle(nullptr)         // Initialize task handle

{
    setState(State::INITIALIZED);
}

Proc_Button::~Proc_Button()
{
    // destructor implementation
}

sys_error_t Proc_Button::start()
{
    // Validate buttons pointer
    RETURN_IF_ERROR(_buttons.get() == nullptr, ERROR_INIT_FAILED, SYS_LOG_D("Buttons pointer is null"));

    // Ensure process is in INITIALIZED or STOPPED state
    RETURN_IF_ERROR_WITH_LOG(
        getState() != State::INITIALIZED && getState() != State::STOPPED, // Expression
        ERROR_INVALID_STATE,                                              // Error Code
        "Process not in INITIALIZED state");                              // Error Message

    // Create the Button Listener
    BaseType_t result = xTaskCreate(
        buttonListener,         // Task function
        "button_listener_task", // Task name
        _stackSize,             // Stack size
        this,                   // Task parameter
        _taskPriority,          // Task priority
        &_taskHandle);          // Task handle

    RETURN_IF_ERROR_WITH_LOG(result != pdPASS, ERROR_FAIL, "Failed to create button listener task");

    return ERROR_SUCCESS;
}
sys_error_t Proc_Button::stop()
{
    // Stop the Button Listener
    vTaskDelete(_taskHandle);
    _taskHandle = nullptr;

    for (BUTTON::Instance_t* button : *_buttons)
    {
        // Delete the Button Listener
        buttonDataClear(button);
    }

    return ERROR_SUCCESS;
}

sys_error_t Proc_Button::pause()
{
    RETURN_IF_ERROR_WITH_LOG(getState() != State::RUNNING, ERROR_INVALID_STATE, "Process not in RUNNING state");

    vTaskSuspend(_taskHandle);

    return ERROR_SUCCESS;
}

sys_error_t Proc_Button::resume()
{
    RETURN_IF_ERROR_WITH_LOG(getState() != State::PAUSED, ERROR_INVALID_STATE, "Process not in PAUSED state");

    vTaskResume(_taskHandle);

    return ERROR_SUCCESS;
}

void Proc_Button::buttonListener(void* arg)
{
    // Validate argument
    RETURN_IF_ERROR(arg == nullptr, , SYS_LOG_E("Proc_Button argument is null"));

    // Get the Proc_Button instance
    Proc_Button* proc = static_cast<Proc_Button*>(arg);

    // Create queue set for all button queues
    QueueSetHandle_t queueSet = xQueueCreateSet(proc->_buttons->size() * IHAL_GPIO_EVENT_QUEUE_LENGTH);

    // Validate queue set creation
    RETURN_IF_ERROR(queueSet == nullptr, , SYS_LOG_E("Failed to create queue set for button listener"));

    // Add all button queues to the queue set
    for (BUTTON::Instance_t* button : *(proc->_buttons))
    {
        RETURN_IF_ERROR(
            xQueueAddToSet(reinterpret_cast<QueueHandle_t>(button->gpio.getEventQueue()), queueSet) != pdTRUE, // Expression
            ,                                                                                                  // Return void
            SYS_LOG_E("Failed to add button queue to queue set")                                               // Error Message
        );
    }

    for (;;)
    {
        // Wait indefinitely for the first event on any queue in the set.
        QueueHandle_t activeQueue = xQueueSelectFromSet(queueSet, portMAX_DELAY);
        // Find which button triggered
        for (BUTTON::Instance_t* button : *(proc->_buttons))
        {
            if (activeQueue == button->gpio.getEventQueue())
            {
                hal_gpio_event_t event;
                xQueueReceive(activeQueue, &event, 0);
                proc->processButtonEvent(button, &event);
                break;
            }
        }
    }
}

void Proc_Button::processButtonEvent(BUTTON::Instance_t* button, hal_gpio_event_t* event)
{
    RETURN_IF_ERROR((button == nullptr || event == nullptr), , SYS_LOG_E("Button instance is null"));

    BUTTON::EventData_t eventData;
    eventData.index                = button->index;
    QueueHandle_t buttonEventQueue = reinterpret_cast<QueueHandle_t>(button->eventQueue);

    // Get current time in ticks
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Button pressed and was not already pressed
    if (event->level == button->config.pressedState && !button->state.isPressed)
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
    else if (event->level != button->config.pressedState && button->state.isPressed)
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
    }
    // else: ignore very long presses
    // If no double click detected after window, reset
    if (button->state.waitingForSecondClick && (now - button->state.firstClickTime > button->config.doubleClickWindowMs))
    {
        button->state.waitingForSecondClick = false;
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