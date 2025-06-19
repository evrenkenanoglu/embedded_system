#ifndef URI_WELCOME_HPP
#define URI_WELCOME_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriGet.hpp"

class UriWelcome : public HttpUriGet
{
public:
    UriWelcome();
    ~UriWelcome();
};

#endif // URI_WELCOME_HPP