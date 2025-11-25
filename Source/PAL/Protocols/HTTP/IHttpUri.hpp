#pragma once

#include <cstddef>
#include <cstdint>

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
    using Handler = uint16_t (*)(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx);

    virtual ~IHttpUri() = default;

    /**
     * @brief Get the Path object
     *
     * @return URI path string
     */
    virtual const char* getPath() const = 0;

    /**
     * @brief Get the Method object
     *
     * @return HTTP method enum
     */
    virtual HttpMethod getMethod() const = 0;

    /**
     * @brief Get the User Context object
     *
     * @return Pointer to user-defined context
     */
    virtual void* getUserContext() const = 0;

    /**
     * @brief Get the Response Buffer Size object
     *
     * @return Size of the response buffer to be used in handler
     */
    virtual size_t getResponseBufferSize() const = 0;

    /**
     * @brief Set whether this URI is a WebSocket
     *
     * @param is_ws true if WebSocket, false otherwise
     */
    virtual void setWebSocket(bool is_ws) = 0;

    /**
     * @brief Set the User Context object
     *
     * @param user_ctx Pointer to user-defined context
     */
    virtual void setUserContext(void* user_ctx) = 0;

    /**
     * @brief Set the Static Content object
     *
     * @param content Pointer to static content
     * @param len Length of static content
     */
    virtual void setStaticContent(const char* content, size_t len) = 0;
};

class IWebSocketUri
{
public:
    using OnOpen    = int (*)(void* user_ctx);                               // return 0 on success
    using OnMessage = int (*)(const char* data, size_t len, void* user_ctx); // return 0 on success
    using OnClose   = void (*)(void* user_ctx);

    virtual ~IWebSocketUri() = default;

    // Callbacks
    virtual int  onOpen(void* user_ctx) const                                  = 0;
    virtual int  onMessage(const char* data, size_t len, void* user_ctx) const = 0;
    virtual void onClose(void* user_ctx) const                                 = 0;
};

namespace HTTP
{
enum RESPONSE
{
    // 1xx Informational Responses
    CONTINUE            = 100,
    SWITCHING_PROTOCOLS = 101,
    PROCESSING          = 102,
    EARLY_HINTS         = 103,

    // 2xx Successful Responses
    OK                            = 200,
    CREATED                       = 201,
    ACCEPTED                      = 202,
    NON_AUTHORITATIVE_INFORMATION = 203,
    NO_CONTENT                    = 204,
    RESET_CONTENT                 = 205,
    PARTIAL_CONTENT               = 206,
    MULTI_STATUS                  = 207,
    ALREADY_REPORTED              = 208,
    IM_USED                       = 226,

    // 3xx Redirection Messages
    MULTIPLE_CHOICES   = 300,
    MOVED_PERMANENTLY  = 301,
    FOUND              = 302,
    SEE_OTHER          = 303,
    NOT_MODIFIED       = 304,
    USE_PROXY          = 305,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,

    // 4xx Client Error Responses
    BAD_REQUEST                     = 400,
    UNAUTHORIZED                    = 401,
    PAYMENT_REQUIRED                = 402,
    FORBIDDEN                       = 403,
    NOT_FOUND                       = 404,
    METHOD_NOT_ALLOWED              = 405,
    NOT_ACCEPTABLE                  = 406,
    PROXY_AUTHENTICATION_REQUIRED   = 407,
    REQUEST_TIMEOUT                 = 408,
    CONFLICT                        = 409,
    GONE                            = 410,
    LENGTH_REQUIRED                 = 411,
    PRECONDITION_FAILED             = 412,
    PAYLOAD_TOO_LARGE               = 413,
    URI_TOO_LONG                    = 414,
    UNSUPPORTED_MEDIA_TYPE          = 415,
    RANGE_NOT_SATISFIABLE           = 416,
    EXPECTATION_FAILED              = 417,
    IM_A_TEAPOT                     = 418,
    MISDIRECTED_REQUEST             = 421,
    UNPROCESSABLE_ENTITY            = 422,
    LOCKED                          = 423,
    FAILED_DEPENDENCY               = 424,
    TOO_EARLY                       = 425,
    UPGRADE_REQUIRED                = 426,
    PRECONDITION_REQUIRED           = 428,
    TOO_MANY_REQUESTS               = 429,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    UNAVAILABLE_FOR_LEGAL_REASONS   = 451,

    // 5xx Server Error Responses
    INTERNAL_SERVER_ERROR           = 500,
    NOT_IMPLEMENTED                 = 501,
    BAD_GATEWAY                     = 502,
    SERVICE_UNAVAILABLE             = 503,
    GATEWAY_TIMEOUT                 = 504,
    HTTP_VERSION_NOT_SUPPORTED      = 505,
    VARIANT_ALSO_NEGOTIATES         = 506,
    INSUFFICIENT_STORAGE            = 507,
    LOOP_DETECTED                   = 508,
    NOT_EXTENDED                    = 510,
    NETWORK_AUTHENTICATION_REQUIRED = 511
};
}