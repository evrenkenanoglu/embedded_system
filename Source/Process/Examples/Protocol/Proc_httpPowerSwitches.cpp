/**
 * @file Proc_httpPowerSwitches.cpp
 * @brief Source file for Proc_httpPowerSwitches
 *
 * This file contains definitions for the Proc_httpPowerSwitches class and related data types and functions.
 */

#include "Proc_httpPowerSwitches.hpp"
#include "HAL/Platform/ESP32/library/logImpl.h"
#include "Library/UI/HTTP/ui_wifi_power_sockets.h"
#include "cJSON.h"
#include <iostream>

/**
 * @brief HTTP GET handler for the app interface
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t app_interface_get_handler(httpd_req_t* req);

/**
 * @brief HTTP POST handler for the write request
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t control_post_handler(httpd_req_t* req);

Proc_httpPowerSwitches::Proc_httpPowerSwitches(QueueHandle_t switchesQueue)
    : _switchesQueue(switchesQueue)                                                                                          // Initialize the switch queue
    , _server(NULL)                                                                                                          // Initialize the server handle
    , _config(HTTPD_DEFAULT_CONFIG())                                                                                        // Initialize the server and configuration
    , _powerSwitchesHtml(HTML_UI_WIFI_POWER_SOCKETS_CONTENT)                                                                 // Initialize the power switches HTML content
    , appInterface({.uri = "/powerSwitchesApp", .method = HTTP_GET, .handler = app_interface_get_handler, .user_ctx = this}) // Initialize the URI handler for the app interface
    , control({.uri = "/power-switches-control", .method = HTTP_POST, .handler = control_post_handler, .user_ctx = this})    // Initialize the URI handler for the write
{
    setState(Process::State::INITIALIZED);
}

Proc_httpPowerSwitches::~Proc_httpPowerSwitches()
{
    // destructor implementation
}

sys_error_t Proc_httpPowerSwitches::start()
{
    if (getState() == Process::State::RUNNING)
    {
        logger().log(ILog::LogLevel::WARNING, "HTTP server is already running");
        return ERROR_SUCCESS;
    }

    if (httpd_start(&_server, &_config) == ESP_OK)
    {
        // Set URI handlers
        logger().log(ILog::LogLevel::INFO, "Registering URI handlers");
        httpd_register_uri_handler(_server, &appInterface);
        httpd_register_uri_handler(_server, &control);
        setState(Process::State::RUNNING);
    }
    else
    {
        logger().log(ILog::LogLevel::ERROR, "Starting HTTP server failed!");
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t Proc_httpPowerSwitches::stop()
{
    if (getState() != Process::State::RUNNING)
    {
        logger().log(ILog::LogLevel::WARNING, "HTTP server is not running, nothing to stop");
        return ERROR_SUCCESS;
    }

    httpd_stop(_server);
    setState(Process::State::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_httpPowerSwitches::pause()
{
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t Proc_httpPowerSwitches::resume()
{
    return ERROR_NOT_IMPLEMENTED;
}

const char* Proc_httpPowerSwitches::getPowerSwitchesHtml() const
{
    return _powerSwitchesHtml;
}

QueueHandle_t Proc_httpPowerSwitches::getSwitchesQueue()
{
    return _switchesQueue;
}

/**
 * @brief HTTP GET handler for the app interface
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t app_interface_get_handler(httpd_req_t* req)
{
    Proc_httpPowerSwitches* proc = (Proc_httpPowerSwitches*)req->user_ctx;

    /* Send response with custom headers and body set as the *string passed in user context*/
    ESP_ERROR_CHECK(httpd_resp_send(req, proc->getPowerSwitchesHtml(), HTTPD_RESP_USE_STRLEN));
    return ESP_OK;
}

/**
 * @brief HTTP POST handler for the write request
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t control_post_handler(httpd_req_t* req)
{

    Proc_httpPowerSwitches* proc = (Proc_httpPowerSwitches*)req->user_ctx;

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
        logger().log(ILog::LogLevel::ERROR, "Error parsing JSON data");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Extract the socketId and state from the JSON data
    cJSON* socketIdJson = cJSON_GetObjectItem(json, "socketId");
    cJSON* stateJson    = cJSON_GetObjectItem(json, "state");

    if (!cJSON_IsNumber(socketIdJson) || !cJSON_IsNumber(stateJson))
    {
        logger().log(ILog::LogLevel::ERROR, "Invalid JSON data");
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

    if(xQueueSend(proc->getSwitchesQueue(), &switchQueue, 0) == pdTRUE)
    {
        logger().log(ILog::LogLevel::INFO, "Switch state updated successfully");
    }
    else
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to update switch state");
    }

    // Send a response back to the client
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"message\":\"Success\"}");

    // Clean up
    cJSON_Delete(json);

    return ESP_OK;
}
