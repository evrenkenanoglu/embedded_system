#include "uri_wifiScan.hpp"

#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "Process/Examples/Network/Proc_wifiConfigurationManager.hpp"
#include "cJSON.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

UriWiFiScan::UriWiFiScan(EventGroupHandle_t& wifiConfigEventGroup, QueueHandle_t wifiConfigScanResults)
    : HttpUriGet("/wifiScan", nullptr, this, nullptr)
    , _wifiConfigEventGroup(wifiConfigEventGroup)
    , _wifiConfigScanResults(wifiConfigScanResults)
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

    // Send SCAN REQUEST event
    xEventGroupSetBits(uriPtr->_wifiConfigEventGroup, WIFI_CONFIG_SCAN_REQUESTED);

    // Wait for SCAN DONE event
    EventBits_t bits = xEventGroupWaitBits(uriPtr->_wifiConfigEventGroup, WIFI_CONFIG_SCAN_DONE, pdTRUE, pdTRUE, WIFI_SCAN_TIMEOUT);

    if (bits & WIFI_CONFIG_SCAN_DONE)
    {
        // Get the scanned APs over Scan Results Queue
        wifiApRecord_t ap_info[WIFI_SCAN_MAX_RECORDS];

        // Get the results from the scan result Queue
        int ap_count = uxQueueMessagesWaiting(uriPtr->_wifiConfigScanResults);
        for (int i = 0; i < ap_count; i++)
        {
            xQueueReceive(uriPtr->_wifiConfigScanResults, &ap_info[i], 0);
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
    const char* response_str = cJSON_PrintUnformatted(ap_array);

    SYS_LOG_D("WiFi Scan Response: %s", response_str);

    // Leave space for NUL
    size_t response_len = strlen(response_str);
    size_t max_copy     = (resp_buf_len > 0) ? (resp_buf_len - 1) : 0;
    size_t to_copy      = (response_len < max_copy) ? response_len : max_copy;

    // Copy response to resp_buf
    if (to_copy > 0)
        std::memcpy(resp_buf, response_str, to_copy);
    resp_buf[to_copy] = '\0'; // Null-terminate the response

    SYS_LOG_D("WiFi Scan response prepared, length: %d", static_cast<int>(to_copy));

    cJSON_Delete(ap_array);
    return HTTP::RESPONSE::OK;
}