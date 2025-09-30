// ...existing code...
#pragma once

#include "HttpUri.hpp"
#include "PAL/Protocols/HTTP/IHttpUri.hpp"
#include "System/system.h"

class HttpUriWebsocket : public HttpUri, public IWebSocketUri
{
public:
    // handler: HTTP GET handler (serves htmlContent if handler == nullptr)
    // user_ctx is forwarded to both HTTP handler and websocket callbacks.
    HttpUriWebsocket(
        const char* uriName, Handler handler = nullptr, void* user_ctx = nullptr, const char* htmlContent = nullptr, OnOpen on_open = nullptr, OnMessage on_message = nullptr,
        OnClose on_close = nullptr) noexcept;

    virtual ~HttpUriWebsocket() = default;

    const char* getHtmlContent() const noexcept
    {
        return _htmlContent;
    }
    std::size_t getHtmlContentLen() const noexcept
    {
        return _htmlContentLen;
    }

    // IWebSocketUri implementation (for platform manager)
    const char* getPath() const noexcept override
    {
        return HttpUri::getPath();
    }
    OnOpen onOpen() const noexcept override
    {
        return _on_open ? _on_open : &HttpUriWebsocket::default_on_open;
    }
    OnMessage onMessage() const noexcept override
    {
        return _on_message ? _on_message : &HttpUriWebsocket::default_on_message;
    }
    OnClose onClose() const noexcept override
    {
        return _on_close ? _on_close : &HttpUriWebsocket::default_on_close;
    }
    void* getUserContext() const noexcept override
    {
        return HttpUri::getUserContext();
    }

private:
    // default GET page handler (copies _htmlContent into resp_buf)
    static int default_html_handler(const char* req_ptr, std::size_t req_len, char* resp_buf, std::size_t resp_buf_len, void* user_ctx) noexcept;

    // default websocket callbacks (no-op / success)
    static int default_on_open(void* /*user_ctx*/) noexcept
    {
        return 0;
    }
    static int default_on_message(const char* /*data*/, std::size_t /*len*/, void* /*user_ctx*/) noexcept
    {
        return 0;
    }
    static void default_on_close(void* /*user_ctx*/) noexcept {}

    const char* _htmlContent;
    std::size_t _htmlContentLen;

    OnOpen    _on_open;
    OnMessage _on_message;
    OnClose   _on_close;
};