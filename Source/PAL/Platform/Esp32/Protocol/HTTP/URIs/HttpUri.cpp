#include "HttpUri.hpp"
#include "HttpUriWebsocket.hpp"
#include <cstring>

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

namespace
{
constexpr size_t REQ_BUF_SZ      = 512;  // Request Buffer Size
constexpr size_t RESP_BUF_SZ     = 1024; // Response Buffer Size
constexpr size_t MAX_WS_FRAME_SZ = 256;  // Max WebSocket Frame Size
} // namespace

// Map portable HttpMethod to esp-idf httpd_method_t
httpd_method_t HttpUri::to_httpd_method(HttpMethod m)
{
    switch (m)
    {
        case HttpMethod::GET:
            return HTTP_GET;
        case HttpMethod::POST:
            return HTTP_POST;
        case HttpMethod::PUT:
            return HTTP_PUT;
        case HttpMethod::DELETE:
            return HTTP_DELETE;
        case HttpMethod::PATCH:
            return HTTP_PATCH;
        case HttpMethod::OPTIONS:
            return HTTP_OPTIONS;
        case HttpMethod::HEAD:
            return HTTP_HEAD;
        default:
            return HTTP_GET;
    }
}

// Map esp-idf httpd_method_t back to portable HttpMethod
IHttpUri::HttpMethod HttpUri::from_httpd_method(httpd_method_t m)
{
    switch (m)
    {
        case HTTP_GET:
            return HttpMethod::GET;
        case HTTP_POST:
            return HttpMethod::POST;
        case HTTP_PUT:
            return HttpMethod::PUT;
        case HTTP_DELETE:
            return HttpMethod::DELETE;
        case HTTP_PATCH:
            return HttpMethod::PATCH;
        case HTTP_OPTIONS:
            return HttpMethod::OPTIONS;
        case HTTP_HEAD:
            return HttpMethod::HEAD;
        default:
            return HttpMethod::UNKNOWN;
    }
}

// Minimal safe strncpy helper
static inline void safe_strncpy(char* dst, const char* src, size_t dst_len)
{
    if (!dst || dst_len == 0)
        return;
    if (!src)
    {
        dst[0] = '\0';
        return;
    }
    std::strncpy(dst, src, dst_len);
    dst[dst_len - 1] = '\0';
}

HttpUri::HttpUri(const char* path, HttpMethod method, Handler handler, void* user_ctx)
    : _esp{.uri = nullptr, .method = HTTP_GET, .handler = nullptr, .user_ctx = nullptr, .is_websocket = false, .handle_ws_control_frames = false, .supported_subprotocol = nullptr}
    , _handler(handler)
    , _user_ctx(user_ctx)
    , _static_content(nullptr)
    , _static_len(0)
{
    std::strncpy(_path_buf, path ? path : "/", sizeof(_path_buf));
    _path_buf[sizeof(_path_buf) - 1] = '\0';

    _esp.uri                      = _path_buf;
    _esp.method                   = to_httpd_method(method);
    _esp.handler                  = &HttpUri::dispatch;
    _esp.user_ctx                 = this;
    _esp.is_websocket             = false;
    _esp.handle_ws_control_frames = false;
}

// ========================================================================
// 1. The Main Dispatcher
// ========================================================================
esp_err_t HttpUri::dispatch(httpd_req_t* req)
{
    RETURN_IF_ERROR(!req || !req->user_ctx, ESP_FAIL, SYS_LOG_D("Invalid request or user context"));

    auto* self = static_cast<HttpUri*>(req->user_ctx);

    // 1. WebSocket
    if (self->_esp.is_websocket)
    {
        return self->handle_websocket_frame(req);
    }

    // 2. Route based on Method
    switch (req->method)
    {
        case HTTP_GET:
            return self->handle_get_request(req);

        case HTTP_POST:
        case HTTP_PUT:
            return self->handle_body_request(req);

        default:
            httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
            return ESP_FAIL;
    }
}

