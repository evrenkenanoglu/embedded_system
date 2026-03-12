#pragma once
#include "IMatterDevice.hpp"
#include "System/system.h"
#include <functional>
#include <stdint.h>

// Interface for Matter Protocol Abstraction Layer
class IMatterStack
{
public:
    virtual ~IMatterStack() = default;

    virtual sys_error_t init() = 0;

    virtual sys_error_t addDevice(IMatterDevice& device) = 0;

    // Start the Network / Event Loop
    virtual sys_error_t start() = 0;

    /**
     * @brief Platform implementations must define how to sync state to cloud
     */
    virtual sys_error_t updateLocalState(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, MatterTypes::AttributeValue_t* value) = 0;
};