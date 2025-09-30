#pragma once

#include "HttpUri.hpp"
#include <cstddef>

class HttpUriGet : public HttpUri
{
public:
    HttpUriGet(const char* uriName, Handler handler = nullptr, void* user_ctx = nullptr, const char* htmlContent = nullptr) noexcept;

    virtual ~HttpUriGet() = default;

    const char* getHtmlContent() const noexcept { return _htmlContent; }
    std::size_t getHtmlContentLen() const noexcept { return _htmlContentLen; }

private:
    static int default_html_handler(const char* req_ptr, std::size_t req_len, char* resp_buf, std::size_t resp_buf_len, void* user_ctx) noexcept;

    const char* _htmlContent;
    std::size_t _htmlContentLen;
};