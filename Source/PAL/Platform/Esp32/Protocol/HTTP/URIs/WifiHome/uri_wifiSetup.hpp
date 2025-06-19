#ifndef URI_WIFI_SETUP_HPP
#define URI_WIFI_SETUP_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriGet.hpp"

class UriWifiSetup : public HttpUriGet
{
public:
    UriWifiSetup();
    ~UriWifiSetup();
};

#endif // URI_WIFI_SETUP_HPP