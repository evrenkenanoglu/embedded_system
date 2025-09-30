#include "HttpUriWebsocket.hpp"
#include <cstring>

HttpUriWebsocket::HttpUriWebsocket(
    const char* uriName, Handler handler, void* user_ctx, const char* htmlContent, OnOpen on_open, OnMessage on_message, OnClose on_close) noexcept
    : HttpUri(
          uriName,                                                     //
          IHttpUri::HttpMethod::GET,                                   //
          handler ? handler : &HttpUriWebsocket::default_html_handler, //
          handler ? user_ctx : static_cast<void*>(this)                //
          )
    , _htmlContent(htmlContent)
    , _htmlContentLen(htmlContent ? std::strlen(htmlContent) : 0)
    , _on_open(on_open)
    , _on_message(on_message)
    , _on_close(on_close)
{
    // Change user_ctx to this class and set is_websocket flag
    _esp.is_websocket = true;
    _esp.user_ctx     = this; // Ensure dispatch gets a pointer to HttpUriWebsocket instance
}

int HttpUriWebsocket::default_html_handler(const char* /*req_ptr*/, std::size_t /*req_len*/, char* resp_buf, std::size_t resp_buf_len, void* user_ctx) noexcept
{
    if (!resp_buf || resp_buf_len == 0)
        return 500;

    const HttpUriWebsocket* self     = static_cast<const HttpUriWebsocket*>(user_ctx);
    const char*             body     = (self && self->_htmlContent) ? self->_htmlContent : "";
    std::size_t             body_len = (self) ? self->_htmlContentLen : 0;

    std::size_t max_copy = (resp_buf_len > 0) ? (resp_buf_len - 1) : 0;
    std::size_t to_copy  = (body_len < max_copy) ? body_len : max_copy;

    if (to_copy > 0)
        std::memcpy(resp_buf, body, to_copy);

    resp_buf[to_copy] = '\0';
    (void)resp_buf_len;

    return 200;
}