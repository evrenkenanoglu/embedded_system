#include "uri_wifiSetup.hpp"

#include "Library/UI/HTTP/ui_wifi_setup/output/header/ui_wifi_setup.h"

UriWifiSetup::UriWifiSetup()
    : HttpUriGet("/wifiSetup", nullptr, this, HTML_UI_WIFI_SETUP_CONTENT)
{
}

UriWifiSetup::~UriWifiSetup() {}
