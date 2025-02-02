#pragma once

#include "IHttpUri.hpp"
#include "system/system.h"
#include <esp_http_server.h>

// Abstract class for HTML pages
class HttpUri : public IHttpUri
{
public:
    HttpUri(const char* uriName, httpd_method_t method, esp_err_t (*handler)(httpd_req_t*), void* user_ctx = nullptr)
        : uri({.uri = uriName, .method = method, .handler = handler, .user_ctx = user_ctx})
    {
    }
    virtual ~HttpUri() = default;

    const httpd_uri_t& getUri() const override
    {
        return uri;
    }

private:
    const httpd_uri_t uri; // Declare uri as const
};

