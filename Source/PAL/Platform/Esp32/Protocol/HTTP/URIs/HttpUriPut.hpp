#pragma once

#include "HttpUri.hpp"
#include <cstddef>

class HttpUriPut : public HttpUri
{
public:
    HttpUriPut(const char* uriName, Handler handler = nullptr, void* user_ctx = nullptr);

    virtual ~HttpUriPut() = default;
};