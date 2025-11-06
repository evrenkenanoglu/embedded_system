#pragma once

#include "HttpUri.hpp"
#include <cstddef>

class HttpUriGet : public HttpUri
{
public:
    HttpUriGet(const char* uriName, Handler handler = nullptr, void* user_ctx = nullptr, const char* htmlContent = nullptr);

    virtual ~HttpUriGet() = default;

    const char* getHtmlContent() const
    {
        return _htmlContent;
    }
    size_t getHtmlContentLen() const
    {
        return _htmlContentLen;
    }

private:
    static int default_html_handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx);

    const char* _htmlContent;
    size_t      _htmlContentLen;
};