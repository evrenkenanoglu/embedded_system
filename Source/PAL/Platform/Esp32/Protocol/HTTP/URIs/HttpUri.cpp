#include "HttpUri.hpp"
#include "HttpUriWebsocket.hpp"
#include <cstring>
#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

namespace
{
constexpr size_t REQ_BUF_SZ  = 512;  // Request Buffer Size
constexpr size_t RESP_BUF_SZ = 1024; // Response Buffer Size
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

// Dispatcher: convert httpd_req_t -> Handler call, then send response
esp_err_t HttpUri::dispatch(httpd_req_t* req)
{
    ////////////////////////////////////////////////////////////////////////
    // Initial validation
    ////////////////////////////////////////////////////////////////////////

    SYS_LOG_D("Request received for URI: %s", req ? req->uri : "NULL");

    if (!req)
        return ESP_FAIL;

    auto* self = static_cast<HttpUri*>(req->user_ctx);
    if (!self || !self->_handler)
        return ESP_FAIL;

    httpd_method_t method  = static_cast<httpd_method_t>(req->method);
    const bool     is_head = (method == HTTP_HEAD);

    ////////////////////////////////////////////////////////////////////////
    // Route to WebSocket handler if this URI is a websocket
    ////////////////////////////////////////////////////////////////////////

    if (self->_esp.is_websocket)
    {
        return handle_websocket_frame(req);
    }

    ////////////////////////////////////////////////////////////////////////
    // Determine method and HEAD flag
    ////////////////////////////////////////////////////////////////////////

    // Allow only GET/HEAD, POST, PUT
    if (!(method == HTTP_GET || method == HTTP_HEAD || method == HTTP_POST || method == HTTP_PUT))
    {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method Not Allowed");
        return ESP_FAIL;
    }

    ////////////////////////////////////////////////////////////////////////
    // Build request buffer path + optional query
    ////////////////////////////////////////////////////////////////////////

    char req_buf[REQ_BUF_SZ];
    req_buf[0] = '\0';
    safe_strncpy(req_buf, req->uri, sizeof(req_buf));

    int qlen = httpd_req_get_url_query_len(req);
    if (qlen > 0)
    {
        size_t cur = std::strlen(req_buf);
        if (cur + 1 < REQ_BUF_SZ)
        {
            req_buf[cur] = '?';
            httpd_req_get_url_query_str(req, req_buf + cur + 1, REQ_BUF_SZ - cur - 1);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Read request body when applicable
    ////////////////////////////////////////////////////////////////////////

    size_t req_len = std::strlen(req_buf);

    // Read body only for POST and PUT (up to remaining space); consume rest if truncated
    if (method == HTTP_POST || method == HTTP_PUT)
    {
        size_t content_len = (req->content_len > 0) ? static_cast<size_t>(req->content_len) : 0;
        size_t space       = (req_len < REQ_BUF_SZ) ? (REQ_BUF_SZ - req_len - 1) : 0;
        if (space > 0 && content_len > 0)
        {
            size_t to_read     = (content_len < space) ? content_len : space;
            char*  body_ptr    = req_buf + req_len;
            size_t read_so_far = 0;
            while (read_so_far < to_read)
            {
                int r = httpd_req_recv(req, body_ptr + read_so_far, static_cast<size_t>(to_read - read_so_far));
                if (r <= 0)
                    break;
                read_so_far += static_cast<size_t>(r);
            }
            req_len += read_so_far;
            req_buf[req_len] = '\0';

            // Consume remaining bytes if request body was larger than buffer
            if (content_len > read_so_far)
            {
                size_t remaining = content_len - read_so_far;
                char   tmp[128];
                while (remaining)
                {
                    size_t chunk = (remaining > sizeof(tmp)) ? sizeof(tmp) : remaining;
                    int    r     = httpd_req_recv(req, tmp, chunk);
                    if (r <= 0)
                        break;
                    remaining -= static_cast<size_t>(r);
                }
            }
        }
        else
        {
            // No space: consume body to keep connection consistent
            size_t to_consume = (req->content_len > 0) ? static_cast<size_t>(req->content_len) : 0;
            char   tmp[128];
            while (to_consume)
            {
                size_t chunk = (to_consume > sizeof(tmp)) ? sizeof(tmp) : to_consume;
                int    r     = httpd_req_recv(req, tmp, chunk);
                if (r <= 0)
                    break;
                to_consume -= static_cast<size_t>(r);
            }
        }
    }
    else
    {
        // GET/HEAD: ensure any unexpected body is consumed
        if (req->content_len > 0)
        {
            size_t to_consume = static_cast<size_t>(req->content_len);
            char   tmp[128];
            while (to_consume)
            {
                size_t chunk = (to_consume > sizeof(tmp)) ? sizeof(tmp) : to_consume;
                int    r     = httpd_req_recv(req, tmp, chunk);
                if (r <= 0)
                    break;
                to_consume -= static_cast<size_t>(r);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Prepare response buffer
    ////////////////////////////////////////////////////////////////////////

    char resp_buf[RESP_BUF_SZ];
    resp_buf[0] = '\0';

    ////////////////////////////////////////////////////////////////////////
    // Call portable handler
    ////////////////////////////////////////////////////////////////////////

    int hret = self->_handler(req_buf, req_len, resp_buf, RESP_BUF_SZ, self->_user_ctx);

    ////////////////////////////////////////////////////////////////////////
    // Interpret handler return and send response
    ////////////////////////////////////////////////////////////////////////

    // Map handler return to HTTP response
    if (hret >= 100 && hret <= 599)
    {
        if (is_head)
        {
            char status_str[4] = {0};
            std::snprintf(status_str, sizeof(status_str), "%d", hret);
            httpd_resp_set_status(req, status_str);
            httpd_resp_send(req, nullptr, 0);
            return ESP_OK;
        }

        if (hret == 204)
        {
            httpd_resp_set_status(req, "204");
            httpd_resp_send(req, nullptr, 0);
            return ESP_OK;
        }

        size_t    body_len = std::strlen(resp_buf);
        esp_err_t err      = httpd_resp_send(req, resp_buf, body_len);
        return (err == ESP_OK) ? ESP_OK : ESP_FAIL;
    }
    else
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, resp_buf[0] ? resp_buf : "Handler error");
        return ESP_FAIL;
    }
}

esp_err_t HttpUri::handle_websocket_frame(httpd_req_t* req)
{
    auto* self = static_cast<HttpUriWebsocket*>(req->user_ctx);
    if (!self)
        return ESP_FAIL;

    uint8_t          buf[128] = {0};
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    ws_pkt.type    = HTTPD_WS_TYPE_TEXT; // Default type

    // This is the initial handshake. esp-httpd handles it automatically.
    // We just need to call our onOpen callback.
    if (req->method == HTTP_GET)
    {
        SYS_LOG_I("WebSocket handshake for URI %s, client FD: %d", self->getPath(), httpd_req_to_sockfd(req));
        if (self->onOpen()(self->getUserContext()) != 0)
        {
            // User rejected connection
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    // Get the frame info
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        SYS_LOG_E("httpd_ws_recv_frame failed with %d", ret);
        // This often means the client disconnected. Trigger onClose.
        self->onClose()(self->getUserContext());
        return ret;
    }

    // If frame is larger than our buffer, we need to read the rest
    if (ws_pkt.len > 0 && ws_pkt.len > sizeof(buf))
    {
        // Note: For simplicity, this example doesn't handle fragmented packets
        // or packets larger than the buffer. A production implementation should.
        SYS_LOG_W("WS frame truncated (len %d, buf %d)", ws_pkt.len, sizeof(buf));
    }

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
            self->onMessage()(reinterpret_cast<const char*>(ws_pkt.payload), ws_pkt.len, self->getUserContext());
            break;

        case HTTPD_WS_TYPE_CLOSE:
            // Client wants to close. The server will automatically send a close frame back.
            SYS_LOG_I("WS CLOSE frame received. Closing connection for FD %d", httpd_req_to_sockfd(req));
            self->onClose()(self->getUserContext());
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