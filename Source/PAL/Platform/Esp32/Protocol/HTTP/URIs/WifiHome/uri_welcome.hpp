#ifndef URI_WELCOME_HPP
#define URI_WELCOME_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriGet.hpp"

class UriWifiSetup : public HttpUriGet
{
public:
    UriWifiSetup();
    ~UriWifiSetup();
};

#endif // URI_WELCOME_HPP