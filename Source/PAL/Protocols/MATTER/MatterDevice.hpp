#pragma once
#include "IMatterDevice.hpp"
#include <stdint.h>

// Abstract Base Class for a generic Matter Endpoint
class MatterDevice : public IMatterDevice
{
public:
    virtual ~MatterDevice() = default;

    // Called by the Stack when the endpoint is created
    void setEndpointId(uint16_t id)
    {
        _endpoint_id = id;
    }
    uint16_t getEndpointId() const
    {
        return _endpoint_id;
    }

    void setClusterId(uint32_t id)
    {
        _cluster_id = id;
    }

    uint32_t getClusterId() const
    {
        return _cluster_id;
    }

    void setAttributeId(uint32_t id)
    {
        _attribute_id = id;
    }

    uint32_t getAttributeId() const
    {
        return _attribute_id;
    }

    // Platform implementations must define how to sync state to cloud
    virtual sys_error_t updateLocalState(MatterTypes::AttributeValue_t* value) = 0;

    // This method is called by the Stack when Cloud sends data
    virtual sys_error_t onRemoteCommand(MatterTypes::Event event, MatterTypes::AttributeValue_t* value) = 0;

    /**
     * @brief Set callback for local state updates (from device to cloud)
     * @param callback Function pointer to the callback
     */
    void setOnLocalStateUpdateCallback(MatterTypes::OnLocalStateUpdate_t callback) override
    {
        _onLocalStateUpdateCallback = callback;
    }

    void setOnRemoteUpdateCallback(MatterTypes::OnRemoteUpdate_t callback, void* params = nullptr)
    {
        _onRemoteUpdateCallback = callback;
        _params                     = params;

    }

    /**
     * @brief Get Device Type
     * @return MatterTypes::Device Device Type
     */
    MatterTypes::Device getDeviceType() const
    {
        return _device_type;
    }

protected:
    uint16_t                          _endpoint_id  = 0;
    uint32_t                          _cluster_id   = 0;
    uint32_t                          _attribute_id = 0;
    MatterTypes::Device               _device_type;
    MatterTypes::OnLocalStateUpdate_t _onLocalStateUpdateCallback = nullptr;
    MatterTypes::OnRemoteUpdate_t     _onRemoteUpdateCallback     = nullptr;
    void*                             _params                     = nullptr;
};