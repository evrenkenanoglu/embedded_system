#pragma once

#include "HttpUri.hpp"

class HttpUriPost : public HttpUri
{
public:
    HttpUriPost(const char* uriName, esp_err_t (*handler)(httpd_req_t*), void* user_ctx = nullptr);
    virtual ~HttpUriPost() = default;

private:
};