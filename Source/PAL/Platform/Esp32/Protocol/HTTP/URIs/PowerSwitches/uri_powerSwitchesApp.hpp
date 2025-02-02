#ifndef URI_POWERSWITCHESAPP_HPP
#define URI_POWERSWITCHESAPP_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriGet.hpp"
#include "System/system.h"

class UriPowerSwitchesApp : public HttpUriGet
{
public:
    UriPowerSwitchesApp();
    ~UriPowerSwitchesApp();
};

#endif // URI_POWERSWITCHES_APP_HPP