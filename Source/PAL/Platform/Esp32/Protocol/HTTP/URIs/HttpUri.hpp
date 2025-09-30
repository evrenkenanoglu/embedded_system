#pragma once

#include "PAL/Protocols/HTTP/IHttpUri.hpp"
#include <cstring>
#include <esp_https_server.h>

// Platform-specific implementation of IHttpUri for ESP32
class HttpUri : public IHttpUri
{
public:
    HttpUri(const char* path, HttpMethod method, Handler handler, void* user_ctx) noexcept
        : _handler(handler)
        , _user_ctx(user_ctx)
    {
        std::strncpy(_path_buf, path ? path : "/", sizeof(_path_buf));
        _path_buf[sizeof(_path_buf) - 1] = '\0';
        _esp.uri                         = _path_buf;
        _esp.method                      = to_httpd_method(method);
        _esp.handler                     = &HttpUri::dispatch;
        _esp.user_ctx                    = this;
        _esp.is_websocket                = false;
    }

    // IHttpUri
    const char* getPath() const noexcept override
    {
        return _path_buf;
    }
    HttpMethod getMethod() const noexcept override
    {
        return from_httpd_method(_esp.method);
    }
    Handler getHandler() const noexcept override
    {
        return _handler;
    }
    void* getUserContext() const noexcept override
    {
        return _user_ctx;
    }

    // platform helper for registration
    const httpd_uri_t& platformUri() const noexcept
    {
        return _esp;
    }

protected:
    static esp_err_t      dispatch(httpd_req_t* req) noexcept;
    static httpd_method_t to_httpd_method(HttpMethod m) noexcept;
    static HttpMethod     from_httpd_method(httpd_method_t m) noexcept;
    static esp_err_t      handle_websocket_frame(httpd_req_t* req) noexcept;

    char        _path_buf[128];
    httpd_uri_t _esp;
    Handler     _handler;
    void*       _user_ctx;
};