#pragma once

#include "Library/Utility/switch.hpp"
#include "PAL/Protocols/MATTER/Device/OnOff/OnOff_Plugin.hpp"

class OnOff_Switch : public OnOff_Plugin
{
public:
    OnOff_Switch(SWITCH::Instance_t& instance, QueueHandle_t eventQueueSwitches);
    ~OnOff_Switch();

    static sys_error_t onRemoteCommandHandleCb(MatterTypes::Event event, MatterTypes::AttributeValue_t* value, void* params);

private:
    SWITCH::Instance_t& _instance;
    QueueHandle_t       _eventQueueSwitches;
};
