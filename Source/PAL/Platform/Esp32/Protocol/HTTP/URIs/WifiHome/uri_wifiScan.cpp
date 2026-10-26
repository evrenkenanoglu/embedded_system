/** @file       uri_wifiScan.cpp
 *  @brief      WiFi scan HTTP URI implementation (GET /wifiScan).
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @date       26/10/2026
 */

#include "uri_wifiScan.hpp"

#include "cJSON.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

#include <cstring>

UriWiFiScan::UriWiFiScan(WifiConfigurationManager& wifiMgr)
    : HttpUriGet("/wifiScan", nullptr, this, nullptr)
    , _wifiMgr(wifiMgr)
{
}

UriWiFiScan::~UriWiFiScan() {}

uint16_t UriWiFiScan::handler(const char* /*req_ptr*/, size_t /*req_len*/, char* resp_buf, size_t resp_buf_len, void* user_ctx)
{
    RETURN_IF_ERROR_WITH_LOG((!resp_buf) || (resp_buf_len == 0), 500, "Response buffer is null or of zero length");

    UriWiFiScan* uriPtr = static_cast<UriWiFiScan*>(user_ctx);

    RETURN_IF_ERROR(uriPtr == nullptr, 500, SYS_LOG_D("UriWiFiScan user_ctx is null"));

    SYS_LOG_D("Handling WiFi Scan GET request");

    // Create a JSON array of APs
    cJSON* ap_array = cJSON_CreateArray();

    // Run a bounded scan through the connection manager (typed HAL results).
    constexpr uint16_t   kMaxRecords          = 16;
    hal_wifi_ap_record_t records[kMaxRecords] = {};
    uint16_t             actualRecords        = 0;

    sys_error_t error = uriPtr->_wifiMgr.requestScan(true);
    if (error == ERROR_SUCCESS)
    {
        error = uriPtr->_wifiMgr.getScanResults(records, kMaxRecords, actualRecords);
    }

    if (error == ERROR_SUCCESS)
    {
        SYS_LOG_I("Total APs scanned = %u", actualRecords);
        for (uint16_t i = 0; i < actualRecords; i++)
        {
            cJSON* ap = cJSON_CreateObject();
            cJSON_AddStringToObject(ap, "ssid", records[i].ssid);
            cJSON_AddNumberToObject(ap, "rssi", records[i].rssi);
            cJSON_AddNumberToObject(ap, "channel", records[i].primary_channel);
            cJSON_AddNumberToObject(ap, "authmode", static_cast<int>(records[i].authmode));
            cJSON_AddItemToArray(ap_array, ap);
        }
    }
    else
    {
        SYS_LOG_E("WiFi scan failed: %d", static_cast<int>(error));
    }

    // Send the JSON array
    const char* response_str = cJSON_PrintUnformatted(ap_array);

    // Leave space for NUL
    size_t response_len = strlen(response_str);
    size_t max_copy     = (resp_buf_len > 0) ? (resp_buf_len - 1) : 0;
    size_t to_copy      = (response_len < max_copy) ? response_len : max_copy;

    // Copy response to resp_buf
    if (to_copy > 0)
        std::memcpy(resp_buf, response_str, to_copy);
    resp_buf[to_copy] = '\0'; // Null-terminate the response

    cJSON_Delete(ap_array);
    return HTTP::RESPONSE::OK;
}
