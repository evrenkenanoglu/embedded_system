#pragma once

#include "HttpUri.hpp"

class HttpUriPut : public HttpUri
{
public:
    HttpUriPut(const char* uriName, esp_err_t (*handler)(httpd_req_t*), void* user_ctx = nullptr);
    virtual ~HttpUriPut() = default;

private:
};