#ifndef URI_WIFICONNECT_HPP
#define URI_WIFICONNECT_HPP

#include "HAL/IHal.h"
#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriPost.hpp"

class UriWifiConnect : public HttpUriPost
{
public:
    UriWifiConnect(EventGroupHandle_t& wifiConfigEventGroup, IHAL_MEM& memDevice);
    ~UriWifiConnect();

    /**
     * @brief Get the Wifi Config Event Group Handle
     *
     * @return EventGroupHandle_t&
     */
    EventGroupHandle_t& getWifiConfigEventGroup();

    /**
     * @brief Get the Mem Device object
     *
     * @return IHAL_MEM&
     */
    IHAL_MEM& getMemDevice() const;

private:
    EventGroupHandle_t& _wifiConfigEventGroup;
    IHAL_MEM&           _memDevice;
};

#endif // URI_WIFICONNECT_HPP