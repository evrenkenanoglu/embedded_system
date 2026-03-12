#pragma once
#include "PAL/Protocols/MATTER/MatterDevice.hpp"

class OnOff : public MatterDevice
{
public:
    OnOff(MatterTypes::Device type);
    ~OnOff() = default;

    sys_error_t updateLocalState(MatterTypes::AttributeValue_t* value) override;

    sys_error_t onRemoteCommand(MatterTypes::Event event, MatterTypes::AttributeValue_t* value) override;
};