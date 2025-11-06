#include "HttpUriGet.hpp"
#include <cstring>

HttpUriGet::HttpUriGet(const char* uriName, Handler handler, void* user_ctx, const char* htmlContent)
    : HttpUri(
          uriName,                                               // URI Name
          HttpMethod::GET,                                       // Method
          handler ? handler : &HttpUriGet::default_html_handler, // Handler
          user_ctx                                               // User Context
          )
    , _htmlContent(htmlContent)
    , _htmlContentLen(htmlContent ? std::strlen(htmlContent) : 0)
{

    if (!handler && !user_ctx)
    {
        SYS_LOG_I("No handler and no user_ctx provided, using default HTML handler");
        // If no handler and no user_ctx provided, set user_ctx to this for default handler
        this->_user_ctx = this;
    }
}

int HttpUriGet::default_html_handler(const char* /*req_ptr*/, size_t /*req_len*/, char* resp_buf, size_t resp_buf_len, void* user_ctx)
{
    if (!resp_buf || resp_buf_len == 0)
        return 500;

    const HttpUriGet* self     = static_cast<const HttpUriGet*>(user_ctx);
    const char*       body     = (self && self->_htmlContent) ? self->_htmlContent : "";
    size_t            body_len = (self) ? self->_htmlContentLen : 0;

    // leave space for NUL
    size_t max_copy = (resp_buf_len > 0) ? (resp_buf_len - 1) : 0;
    size_t to_copy  = (body_len < max_copy) ? body_len : max_copy;

    if (to_copy)
        std::memcpy(resp_buf, body, to_copy);
    resp_buf[to_copy] = '\0';

    return 200;
}