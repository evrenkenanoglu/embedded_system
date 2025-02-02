#include "uri_wifiConnect.hpp"
#include "HAL/Platform/ESP32/library/logImpl.h"
#include "Process/Examples/Network/Proc_wifiConfigurationManager.hpp"
#include "cJSON.h"

namespace
{
constexpr uint16_t post_content_length = 256;
}

/**
 * @brief HTTP POST handler for the connect wifi request
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t connect_post_handler(httpd_req_t* req);

UriWifiConnect::UriWifiConnect(EventGroupHandle_t& wifiConfigEventGroup, IHAL_MEM& memDevice)
    : HttpUriPost("/wifiConnect", connect_post_handler, this) // Initialize Uri post for the connect
    , _wifiConfigEventGroup(wifiConfigEventGroup)             // Initialize the event group
    , _memDevice(memDevice)                                   // Initialize the memory device

{
}

UriWifiConnect::~UriWifiConnect()
{
}

EventGroupHandle_t& UriWifiConnect::getWifiConfigEventGroup()
{
    return _wifiConfigEventGroup;
}

IHAL_MEM& UriWifiConnect::getMemDevice() const
{
    return _memDevice;
}

/* An HTTP POST handler */
static error_t connect_post_handler(httpd_req_t* req)
{
    UriWifiConnect* uriPtr = (UriWifiConnect*)req->user_ctx;

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

            std::cout << "WIFI_SSID: " << WIFI_SSID << std::endl;
            std::cout << "WIFI_PASSWORD: " << WIFI_PASSWORD << std::endl;

            // Write the SSID and password to the memory device
            sys_error_t errorWifi = uriPtr->getMemDevice().writeData(WIFI_SSID, (uint8_t*)json_ssid->valuestring, strlen(json_ssid->valuestring) + 1);
            sys_error_t errorPass = uriPtr->getMemDevice().writeData(WIFI_PASSWORD, (uint8_t*)json_password->valuestring, strlen(json_password->valuestring) + 1);

            std::string responseMessage;
            if (errorWifi != ERROR_SUCCESS || errorPass != ERROR_SUCCESS)
            {
                logger().log(ILog::LogLevel::ERROR, "Storing WiFi and Password Failed!");
                responseMessage = "Storing WiFi And Password Failed!";
            }
            else // if writing to memory device operation Success
            {
                xEventGroupSetBits(uriPtr->getWifiConfigEventGroup(), WIFI_CONFIG_CREDENTIALS_STORED);
                // Send back the SSID and password
                responseMessage = "Received SSID and password. Connecting to  ";
                responseMessage.append(json_ssid->valuestring);
                responseMessage.append("...");
            }

            cJSON_AddStringToObject(response, "message", responseMessage.c_str());
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