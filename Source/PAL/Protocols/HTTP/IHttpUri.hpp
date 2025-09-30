#pragma once

#include <cstddef>

class IHttpUri
{
public:
    // Minimal HTTP method set — extend as needed
    enum class HttpMethod : unsigned char
    {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        OPTIONS,
        HEAD,
        UNKNOWN
    };

    // Handler signature optimized for embedded:
    // - request data as pointer+length (no allocations)
    // - response buffer provided by caller with size limit
    // - user_ctx for instance methods/state
    // Return: HTTP status code (e.g., 200) or negative error code
    using Handler = int (*)(const char* req_ptr, std::size_t req_len, char* resp_buf, std::size_t resp_buf_len, void* user_ctx);

    virtual ~IHttpUri() = default;

    // Return path as null-terminated C string (no allocation)
    virtual const char* getPath() const noexcept = 0;

    // Return HTTP method
    virtual HttpMethod getMethod() const noexcept = 0;

    // Return handler function pointer
    virtual Handler getHandler() const noexcept = 0;

    // Optional user context pointer passed to handler
    virtual void* getUserContext() const noexcept = 0;

    // Provide expected maximum response buffer size for this URI (caller may allocate)
    virtual std::size_t getResponseBufferSize() const noexcept
    {
        return 512;
    }
};

class IWebSocketUri
{
public:
    using OnOpen    = int (*)(void* user_ctx);                                    // return 0 on success
    using OnMessage = int (*)(const char* data, std::size_t len, void* user_ctx); // return 0 on success
    using OnClose   = void (*)(void* user_ctx);

    virtual ~IWebSocketUri() = default;

    // Path to register the websocket endpoint (e.g. "/ws")
    virtual const char* getPath() const noexcept = 0;

    // Callbacks
    virtual OnOpen    onOpen() const noexcept    = 0;
    virtual OnMessage onMessage() const noexcept = 0;
    virtual OnClose   onClose() const noexcept   = 0;

    // user context forwarded to callbacks
    virtual void* getUserContext() const noexcept = 0;
};