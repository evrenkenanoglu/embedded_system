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
    uint16_t    port            = 80;      // default to secure port
    bool        use_tls         = false;   // default to no TLS
    const char* tls_cert_pem    = nullptr; // tls certificate in PEM format
    const char* tls_key_pem     = nullptr; // tls private key in PEM format
    std::size_t task_stack_size = 8192;    // stack size for server task
    int         task_priority   = 5;       // task priority for server task
    std::size_t max_connections = 8;       // max simultaneous connections
} HttpServerStartOptions_t;

class IHttpServer
{
public:
    virtual ~IHttpServer() = default;

    // Start/stop server. Pass nullptr to use defaults.
    virtual sys_error_t start(const HttpServerStartOptions_t* options = nullptr) noexcept = 0;
    virtual sys_error_t stop() noexcept                                                   = 0;

    // Register/unregister HTTP URI. Server does NOT take ownership.
    virtual sys_error_t registerUri(IHttpUri& uri) noexcept   = 0;
    virtual sys_error_t unregisterUri(IHttpUri& uri) noexcept = 0;

    // WebSocket send helpers. clientId is implementation-specific (opaque).
    virtual sys_error_t sendWsMessage(int clientId, const uint8_t* data, std::size_t len, WsFrameType ws_type) noexcept = 0;
    virtual sys_error_t broadcastWs(const uint8_t* data, std::size_t len, WsFrameType ws_type) noexcept                 = 0;

    // Optional: expose native handle (platform-specific) for advanced use.
    virtual void* nativeHandle() const noexcept
    {
        return nullptr;
    }
};