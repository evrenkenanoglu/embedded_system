#pragma once

#include "HttpUri.hpp"

class HttpUriGet : public HttpUri
{
public:
    HttpUriGet(const char* uriName, esp_err_t (*handler)(httpd_req_t*), void* user_ctx = nullptr, const char* htmlContent = nullptr);
    virtual ~HttpUriGet() = default;

    const char* getHtmlContent() const
    {
        return _htmlContent;
    }

private:
    const char* _htmlContent;
};