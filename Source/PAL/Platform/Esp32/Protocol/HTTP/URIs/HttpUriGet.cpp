#include "HttpUriGet.hpp"
#include <cstring>

HttpUriGet::HttpUriGet(const char* uriName, Handler handler, void* user_ctx, const char* htmlContent) noexcept
    : HttpUri(
          uriName,                                               //
          HttpMethod::GET,                                       //
          handler ? handler : &HttpUriGet::default_html_handler, //
          handler ? user_ctx : static_cast<void*>(this)          //
          )
    , _htmlContent(htmlContent)
    , _htmlContentLen(htmlContent ? std::strlen(htmlContent) : 0)
{
}

int HttpUriGet::default_html_handler(const char* /*req_ptr*/, std::size_t /*req_len*/, char* resp_buf, std::size_t resp_buf_len, void* user_ctx) noexcept
{
    if (!resp_buf || resp_buf_len == 0)
        return 500;

    const HttpUriGet* self     = static_cast<const HttpUriGet*>(user_ctx);
    const char*       body     = (self && self->_htmlContent) ? self->_htmlContent : "";
    std::size_t       body_len = (self) ? self->_htmlContentLen : 0;

    // leave space for NUL
    std::size_t max_copy = (resp_buf_len > 0) ? (resp_buf_len - 1) : 0;
    std::size_t to_copy  = (body_len < max_copy) ? body_len : max_copy;

    if (to_copy)
        std::memcpy(resp_buf, body, to_copy);
    resp_buf[to_copy] = '\0';

    return 200;
}