// ========================================================================
// 2. GET Handler (Zero Copy OR Buffered)
// ========================================================================
esp_err_t HttpUri::handle_get_request(httpd_req_t* req)
{
    auto* self = static_cast<HttpUri*>(req->user_ctx);

    SYS_LOG_D("Handling GET for %s", req->uri);

    // Scenario A: Static Content (Optimization: Zero Copy)
    if (self->_static_content != nullptr)
    {
        SYS_LOG_D("Serving static content for %s", req->uri);
        // Directly send the pointer. No stack buffer. No strcpy.
        return httpd_resp_send(req, self->_static_content, self->_static_len);
    }

    // Scenario B: User Handler (Standard: Buffer Copy)
    // Since GET usually has no body, we just prepare the response buffer
    char resp_buf[RESP_BUF_SZ] = {0};

    // Pass empty req_buf, 0 len
    int ret = self->handler("", 0, resp_buf, RESP_BUF_SZ, self->_user_ctx);

    // Handle success
    if (ret >= HTTP::RESPONSE::OK && ret < HTTP::RESPONSE::MULTIPLE_CHOICES)
    {
        return httpd_resp_send(req, resp_buf, strlen(resp_buf));
    }
    // Handle errors...

    // Fallback: Empty 200 OK if nothing defined
    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

// ========================================================================
// 3. POST/PUT/PATCH Handler (Buffered)
// ========================================================================
esp_err_t HttpUri::handle_body_request(httpd_req_t* req)
{
    SYS_LOG_D("Handling POST/PUT/PATCH for %s", req->uri);

    auto* self = static_cast<HttpUri*>(req->user_ctx);

    // Read the body into a buffer
    char req_buf[REQ_BUF_SZ] = {0};

    int req_len = httpd_req_recv(req, req_buf, REQ_BUF_SZ - 1); // REQ_BUF_SZ - 1 for 
    if (req_len <= 0)
    {
        if (req_len == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        else if (req_len == HTTPD_SOCK_ERR_INVALID)
        {
            SYS_LOG_E("Invalid argument in httpd_req_recv");
        }
        else
        {
            SYS_LOG_E("Error receiving data from request body: %d", req_len);
        }
        return ESP_FAIL;
    }

    // Null terminate the content string
    req_buf[req_len] = '\0';

    SYS_LOG_D("Received %d bytes in request body", req_len);

    // Scenario A: User Handler
    char resp_buf[RESP_BUF_SZ] = {0};

    uint16_t ret = self->handler(req_buf, req_len, resp_buf, RESP_BUF_SZ, self->_user_ctx);

    // Handle success
    if (resp_buf[0] != '\0' && (ret >= HTTP::RESPONSE::OK && ret < HTTP::RESPONSE::MULTIPLE_CHOICES))
    {
        return httpd_resp_send(req, resp_buf, strlen(resp_buf));
    }
    else if (ret >= HTTP::RESPONSE::BAD_REQUEST)
    {
        // Handle client errors
        httpd_resp_send_err(req, static_cast<httpd_err_code_t>(ret), "Client Error");
        return ESP_FAIL;
    }

    // Fallback: Empty 200 OK if nothing defined
    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

// ========================================================================
// 4. WebSocket Frame Handler
// ========================================================================

esp_err_t HttpUri::handle_websocket_frame(httpd_req_t* req)
{
    auto* self = static_cast<HttpUriWebsocket*>(req->user_ctx);

    uint8_t          buf[MAX_WS_FRAME_SZ] = {0};
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    ws_pkt.type    = HTTPD_WS_TYPE_TEXT; // Default type

    int clientId = httpd_req_to_sockfd(req);

    // This is the initial handshake. esp-httpd handles it automatically.
    // We just need to call our onOpen callback.
    if (req->method == HTTP_GET)
    {
        // Handshake complete
        self->setClientId(clientId);
        SYS_LOG_I("WebSocket handshake for URI %s, client FD: %d", req->uri, clientId);

        if (self->onOpen(self->_user_ctx) != 0)
        {
            // User rejected connection
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    // Get the frame info
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);

    // Error handling
    if (ret != ESP_OK)
    {
        SYS_LOG_E("httpd_ws_recv_frame failed with %d", ret);
        // This often means the client disconnected. Trigger onClose.
        self->onClose(self->_user_ctx);
        return ret;
    }

    if (ws_pkt.len > MAX_WS_FRAME_SZ)
    {
        SYS_LOG_W("WebSocket frame too large: %zu bytes (max %zu). Truncating.", ws_pkt.len, MAX_WS_FRAME_SZ);
    }
    size_t max_read = (ws_pkt.len >= sizeof(buf)) ? (sizeof(buf) - 1) : ws_pkt.len;

    ws_pkt.len = max_read; // Some ESP-IDF versions do not update ws_pkt.len after reading
    httpd_ws_recv_frame(req, &ws_pkt, max_read);

    SYS_LOG_D("Received WebSocket frame of type %d, length %zu from client FD %d", ws_pkt.type, ws_pkt.len, clientId);

    // Handle different frame types
    switch (ws_pkt.type)
    {
        case HTTPD_WS_TYPE_TEXT:
        case HTTPD_WS_TYPE_BINARY:
            // Null-terminate for safety if it's text
            if (ws_pkt.type == HTTPD_WS_TYPE_TEXT)
            {
                buf[ws_pkt.len] = '\0';
            }
            self->onMessage(reinterpret_cast<const char*>(ws_pkt.payload), ws_pkt.len, self->_user_ctx);
            break;

        case HTTPD_WS_TYPE_CLOSE:
            // Client wants to close. The server will automatically send a close frame back.
            SYS_LOG_I("WS CLOSE frame received. Closing connection for FD %d", clientId);
            self->onClose(self->_user_ctx);
            // The connection will be closed by esp-httpd after this handler returns.
            break;

        case HTTPD_WS_TYPE_PING:
            // The server automatically replies with a PONG frame. Nothing to do here.
            SYS_LOG_D("WS PING frame received");
            break;

        case HTTPD_WS_TYPE_PONG:
            SYS_LOG_D("WS PONG frame received");
            break;

        case HTTPD_WS_TYPE_CONTINUE:
            SYS_LOG_D("WS CONTINUE frame received - not handled in this example");
            break;

        default:
            break;
    }

    return ESP_OK;
}

uint16_t HttpUri::handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx)
{
    SYS_LOG_D("Default handler called for request of length %zu", req_len);

    return _handler ? _handler(req_ptr, req_len, resp_buf, resp_buf_len, user_ctx) : static_cast<uint16_t>(HTTP::RESPONSE::OK);
}

// IHttpUri
const char* HttpUri::getPath() const
{
    return _path_buf;
}
IHttpUri::HttpMethod HttpUri::getMethod() const
{
    return from_httpd_method(_esp.method);
}

void* HttpUri::getUserContext() const
{
    return _user_ctx;
}

// platform helper for registration
const httpd_uri_t& HttpUri::platformUri() const
{
    return _esp;
}

size_t HttpUri::getResponseBufferSize() const
{
    return RESP_BUF_SZ;
}

void HttpUri::setWebSocket(bool is_ws)
{
    _esp.is_websocket = is_ws;
}

void HttpUri::setUserContext(void* user_ctx)
{
    _user_ctx = user_ctx;
}

void HttpUri::setStaticContent(const char* content, size_t len)
{
    _static_content = content;
    _static_len     = len;
}