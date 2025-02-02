#pragma once

#include "system/system.h"
#include <esp_http_server.h>

// Interface class for HTML pages
class IHttpUri
{
public:
    IHttpUri()          = default;
    virtual ~IHttpUri() = default;

    /**
     * @brief Get the URI structure
     *
     * @return const httpd_uri_t& The URI structure
     */
    virtual const httpd_uri_t& getUri() const = 0;
};
