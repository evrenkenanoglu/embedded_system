#pragma once

#include "IHttpServer.hpp" // for WsFrameType
#include <cstddef>

class IWebSocketUri
{
public:
    using OnOpen    = int(*)(int clientId, void* user_ctx) ; // return 0 on success
    using OnMessage = int(*)(int clientId, const uint8_t* data, size_t len, WsFrameType type, void* user_ctx) ;
    using OnClose   = void(*)(int clientId, void* user_ctx) ;

    virtual ~IWebSocketUri() = default;

    virtual const char* getPath() const  = 0;
    virtual OnOpen    onOpen()    const  = 0;
    virtual OnMessage onMessage() const  = 0;
    virtual OnClose   onClose()   const  = 0;
    virtual void*     getUserContext() const  = 0;
};