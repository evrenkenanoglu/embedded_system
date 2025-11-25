#include "uri_powerSwitchesControl.hpp"
#include "Process/Examples/Peripheral/Proc_Switches.hpp"
#include "cJSON.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

UriPowerSwitchesControl::UriPowerSwitchesControl()
    : HttpUriPut("/power-switches-control", nullptr, this)
{
}

UriPowerSwitchesControl::~UriPowerSwitchesControl() {}

uint16_t UriPowerSwitchesControl::handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx)
{
    // Parse the JSON data
    cJSON* json = cJSON_Parse(req_ptr);

    RETURN_IF_ERROR(json == NULL, HTTP::RESPONSE::BAD_REQUEST, SYS_LOG_D("Error parsing JSON data!"));

    // Extract the socketId and state from the JSON data
    cJSON* socketIdJson = cJSON_GetObjectItem(json, "socketId");
    cJSON* stateJson    = cJSON_GetObjectItem(json, "state");

    RETURN_IF_ERROR(!cJSON_IsNumber(socketIdJson) || !cJSON_IsNumber(stateJson), HTTP::RESPONSE::BAD_REQUEST, SYS_LOG_E("Invalid JSON data"));

    int socketId = socketIdJson->valueint;
    int state    = stateJson->valueint;

    SYS_LOG_D("Control Power Switch - Socket ID: %d, State: %d", socketId, state);
    // Proc_Switches::SwitchQueue_t switchQueue = {static_cast<uint8_t>(socketId), static_cast<bool>(state)};

    // if (xQueueSend(proc->getSwitchesQueue(), &switchQueue, 0) == pdTRUE)
    // {
    //     SYS_LOG_I("Switch state updated successfully");
    // }
    // else
    // {
    //     SYS_LOG_E("Failed to update switch state");
    // }

    // Convert response JSON to string
    constexpr char   response_str[] = "{\"message\":\"Success\"}";
    constexpr size_t response_len   = sizeof(response_str) - 1;

    // Copy the response to resp_buf
    size_t copy_len = (response_len < resp_buf_len - 1) ? response_len : (resp_buf_len - 1);
    strncpy(resp_buf, response_str, copy_len);
    resp_buf[copy_len] = '\0';

    // Clean up
    cJSON_Delete(json);

    return HTTP::RESPONSE::OK;
}
