#include "uri_wifiScan.hpp"

#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "cJSON.h"
#include "Process/Examples/Network/Proc_wifiConfigurationManager.hpp"

/**
 * @brief HTTP GET handler for the scan wifi request
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t scan_get_handler(httpd_req_t* req);

UriWiFiScan::UriWiFiScan(EventGroupHandle_t& wifiConfigEventGroup, QueueHandle_t wifiConfigScanResults)
    : HttpUriGet("/wifiScan", scan_get_handler, this)
    , _wifiConfigEventGroup(wifiConfigEventGroup)
    , _wifiConfigScanResults(wifiConfigScanResults)
{
}

UriWiFiScan::~UriWiFiScan() {}

EventGroupHandle_t& UriWiFiScan::getWifiConfigEventGroup() const
{
    return _wifiConfigEventGroup;
}

QueueHandle_t UriWiFiScan::getWifiConfigScanResults() const
{
    return _wifiConfigScanResults;
}

static error_t scan_get_handler(httpd_req_t* req)
{
    UriWiFiScan* uriPtr = (UriWiFiScan*)req->user_ctx;

    if (uriPtr == nullptr)
    {
        return ESP_FAIL;
    }

    std::cout << "Scan request received!" << std::endl;
    // Create a JSON array of APs
    cJSON* ap_array = cJSON_CreateArray();

    // Send SCAN REQUEST event
    xEventGroupSetBits(uriPtr->getWifiConfigEventGroup(), WIFI_CONFIG_SCAN_REQUESTED);

    // Wait for SCAN DONE event
    EventBits_t bits = xEventGroupWaitBits(uriPtr->getWifiConfigEventGroup(), WIFI_CONFIG_SCAN_DONE, pdTRUE, pdTRUE, WIFI_SCAN_TIMEOUT);

    if (bits & WIFI_CONFIG_SCAN_DONE)
    {
        // Get the scanned APs over Scan Results Queue
        wifiApRecord_t ap_info[WIFI_SCAN_MAX_RECORDS];

        // Get the results from the scan result Queue
        int ap_count = uxQueueMessagesWaiting(uriPtr->getWifiConfigScanResults());
        for (int i = 0; i < ap_count; i++)
        {
            xQueueReceive(uriPtr->getWifiConfigScanResults(), &ap_info[i], 0);
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