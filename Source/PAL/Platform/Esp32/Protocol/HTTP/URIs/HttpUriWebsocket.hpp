#pragma once

#include "HttpUri.hpp"

class HttpUriWebsocket : public HttpUri
{
public:
    HttpUriWebsocket(const char* uriName, esp_err_t (*handler)(httpd_req_t*), void* user_ctx = nullptr, const char* htmlContent = nullptr);
    virtual ~HttpUriWebsocket() = default;

    const char* getHtmlContent() const
    {
        return _htmlContent;
    }

protected:
private:
    const char*    _htmlContent;
};