#ifndef URI_WIFICONNECT_HPP
#define URI_WIFICONNECT_HPP

#include "HAL/IHal/IHal.h"
#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriPost.hpp"

class UriWifiConnect : public HttpUriPost
{
public:
    UriWifiConnect(EventGroupHandle_t& wifiConfigEventGroup, IHAL_MEM& memDevice);
    ~UriWifiConnect();

private:
    uint16_t handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx) override;


private:
    EventGroupHandle_t& _wifiConfigEventGroup;
    IHAL_MEM&           _memDevice;
};

#endif // URI_WIFICONNECT_HPP