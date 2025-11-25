#include "HttpUriWebsocket.hpp"
#include <cstring>

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

HttpUriWebsocket::HttpUriWebsocket(const char* uriName, void* user_ctx, OnOpen on_open, OnMessage on_message, OnClose on_close)
    : HttpUri(
          uriName,                   // URI Name
          IHttpUri::HttpMethod::GET, // Method
          nullptr,                   // Handler
          user_ctx                   // User Context
          )
    , _on_open(on_open)       // Initialize callback members
    , _on_message(on_message) // Initialize callback members
    , _on_close(on_close)     // Initialize callback members
    , _clientId(-1)           // Initialize clientId
{
    // Change user_ctx to this class and set is_websocket flag
    setWebSocket(true);
    setUserContext(this);
}

int HttpUriWebsocket::onOpen(void* user_ctx) const
{
    SYS_LOG_D("HttpUriWebsocket::onOpen called");
    return _on_open ? _on_open(user_ctx) : default_on_open(user_ctx);
}

int HttpUriWebsocket::onMessage(const char* data, size_t len, void* user_ctx) const
{
    SYS_LOG_D("HttpUriWebsocket::onMessage called");
    return _on_message ? _on_message(data, len, user_ctx) : default_on_message(data, len, user_ctx);
}

void HttpUriWebsocket::onClose(void* user_ctx) const
{
    SYS_LOG_D("HttpUriWebsocket::onClose called");
    return _on_close ? _on_close(user_ctx) : default_on_close(user_ctx);
}

// default websocket callbacks (no-op / success)
int HttpUriWebsocket::default_on_open(void* /*user_ctx*/)
{
    return 0;
}
int HttpUriWebsocket::default_on_message(const char* /*data*/, size_t /*len*/, void* /*user_ctx*/)
{
    return 0;
}
void HttpUriWebsocket::default_on_close(void* /*user_ctx*/)
{
    return;
}

void HttpUriWebsocket::setClientId(int id)
{
    _clientId = id;
}