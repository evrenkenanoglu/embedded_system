#include "uri_wifiConnect.hpp"
#include "Process/Examples/Network/Proc_wifiConfigurationManager.hpp"
#include "cJSON.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

UriWifiConnect::UriWifiConnect(EventGroupHandle_t& wifiConfigEventGroup, IHAL_MEM& memDevice)
    : HttpUriPost("/wifiConnect", nullptr, this)  // Initialize Uri post for the connect
    , _wifiConfigEventGroup(wifiConfigEventGroup) // Initialize the event group
    , _memDevice(memDevice)                       // Initialize the memory device

{
}

UriWifiConnect::~UriWifiConnect() {}

uint16_t UriWifiConnect::handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx)
{
    // Parse the JSON data
    cJSON* json = cJSON_Parse(req_ptr);

    RETURN_IF_ERROR(json == NULL, 500, SYS_LOG_D("Error parsing JSON data!"));

    // Get the SSID and password from the JSON data
    cJSON* json_ssid     = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON* json_password = cJSON_GetObjectItemCaseSensitive(json, "password");

    cJSON* response = cJSON_CreateObject();
    char*  response_str;

    if (cJSON_IsString(json_ssid) && (json_ssid->valuestring != NULL) && cJSON_IsString(json_password) && (json_password->valuestring != NULL))
    {
        if ((json_ssid->valuestring[0] == '\0') || (json_password->valuestring[0] == '\0')) // Check if SSID and password are empty
        {
            SYS_LOG_E("SSID or password can't be empty!");

            cJSON_AddStringToObject(response, "message", "SSID or password can't be empty!");
        }
        else // SSID and password are not empty
        {
            SYS_LOG_I("Received SSID and password!");
            std::stringstream ss;
            ss << "SSID: " << json_ssid->valuestring << std::endl;
            ss << "Password: " << json_password->valuestring << std::endl;
            SYS_LOG_I(ss.str());

            std::cout << "WIFI_SSID: " << WIFI_SSID << std::endl;
            std::cout << "WIFI_PASSWORD: " << WIFI_PASSWORD << std::endl;

            // Write the SSID and password to the memory device
            sys_error_t errorWifi = _memDevice.writeData(WIFI_SSID, (uint8_t*)json_ssid->valuestring, strlen(json_ssid->valuestring) + 1);
            sys_error_t errorPass = _memDevice.writeData(WIFI_PASSWORD, (uint8_t*)json_password->valuestring, strlen(json_password->valuestring) + 1);

            std::string responseMessage;
            if (errorWifi != ERROR_SUCCESS || errorPass != ERROR_SUCCESS)
            {
                SYS_LOG_E("Storing WiFi and Password Failed!");
                responseMessage = "Storing WiFi And Password Failed!";
            }
            else // if writing to memory device operation Success
            {
                xEventGroupSetBits(_wifiConfigEventGroup, WIFI_CONFIG_CREDENTIALS_STORED);
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
        SYS_LOG_E("Error parsing SSID and password!");

        cJSON_AddStringToObject(response, "message", "Error parsing SSID and password!");
    }

    response_str = cJSON_PrintUnformatted(response);

    // Copy response to resp_buf if provided
    size_t response_len = strlen(response_str);
    size_t max_copy     = (resp_buf_len > 0) ? (resp_buf_len - 1) : 0;
    size_t to_copy      = (response_len < max_copy) ? response_len : max_copy;

    if (to_copy > 0)
        std::memcpy(resp_buf, response_str, to_copy);

    resp_buf[to_copy] = '\0'; // Null-terminate the response buffer

    // Clean up
    free(response_str);
    cJSON_Delete(json);
    cJSON_Delete(response);

    return HTTP::RESPONSE::OK;
}