#ifndef URI_WIFISCAN_HPP
#define URI_WIFISCAN_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriGet.hpp"

class UriWiFiScan : public HttpUriGet
{
public:
    UriWiFiScan(EventGroupHandle_t& wifiConfigEventGroup, QueueHandle_t wifiConfigScanResults);
    ~UriWiFiScan();

    /**
     * @brief Get the Wifi Config Event Group object
     *
     * @return EventGroupHandle_t&
     */
    EventGroupHandle_t& getWifiConfigEventGroup() const;

    /**
     * @brief Get the Wifi Config Scan Results object
     *
     * @return QueueHandle_t
     */
    QueueHandle_t getWifiConfigScanResults() const;

private:
    EventGroupHandle_t& _wifiConfigEventGroup;
    QueueHandle_t       _wifiConfigScanResults;
};

#endif // URI_WIFISCAN_HPP