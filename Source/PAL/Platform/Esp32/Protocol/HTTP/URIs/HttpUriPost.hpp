#pragma once

#include "HttpUri.hpp"
#include <cstddef>

class HttpUriPost : public HttpUri
{
public:
    // Portable Handler is used (no esp types). If handler == nullptr a simple default
    // echo/ack handler is used and user_ctx is set to 'this'.
    HttpUriPost(const char* uriName, Handler handler = nullptr, void* user_ctx = nullptr);

    virtual ~HttpUriPost() = default;
};