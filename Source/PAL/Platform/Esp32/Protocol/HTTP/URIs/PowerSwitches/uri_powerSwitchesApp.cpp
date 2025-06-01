#include "uri_powerSwitchesApp.hpp"

#include "Library/UI/HTTP/ui_wifi_power_socket/output/header/ui_wifi_power_sockets.h"

UriPowerSwitchesApp::UriPowerSwitchesApp()
    : HttpUriGet("/powerSwitchesApp", nullptr, this, HTML_UI_WIFI_POWER_SOCKETS_CONTENT)
{
}

UriPowerSwitchesApp::~UriPowerSwitchesApp() {}
