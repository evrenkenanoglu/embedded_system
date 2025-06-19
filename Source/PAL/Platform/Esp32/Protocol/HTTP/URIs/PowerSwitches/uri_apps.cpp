#include "uri_apps.hpp"

#include "Library/UI/HTTP/PowerSwitches/applications/output/header/applications.h"

UriApps::UriApps()
    : HttpUriGet("/applications", nullptr, this, HTML_APPLICATIONS_CONTENT)
{
}

UriApps::~UriApps() {}
