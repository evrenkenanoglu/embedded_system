#pragma once

#include "HttpUri.hpp"
#include <cstddef>

class HttpUriGet : public HttpUri
{
public:
    HttpUriGet(const char* uriName, Handler handler = nullptr, void* user_ctx = nullptr, const char* static_content = nullptr);

    virtual ~HttpUriGet() = default;
};