#include "uri_welcome.hpp"

#include "Library/UI/HTTP/ui_welcome/output/header/ui_welcome.h"

UriWelcome::UriWelcome()
    : HttpUriGet("/welcome", nullptr, nullptr, HTML_UI_WELCOME_CONTENT)
{
}

UriWelcome::~UriWelcome() {}
