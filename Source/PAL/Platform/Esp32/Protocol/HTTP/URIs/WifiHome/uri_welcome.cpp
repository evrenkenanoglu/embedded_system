#include "uri_welcome.hpp"

#include "Library/UI/HTTP/ui_welcome_wifi_connect.h"

UriWifiSetup::UriWifiSetup()
    : HttpUriGet("/welcome", nullptr, this, HTML_UI_WIFI_SETUP_CONTENT)
{
}

UriWifiSetup::~UriWifiSetup() {}
