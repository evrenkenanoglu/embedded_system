/**
 * @file Proc_Button.cpp
 * @brief Source file for Proc_Button
 *
 * This file contains definitions for the Proc_Button class and related data types and functions.
 */

#include "Proc_Button.hpp"
#include <inttypes.h>

namespace
{
} // namespace

/**
 * @brief Button Listener
 * This task is responsible for processing the GPIO events
 * It reads the GPIO input and calculates the duration of the button press
 * @param arg
 *
 * @details
 * | prevState | currentState | pressedState | Description |
 * |-----------|--------------|--------------|-------------|
 * | 0         | 0            | 0            | The button was pressed and remains pressed. |
 * | 0         | 1            | 0            | The button was pressed and now it's not pressed. The `changeTime` is updated and the duration of the pressed state is calculated and printed. |
 * | 1         | 1            | 0            | The button was not pressed and remains not pressed. |
 * | 1         | 0            | 0            | The button was not pressed and now it's pressed. The `changeTime` is updated. |
 */
static void buttonListener(void* arg);

/**
 * @brief Clear the queue for unwanted events
 *
 * @param xQueue
 */
static void clearQueue(QueueHandle_t xQueue);

Proc_Button::Proc_Button(std::vector<buttonData*>& buttons, uint32_t stackSize, uint8_t taskPriority) : _buttons(buttons), _stackSize(stackSize), _taskPriority(taskPriority)
{
    // constructor implementation
}

Proc_Button::~Proc_Button()
{
    // destructor implementation
}

sys_error_t Proc_Button::start()
{
    // Start the Button Listener
    for (buttonData* button : _buttons)
    {
        // Create the Button Listener
        xTaskCreate(buttonListener,             // Task function
                    "button_listener_task",     // Task name
                    _stackSize,                 // Stack size
                    static_cast<void*>(button), // Task parameter
                    _taskPriority,              // Task priority
                    &(button->taskHandle));     // Task handle
    }
    return ERROR_SUCCESS;
}
sys_error_t Proc_Button::stop()
{
    // Stop the Button Listener
    for (buttonData* button : _buttons)
    {
        // Delete the Button Listener
        vTaskDelete(button->taskHandle);
        button->taskHandle = NULL;
        buttonDataClear(button);
    }

    return ERROR_SUCCESS;
}

sys_error_t Proc_Button::pause()
{
    // Pause the Button Listener
    for (buttonData* button : _buttons)
    {
        // Suspend the Button Listener
        vTaskSuspend(button->taskHandle);
    }
    return ERROR_SUCCESS;
}

sys_error_t Proc_Button::resume()
{
    for (buttonData* button : _buttons)
    {
        // Clear the Button Data
        buttonDataClear(button);

        // Resume the Button Listener
        vTaskResume(button->taskHandle);
    }
    return ERROR_SUCCESS;
}

// Button Listener
// This task is responsible for processing the GPIO events
// It reads the GPIO input and calculates the duration of the button press
// The duration is printed to the console
/*
| prevState | currentState | pressedState | Description |
|-----------|--------------|--------------|-------------|
| 0         | 0            | 0            | The button was pressed and remains pressed. |
| 0         | 1            | 0            | The button was pressed and now it's not pressed. The `changeTime` is updated and the duration of the pressed state is calculated and printed. |
| 1         | 1            | 0            | The button was not pressed and remains not pressed. |
| 1         | 0            | 0            | The button was not pressed and now it's pressed. The `changeTime` is updated. |
*/

static void buttonListener(void* arg)
{
    Proc_Button::buttonData& button = *static_cast<Proc_Button::buttonData*>(arg);

    gpio_num_t gpioNumber = button.gpio.getGpioNumber();
    button.changeTime     = xTaskGetTickCount(); // Get the current time
    button.gpio.get((static_cast<void*>(&button.prevState)));
    QueueHandle_t gpioEventQueue = button.gpio.getEventQueue();

    printf("Waiting for button to be pressed!\n");
    for (;;)
    {
        if (xQueueReceive(gpioEventQueue, &gpioNumber, portMAX_DELAY))
        {
            button.gpio.get(static_cast<void*>(&button.currentState));
            printf("state: %d\n", button.currentState);

            // If the button is pressed and the previous state is not pressed
            // Record the time when the button is pressed
            // PrevState = 0, currentState = 1
            if (button.prevState == button.pressedState && button.currentState != button.pressedState)
            {
                uint32_t now      = xTaskGetTickCount();
                uint32_t duration = pdTICKS_TO_MS(now - button.changeTime); // Calculate the duration

                printf("GPIO[%" PRIu32 "] intr, pressed state duration : %" PRIu32 "ms\n", (uint32_t)gpioNumber, duration);
            }

            // Update the previous state if the current state is different from the previous state
            // Update the change time
            // PrevState = 1, currentState = 0
            if (button.prevState != button.currentState)
            {
                button.prevState  = button.currentState; // Update the previous state
                button.changeTime = xTaskGetTickCount(); // Update the change time
            }
        }
    }
}

void Proc_Button::buttonDataClear(buttonData* button)
{
    button->changeTime   = xTaskGetTickCount();
    button->currentState = GPIO_LOW;
    button->prevState    = GPIO_HIGH;
    // Clear the queue for unwanted events
    clearQueue(button->gpio.getEventQueue());
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