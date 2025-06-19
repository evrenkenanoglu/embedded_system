#include "uri_powerSwitchesApp.hpp"

#include "Library/UI/HTTP/PowerSwitches/ui_wifi_power_sockets/output/header/ui_wifi_power_sockets.h"

UriPowerSwitchesApp::UriPowerSwitchesApp()
    : HttpUriGet("/powerSwitchesApp", nullptr, this, HTML_UI_WIFI_POWER_SOCKETS_CONTENT)
{
}

UriPowerSwitchesApp::~UriPowerSwitchesApp() {}
