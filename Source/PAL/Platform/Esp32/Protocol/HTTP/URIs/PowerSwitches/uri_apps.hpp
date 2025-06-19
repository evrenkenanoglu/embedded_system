#ifndef URI_APPS_HPP
#define URI_APPS_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriGet.hpp"
#include "System/system.h"

class UriApps : public HttpUriGet
{
public:
    UriApps();
    ~UriApps();
};

#endif // URI_APPS_HPP