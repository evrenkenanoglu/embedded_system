#pragma once

#include "HttpUri.hpp"
#include <cstddef>

class HttpUriPut : public HttpUri
{
public:
    // Portable Handler is used (no esp types). If handler == nullptr a simple default
    // echo/ack handler is used and user_ctx is set to 'this'.
    HttpUriPut(const char* uriName, Handler handler = nullptr, void* user_ctx = nullptr) noexcept;

    virtual ~HttpUriPut() = default;

private:
    // Default portable PUT handler: if body present echoes it, otherwise returns 204.
    static int default_put_handler(const char* req_ptr, std::size_t req_len, char* resp_buf, std::size_t resp_buf_len, void* user_ctx) noexcept;
};