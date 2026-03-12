#pragma once
#include "MatterTypes.hpp"
#include "System/system.h"
#include <stdint.h>

// Interface Class for a generic Matter Endpoint
class IMatterDevice
{
public:
    virtual ~IMatterDevice() = default;

    // Called by the Stack when the endpoint is created
    virtual void setEndpointId(uint16_t id) = 0;

    // Called by the Stack to get the endpoint ID
    virtual uint16_t getEndpointId() const = 0;

    virtual void setClusterId(uint32_t id) = 0;

    virtual uint32_t getClusterId() const = 0;

    virtual void setAttributeId(uint32_t id) = 0;

    virtual uint32_t getAttributeId() const = 0;

    // Get Device Type
    virtual MatterTypes::Device getDeviceType() const = 0;

    // Platform implementations must define how to sync state to cloud
    virtual sys_error_t updateLocalState(MatterTypes::AttributeValue_t* value) = 0;

    // This method is called by the Stack when Cloud sends data
    virtual sys_error_t onRemoteCommand(MatterTypes::Event event, MatterTypes::AttributeValue_t* value) = 0;

    // Set callback for local state updates (from device to cloud)
    virtual void setOnLocalStateUpdateCallback(MatterTypes::OnLocalStateUpdate_t callback) = 0;

    // Set callback for remote updates (from cloud to device)
    virtual void setOnRemoteUpdateCallback(MatterTypes::OnRemoteUpdate_t callback, void* params = nullptr) = 0;
};