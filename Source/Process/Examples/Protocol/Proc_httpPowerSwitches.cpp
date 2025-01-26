/**
 * @file Proc_httpPowerSwitches.cpp
 * @brief Source file for Proc_httpPowerSwitches
 *
 * This file contains definitions for the Proc_httpPowerSwitches class and related data types and functions.
 */

#include "Proc_httpPowerSwitches.hpp"
#include "HAL/Platform/ESP32/library/logImpl.h"
#include "Library/UI/HTTP/ui_wifi_power_sockets.h"

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
static error_t write_post_handler(httpd_req_t* req);

Proc_httpPowerSwitches::Proc_httpPowerSwitches()
    : _server(NULL)                                                                                                                   // Initialize the server handle
    , _config(HTTPD_DEFAULT_CONFIG())                                                                                                 // Initialize the server and configuration
    , _powerSwitchesHtml(HTML_UI_WIFI_POWER_SOCKETS_CONTENT)                                                                          // Initialize the power switches HTML content
    , appInterface({.uri = "/powerSwitchesAppInterface", .method = HTTP_GET, .handler = app_interface_get_handler, .user_ctx = this}) // Initialize the URI handler for the app interface
    , write({.uri = "/powerSwitchesWrite", .method = HTTP_POST, .handler = write_post_handler, .user_ctx = this})                     // Initialize the URI handler for the write
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
        httpd_register_uri_handler(_server, &write);
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
static error_t write_post_handler(httpd_req_t* req)
{
    return ESP_OK;
}
