#include "OnOff.hpp"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

OnOff::OnOff(MatterTypes::Device type)
{
    _device_type = type;
}

sys_error_t OnOff::updateLocalState(MatterTypes::AttributeValue_t* value)
{
    // Validate the endpoint, cluster IDs, callback and value
    RETURN_IF_ERROR(
        (_endpoint_id == 0) || (_cluster_id == 0) || (value == nullptr) || (_onLocalStateUpdateCallback == nullptr), // Expression
        ERROR_INVALID_STATE,                                                                                         // Error Code
        SYS_LOG_E("Invalid endpoint or cluster ID")                                                                  // Log Message
    );

    // Update the local state
    _onLocalStateUpdateCallback(_endpoint_id, _cluster_id, 0, value);

    return ERROR_SUCCESS;
}

sys_error_t OnOff::onRemoteCommand(MatterTypes::Event event, MatterTypes::AttributeValue_t* value)
{
    if (_onRemoteUpdateCallback == nullptr)
    {
        SYS_LOG_E("onRemoteUpdateCallback is not set");
        return ERROR_INVALID_STATE;
    }

    SYS_LOG_D("OnOff::onRemoteCommand()");
    return _onRemoteUpdateCallback(event, value, _params);
}