/** @file       uri_wifiConnect.cpp
 *  @brief      WiFi credentials provisioning HTTP URI implementation (POST /wifiConnect).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#include "uri_wifiConnect.hpp"

#include "cJSON.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

#include <cstring>

UriWifiConnect::UriWifiConnect(WifiConfigurationManager& wifiMgr, IHAL_MEM& memDevice)
    : HttpUriPost("/wifiConnect", nullptr, this) // Initialize Uri post for the connect
    , _wifiMgr(wifiMgr)                          // Initialize the WiFi manager reference
    , _memDevice(memDevice)                      // Initialize the memory device
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

            // Persist through the manager (it also triggers the connect flow);
            // the URI keeps a memory HAL reference for direct validation needs.
            sys_error_t error = _wifiMgr.storeCredentials(json_ssid->valuestring, json_password->valuestring);

            if (error != ERROR_SUCCESS)
            {
                SYS_LOG_E("Storing WiFi credentials failed!");
                cJSON_AddStringToObject(response, "message", "Storing WiFi credentials failed!");
            }
            else
            {
                cJSON_AddStringToObject(response, "message", "Received SSID and password. Connecting...");
            }
        }
    }
    else // Error parsing SSID and password
    {
        SYS_LOG_E("Error parsing SSID and password!");

        cJSON_AddStringToObject(response, "message", "Error parsing SSID and password!");
    }

    char* response_str = cJSON_PrintUnformatted(response);

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
