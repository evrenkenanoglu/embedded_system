#include "cJSON.h"
#include "System/LogHandler.h"
#include "uri_powerSwitchesControl.hpp"
#include "Process/Examples/Peripheral/Proc_Switches.hpp"

/**
 * @brief HTTP POST handler for the write request
 *
 * @param req HTTP request
 * @return error_t
 */

static error_t control_put_handler(httpd_req_t* req);

UriPowerSwitchesControl::UriPowerSwitchesControl(QueueHandle_t switchesQueue)
    : HttpUriPut("/power-switches-control", control_put_handler, this)
    , _switchesQueue(switchesQueue)
{
}

UriPowerSwitchesControl::~UriPowerSwitchesControl() {}

QueueHandle_t UriPowerSwitchesControl::getSwitchesQueue() const
{
    return _switchesQueue;
}

static error_t control_put_handler(httpd_req_t* req)
{

    UriPowerSwitchesControl* proc = (UriPowerSwitchesControl*)req->user_ctx;

    // Buffer to store the incoming JSON data
    char content[100];
    int  ret;

    // Read the content of the request
    if ((ret = httpd_req_recv(req, content, sizeof(content))) <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Parse the JSON data
    cJSON* json = cJSON_Parse(content);
    if (json == NULL)
    {
        SYS_LOG_E("Error parsing JSON data");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Extract the socketId and state from the JSON data
    cJSON* socketIdJson = cJSON_GetObjectItem(json, "socketId");
    cJSON* stateJson    = cJSON_GetObjectItem(json, "state");

    if (!cJSON_IsNumber(socketIdJson) || !cJSON_IsNumber(stateJson))
    {
        SYS_LOG_E("Invalid JSON data");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON data");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    int socketId = socketIdJson->valueint;
    int state    = stateJson->valueint;

    // Perform the necessary actions to control the power switches
    // For example, you can call a function to set the GPIO pin state
    std::cout << "Setting socket " << socketId << " to state " << state << std::endl;
    Proc_Switches::SwitchQueue_t switchQueue = {static_cast<uint8_t>(socketId), static_cast<bool>(state)};

    if (xQueueSend(proc->getSwitchesQueue(), &switchQueue, 0) == pdTRUE)
    {
        SYS_LOG_I( "Switch state updated successfully");
    }
    else
    {
        SYS_LOG_E("Failed to update switch state");
    }

    // Send a response back to the client
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"message\":\"Success\"}");

    // Clean up
    cJSON_Delete(json);

    return ESP_OK;
}
