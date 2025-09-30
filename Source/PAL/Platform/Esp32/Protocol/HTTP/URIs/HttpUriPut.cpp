#include "HttpUriPut.hpp"
#include <cstring>

HttpUriPut::HttpUriPut(const char* uriName, Handler handler, void* user_ctx) noexcept
    : HttpUri(uriName,
              HttpMethod::PUT,
              handler ? handler : &HttpUriPut::default_put_handler,
              handler ? user_ctx : static_cast<void*>(this))
{
}

int HttpUriPut::default_put_handler(const char* req_ptr,
                                    std::size_t req_len,
                                    char* resp_buf,
                                    std::size_t resp_buf_len,
                                    void* /*user_ctx*/) noexcept
{
    if (!resp_buf || resp_buf_len == 0)
        return 500;

    // No body -> No Content
    if (!req_ptr || req_len == 0)
        return 204;

    // Copy up to resp_buf_len-1 and NUL-terminate (text echo)
    std::size_t max_copy = (resp_buf_len > 0) ? (resp_buf_len - 1) : 0;
    std::size_t to_copy = (req_len < max_copy) ? req_len : max_copy;

    if (to_copy > 0)
        std::memcpy(resp_buf, req_ptr, to_copy);

    resp_buf[to_copy] = '\0';

    // Return OK
    return 200;
}