/**
 * @file Proc_httpServer.cpp
 * @brief Source file for Proc_httpServer
 *
 * This file contains definitions for the Proc_httpServer class and related data types and functions.
 */

#include "Proc_httpServer.hpp"
#include "HAL/Platform/ESP32/library/logImpl.h"
#include "Library/UI/HTTP/ui_welcome_wifi_connect.h"
#include "Process/Examples/Proc_wifiConfigurationManager.hpp"
#include "cJSON.h"
#include "protocol_examples_utils.h"
#include "string.h"
#include <esp_log.h>
#include <sstream>
#include <stdlib.h>

namespace
{
constexpr uint16_t post_content_length = 256;

}

/**
 * @brief HTTP GET handler for the welcome page
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t welcome_get_handler(httpd_req_t* req);

/**
 * @brief HTTP POST handler for the connect wifi request
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t connect_post_handler(httpd_req_t* req);

/**
 * @brief HTTP GET handler for the scan wifi request
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t scan_get_handler(httpd_req_t* req);

Proc_httpServer::Proc_httpServer(EventGroupHandle_t& wifiConfigEventGroup, QueueHandle_t wifiConfigScanResults)
    : _server(NULL),                                                                                       // Initialize the server handle
      _config(HTTPD_DEFAULT_CONFIG()),                                                                     // Initialize the server and configuration
      _wifiConfigEventGroup(wifiConfigEventGroup),                                                         // Initialize the event group
      _wifiConfigScanResults(wifiConfigScanResults),                                                       // Initialize the scan results queue
      _welcomeWifiConnectHtml(HTML_UI_WELCOME_WIFI_CONNECT_CONTENT),                                       // Initialize the welcome page HTML content
      welcome({.uri = "/welcome", .method = HTTP_GET, .handler = welcome_get_handler, .user_ctx = this}),  // Initialize the URI handler for the welcome page
      scan({.uri = "/scan", .method = HTTP_GET, .handler = scan_get_handler, .user_ctx = this}),           // Initialize the URI handler for the scan
      connect({.uri = "/connect", .method = HTTP_POST, .handler = connect_post_handler, .user_ctx = this}) // Initialize the URI handler for the connect
{
    setState(Process::State::INITIALIZED);
}

Proc_httpServer::~Proc_httpServer()
{
    // destructor implementation
}

sys_error_t Proc_httpServer::start()
{
    std::stringstream ss;
    ss << "Starting server on port: " << _config.server_port;
    logger().log(ILog::LogLevel::INFO, ss.str());

    if (httpd_start(&_server, &_config) == ESP_OK)
    {
        // Set URI handlers
        logger().log(ILog::LogLevel::INFO, "Registering URI handlers");
        httpd_register_uri_handler(_server, &welcome);
        httpd_register_uri_handler(_server, &scan);
        httpd_register_uri_handler(_server, &connect);
        setState(Process::State::RUNNING);
    }
    else
    {
        logger().log(ILog::LogLevel::ERROR, "Starting HTTP server failed!");
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t Proc_httpServer::stop()
{
    httpd_stop(_server);
    setState(Process::State::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_httpServer::pause()
{
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t Proc_httpServer::resume()
{
    return ERROR_NOT_IMPLEMENTED;
}

const char* Proc_httpServer::getWelcomeWifiConnectHtml() const
{
    return _welcomeWifiConnectHtml;
}

EventGroupHandle_t& Proc_httpServer::getWifiConfigEventGroup()
{
    return _wifiConfigEventGroup;
}

QueueHandle_t Proc_httpServer::getWifiConfigScanResults()
{
    return _wifiConfigScanResults;
}

/* An HTTP GET handler */
static error_t welcome_get_handler(httpd_req_t* req)
{
    Proc_httpServer* proc = (Proc_httpServer*)req->user_ctx;

    /* Send response with custom headers and body set as the *string passed in user context*/
    ESP_ERROR_CHECK(httpd_resp_send(req, proc->getWelcomeWifiConnectHtml(), HTTPD_RESP_USE_STRLEN));
    return ESP_OK;
}

