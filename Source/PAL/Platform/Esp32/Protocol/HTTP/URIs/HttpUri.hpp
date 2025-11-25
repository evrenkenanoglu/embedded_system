#pragma once

#include "PAL/Protocols/HTTP/IHttpUri.hpp"
#include <cstring>
#include <esp_https_server.h>
// Platform-specific implementation of IHttpUri for ESP32
class HttpUri : public IHttpUri
{
public:
    HttpUri(const char* path, HttpMethod method, Handler handler, void* user_ctx);

    const char*        getPath() const override;
    HttpMethod         getMethod() const override;
    void*              getUserContext() const override;
    const httpd_uri_t& platformUri() const;
    size_t             getResponseBufferSize() const override;
    void               setWebSocket(bool is_ws) override;
    void               setUserContext(void* user_ctx) override;
    void               setStaticContent(const char* content, size_t len) override;

protected:
    /**
     * @brief The Main Dispatcher
     *
     * @param platform-specific request pointer
     * @return error code
     */
    static esp_err_t dispatch(httpd_req_t* req);

    /**
     * @brief Convert IHttpUri::HttpMethod to httpd_method_t
     *
     * @param m HttpMethod enum
     * @return httpd_method_t equivalent
     */
    static httpd_method_t to_httpd_method(HttpMethod m);

    /**
     * @brief Convert httpd_method_t to IHttpUri::HttpMethod
     *
     * @param m httpd_method_t enum
     * @return HttpMethod equivalent
     */
    static HttpMethod from_httpd_method(httpd_method_t m);

    /**
     * @brief Handle WebSocket Frame
     *
     * @param req platform-specific request pointer
     * @return esp_err_t
     */
    static esp_err_t handle_websocket_frame(httpd_req_t* req);

    /**
     * @brief Handle GET Request
     *
     * @param req platform-specific request pointer
     * @return esp_err_t
     */
    static esp_err_t handle_get_request(httpd_req_t* req);

    /**
     * @brief Handle Body Request (POST/PUT/PATCH)
     *
     * @param req platform-specific request pointer
     * @return esp_err_t
     */
    static esp_err_t handle_body_request(httpd_req_t* req);

    // Default handler implementation calls user-defined handler
    /**
     * @brief Handler function
     *
     * @param req_ptr Pointer to request data
     * @param req_len Length of request data
     * @param resp_buf Pointer to response buffer
     * @param resp_buf_len Length of response buffer
     * @param user_ctx User-defined context pointer
     * @return uint16_t HTTP status code or error code
     */
    virtual uint16_t handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx);

protected:
    char        _path_buf[128];
    httpd_uri_t _esp;
    Handler     _handler;
    void*       _user_ctx;
    const char* _static_content;
    size_t      _static_len;
};