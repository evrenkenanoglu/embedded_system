#pragma once
#include "IMatterDevice.hpp"
#include "System/system.h"
#include <functional>
#include <stdint.h>

/**
 * @brief Platform-independent Matter commissioning lifecycle events.
 */
enum class MatterStackEvent : uint8_t
{
    IP_ADDRESS_CHANGED = 0,
    COMMISSIONING_SESSION_STARTED,
    COMMISSIONING_SESSION_STOPPED,
    COMMISSIONING_WINDOW_OPENED,
    ,
    COMMISSIONING_COMPLETE,
    COMMISSIONING_FAILED
};

/**
 * @brief Matter lifecycle callback invoked from the platform Matter event task.
 *
 * Implementations must be short and non-blocking.
 */
using MatterStackEventCallback = void (*)(MatterStackEvent event, void* userData);

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
     * @brief Register a Matter lifecycle callback before starting the stack.
     *
     * @param callback Callback to register; nullptr unregisters it.
     * @param userData Opaque context passed to the callback.
     * @return sys_error_t ERROR_SUCCESS on success, otherwise an error code.
     */
    virtual sys_error_t setEventCallback(MatterStackEventCallback callback, void* userData = nullptr) = 0;

    /**
     * @brief Platform implementations must define how to sync state to cloud
     */
    virtual sys_error_t updateLocalState(uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, MatterTypes::AttributeValue_t* value) = 0;
};
