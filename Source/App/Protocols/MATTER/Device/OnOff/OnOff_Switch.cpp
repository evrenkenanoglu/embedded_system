#include "OnOff_Switch.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

OnOff_Switch::OnOff_Switch(SWITCH::Instance_t& instance, QueueHandle_t eventQueueSwitches)
    : OnOff_Plugin()
    , _instance(instance)
    , _eventQueueSwitches(eventQueueSwitches)

{
    setOnRemoteUpdateCallback(onRemoteCommandHandleCb, this);
}

OnOff_Switch::~OnOff_Switch() {}

sys_error_t OnOff_Switch::onRemoteCommandHandleCb(MatterTypes::Event event, MatterTypes::AttributeValue_t* value, void* params)
{
    OnOff_Switch* onOffSwitch = static_cast<OnOff_Switch*>(params);
    RETURN_IF_ERROR(onOffSwitch == nullptr, ERROR_INVALID_ARG, SYS_LOG_E("Invalid parameters"));

    RETURN_IF_ERROR(
        (event != MatterTypes::Event::POST_UPDATE) && (value->type != MatterTypes::Value::BOOLEAN), // Expression
        ERROR_INVALID_ARG,                                                                          // Error code
        SYS_LOG_E("Invalid event: %d", (int)event)                                                  // Log Message
    );

    // Create the event data
    SWITCH::EventData_t eventData = {
        .index = onOffSwitch->_instance.index,              // Index
        .event = SWITCH::Event::STATE_CHANGED,              // Event
        .state = static_cast<SWITCH::State>(value->value.b) // State
    };

    RETURN_IF_ERROR(
        xQueueSend(onOffSwitch->_eventQueueSwitches, &eventData, portMAX_DELAY) != pdPASS,
        ERROR_FAIL,
        SYS_LOG_E("Failed to send switch event for switch index %d", onOffSwitch->_instance.index));

    return ERROR_SUCCESS;
}
