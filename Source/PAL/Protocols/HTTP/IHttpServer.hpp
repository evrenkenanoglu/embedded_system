#pragma once

#include "System/system.h"

#include "IHttpUri.hpp"

enum class WsFrameType : uint8_t
{
    TEXT   = 1,
    BINARY = 2,
    PING   = 3,
    PONG   = 4,
    CLOSE  = 5
};

typedef struct
{
    uint16_t       port            = 80;      // default to secure port
    bool           use_tls         = false;   // default to no TLS
    const uint8_t* tls_cert_pem    = nullptr; // tls certificate in PEM format
    size_t         tls_cert_len    = 0;       // length of tls certificate
    const uint8_t* tls_key_pem     = nullptr; // tls private key in PEM format
    size_t         tls_key_len     = 0;       // length of tls private key
    size_t         task_stack_size = 8192;    // stack size for server task
    int            task_priority   = 5;       // task priority for server task
    size_t         max_connections = 7;       // max simultaneous connections
} HttpServerStartOptions_t;

class IHttpServer
{
public:
    virtual ~IHttpServer() = default;

    // Start/stop server. Pass nullptr to use defaults.
    virtual sys_error_t start()  = 0;
    virtual sys_error_t stop()   = 0;

    // Register/unregister HTTP URI. Server does NOT take ownership.
    virtual sys_error_t registerUri(IHttpUri& uri)    = 0;
    virtual sys_error_t unregisterUri(IHttpUri& uri)  = 0;

    // WebSocket send helpers. clientId is implementation-specific (opaque).
    virtual sys_error_t sendWsMessage(int clientId, const uint8_t* data, size_t len, WsFrameType ws_type)  = 0;
    virtual sys_error_t broadcastWs(const uint8_t* data, size_t len, WsFrameType ws_type)                  = 0;

    // Optional: expose native handle (platform-specific) for advanced use.
    virtual void* nativeHandle() const 
    {
        return nullptr;
    }
};