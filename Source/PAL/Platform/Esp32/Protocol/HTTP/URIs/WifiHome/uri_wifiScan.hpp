#ifndef URI_WIFISCAN_HPP
#define URI_WIFISCAN_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriGet.hpp"

class UriWiFiScan : public HttpUriGet
{
public:
    UriWiFiScan(EventGroupHandle_t& wifiConfigEventGroup, QueueHandle_t wifiConfigScanResults);
    ~UriWiFiScan();

private:
    uint16_t handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx) override;

private:
    EventGroupHandle_t& _wifiConfigEventGroup;
    QueueHandle_t       _wifiConfigScanResults;
};

#endif // URI_WIFISCAN_HPP