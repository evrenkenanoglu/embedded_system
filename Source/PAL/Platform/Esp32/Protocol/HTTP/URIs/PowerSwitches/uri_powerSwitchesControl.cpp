#include "uri_powerSwitchesControl.hpp"
#include "Library/Utility/switch.hpp"
#include "Process/Examples/Peripheral/Proc_Switches.hpp"
#include "cJSON.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

UriPowerSwitchesControl::UriPowerSwitchesControl(QueueHandle_t httpButtonEventQueue)
    : HttpUriPut("/power-switches-control", nullptr, this)
    , _httpButtonEventQueue(httpButtonEventQueue)
{
    RETURN_IF_ERROR(httpButtonEventQueue == nullptr, , SYS_LOG_E("Invalid HTTP button event queue"));
}

UriPowerSwitchesControl::~UriPowerSwitchesControl() {}

uint16_t UriPowerSwitchesControl::handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx)
{
    // Ensure the event queue is valid
    RETURN_IF_ERROR(_httpButtonEventQueue == nullptr, HTTP::RESPONSE::INTERNAL_SERVER_ERROR, SYS_LOG_E("HTTP button event queue is null"));

    // Parse the JSON data
    cJSON* json = cJSON_Parse(req_ptr);

    // Check if parsing was successful
    RETURN_IF_ERROR(json == NULL, HTTP::RESPONSE::BAD_REQUEST, SYS_LOG_D("Error parsing JSON data!"));

    // Extract the socketId and state from the JSON data
    cJSON* socketIdJson = cJSON_GetObjectItem(json, "socketId");
    cJSON* stateJson    = cJSON_GetObjectItem(json, "state");

    // Validate the extracted data
    RETURN_IF_ERROR(!cJSON_IsNumber(socketIdJson) || !cJSON_IsNumber(stateJson), HTTP::RESPONSE::BAD_REQUEST, SYS_LOG_E("Invalid JSON data"));

    int socketId = socketIdJson->valueint;
    int state    = stateJson->valueint;

    SYS_LOG_D("Control Power Switch - Socket ID: %d, State: %d", socketId, state);

    // Create a switch event and send it to the queue
    SWITCH::EventData_t eventData;
    eventData.index = static_cast<uint16_t>(socketId);
    eventData.event = SWITCH::Event::STATE_CHANGED;
    eventData.state = static_cast<SWITCH::State>(state);

    RETURN_IF_ERROR(xQueueSend(_httpButtonEventQueue, &eventData, 0) != pdPASS, HTTP::RESPONSE::INTERNAL_SERVER_ERROR, SYS_LOG_E("Failed to send switch event to queue"));

    SYS_LOG_D("Switch event sent to queue successfully");

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
