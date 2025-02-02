#include "uri_welcome.hpp"

#include "Library/UI/HTTP/ui_welcome_wifi_connect.h"

UriWelcome::UriWelcome()
    : HttpUriGet("/welcome", nullptr, this, HTML_UI_WELCOME_WIFI_CONNECT_CONTENT)
{
}

UriWelcome::~UriWelcome() {}