/* An HTTP POST handler */
static error_t connect_post_handler(httpd_req_t* req)
{
    char content[post_content_length];

    // Read the content of the POST request
    int ret = httpd_req_recv(req, content, sizeof(content));

    if (ret <= 0)
    { // 0 return value indicates connection closed
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        logger().log(ILog::LogLevel::ERROR, "Error receiving data from POST request!");
        return ESP_FAIL;
    }

    // Null terminate the content string
    content[ret] = '\0';

    // Parse the JSON data
    cJSON* json = cJSON_Parse(content);
    if (json == NULL)
    {
        logger().log(ILog::LogLevel::ERROR, "Error parsing JSON data!");
        return ESP_FAIL;
    }

    // Get the SSID and password from the JSON data
    cJSON* json_ssid     = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON* json_password = cJSON_GetObjectItemCaseSensitive(json, "password");

    cJSON* response = cJSON_CreateObject();
    char*  response_str;
    httpd_resp_set_type(req, "application/json");

    if (cJSON_IsString(json_ssid) && (json_ssid->valuestring != NULL) && cJSON_IsString(json_password) && (json_password->valuestring != NULL))
    {
        if ((json_ssid->valuestring[0] == '\0') || (json_password->valuestring[0] == '\0')) // Check if SSID and password are empty
        {
            logger().log(ILog::LogLevel::ERROR, "SSID or password can't be empty!");

            cJSON_AddStringToObject(response, "message", "SSID or password can't be empty!");
        }
        else // SSID and password are not empty
        {
            logger().log(ILog::LogLevel::INFO, "Received SSID and password!");
            std::stringstream ss;
            ss << "SSID: " << json_ssid->valuestring << std::endl;
            ss << "Password: " << json_password->valuestring << std::endl;
            logger().log(ILog::LogLevel::INFO, ss.str());

            // Send back the SSID and password
            char responseMessage[128] = "Received SSID and password. Connecting to  ";

            strcat(responseMessage, json_ssid->valuestring);
            strcat(responseMessage, "...");

            cJSON_AddStringToObject(response, "message", responseMessage);
        }
    }
    else // Error parsing SSID and password
    {
        logger().log(ILog::LogLevel::ERROR, "Error parsing SSID and password!");

        cJSON_AddStringToObject(response, "message", "Error parsing SSID and password!");
    }

    response_str = cJSON_PrintUnformatted(response);

    // Send the response
    httpd_resp_send(req, response_str, strlen(response_str));

    // Clean up
    free(response_str);
    cJSON_Delete(json);
    cJSON_Delete(response);

    return ESP_OK;
}

static error_t scan_get_handler(httpd_req_t* req)
{
    Proc_httpServer* proc = (Proc_httpServer*)req->user_ctx;

    std::cout << "Scan request received!" << std::endl;
    // Create a JSON array of APs
    cJSON* ap_array = cJSON_CreateArray();

    // Send SCAN REQUEST event
    xEventGroupSetBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_SCAN_REQUESTED);

    // Wait for SCAN DONE event
    EventBits_t bits = xEventGroupWaitBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_SCAN_DONE, pdTRUE, pdTRUE, WIFI_SCAN_TIMEOUT);

    if (bits & WIFI_CONFIG_SCAN_DONE)
    {
        // Get the scanned APs over Scan Results Queue
        wifiApRecord_t ap_info[WIFI_SCAN_MAX_RECORDS];

        // Get the results from the scan result Queue
        int ap_count = uxQueueMessagesWaiting(proc->getWifiConfigScanResults());
        for (int i = 0; i < ap_count; i++)
        {
            xQueueReceive(proc->getWifiConfigScanResults(), &ap_info[i], 0);
        }

        // Add the scanned APs to the JSON array
        for (int i = 0; i < ap_count; i++)
        {
            cJSON* ap = cJSON_CreateObject();
            cJSON_AddStringToObject(ap, "ssid", (const char*)ap_info[i].ssid);
            cJSON_AddItemToArray(ap_array, ap);
        }
    }

    // Send the JSON array
    char* response_str = cJSON_PrintUnformatted(ap_array);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));

    cJSON_Delete(ap_array);

    return ESP_OK;
}