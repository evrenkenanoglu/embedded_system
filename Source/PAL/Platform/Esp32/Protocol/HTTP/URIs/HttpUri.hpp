#pragma once

#include "PAL/Protocols/HTTP/IHttpUri.hpp"
#include <cstring>
#include <esp_https_server.h>

#include "System/LogHandler.h"

// Platform-specific implementation of IHttpUri for ESP32
class HttpUri : public IHttpUri
{
public:
    HttpUri(const char* path, HttpMethod method, Handler handler, void* user_ctx)
        : _esp{.uri = nullptr, .method = HTTP_GET, .handler = nullptr, .user_ctx = nullptr, .is_websocket = false, .handle_ws_control_frames = false, .supported_subprotocol = nullptr}
        , _handler(handler)
        , _user_ctx(user_ctx)
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

    // IHttpUri
    const char* getPath() const override
    {
        return _path_buf;
    }
    HttpMethod getMethod() const override
    {
        return from_httpd_method(_esp.method);
    }
    Handler getHandler() const override
    {
        return _handler;
    }
    void* getUserContext() const override
    {
        return _user_ctx;
    }

    // platform helper for registration
    const httpd_uri_t& platformUri() const
    {
        return _esp;
    }

protected:
    static esp_err_t      dispatch(httpd_req_t* req);
    static httpd_method_t to_httpd_method(HttpMethod m);
    static HttpMethod     from_httpd_method(httpd_method_t m);
    static esp_err_t      handle_websocket_frame(httpd_req_t* req);

    char        _path_buf[128];
    httpd_uri_t _esp;
    Handler     _handler;
    void*       _user_ctx;
};