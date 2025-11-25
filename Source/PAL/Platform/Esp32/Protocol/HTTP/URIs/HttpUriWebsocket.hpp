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
    HttpUriWebsocket(const char* uriName, void* user_ctx = nullptr, OnOpen on_open = nullptr, OnMessage on_message = nullptr, OnClose on_close = nullptr);

    virtual ~HttpUriWebsocket() = default;

    int  onOpen(void* user_ctx) const override;
    int  onMessage(const char* data, size_t len, void* user_ctx) const override;
    void onClose(void* user_ctx) const override;

    void setClientId(int id);

private:
    static int  default_on_open(void* user_ctx);
    static int  default_on_message(const char* data, size_t len, void* user_ctx);
    static void default_on_close(void* user_ctx);

private:
    OnOpen    _on_open;
    OnMessage _on_message;
    OnClose   _on_close;

    int _clientId;
};