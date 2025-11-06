#pragma once

#include "HttpUri.hpp"
#include <cstddef>

class HttpUriPost : public HttpUri
{
public:
    // Portable Handler is used (no esp types). If handler == nullptr a simple default
    // echo/ack handler is used and user_ctx is set to 'this'.
    HttpUriPost(const char* uriName, Handler handler = nullptr, void* user_ctx = nullptr) ;

    virtual ~HttpUriPost() = default;

private:
    // Default portable POST handler: if body present echoes it, otherwise returns 204.
    static int default_post_handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx) ;
};