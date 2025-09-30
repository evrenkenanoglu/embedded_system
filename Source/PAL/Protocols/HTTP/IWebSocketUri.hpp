#pragma once

#include "IHttpServer.hpp" // for WsFrameType
#include <cstddef>

class IWebSocketUri
{
public:
    using OnOpen    = int(*)(int clientId, void* user_ctx) noexcept; // return 0 on success
    using OnMessage = int(*)(int clientId, const uint8_t* data, std::size_t len, WsFrameType type, void* user_ctx) noexcept;
    using OnClose   = void(*)(int clientId, void* user_ctx) noexcept;

    virtual ~IWebSocketUri() = default;

    virtual const char* getPath() const noexcept = 0;
    virtual OnOpen    onOpen()    const noexcept = 0;
    virtual OnMessage onMessage() const noexcept = 0;
    virtual OnClose   onClose()   const noexcept = 0;
    virtual void*     getUserContext() const noexcept = 0;
};