#pragma once

#include "PAL/Protocols/HTTP/IHttpServer.hpp"
#include "URIs/HttpUri.hpp"
#include <esp_https_server.h>
#include <vector>

class HttpsServer : public IHttpServer
{
public:
    explicit HttpsServer(const HttpServerStartOptions_t& options) ;
    ~HttpsServer() override;

    sys_error_t start()  override;
    sys_error_t stop()  override;

    sys_error_t registerUri(IHttpUri& uri)  override;
    sys_error_t unregisterUri(IHttpUri& uri)  override;

    sys_error_t registerTestUri(httpd_uri_t& uri);

    sys_error_t sendWsMessage(int clientId, const uint8_t* data, size_t len, WsFrameType ws_type)  override;
    sys_error_t broadcastWs(const uint8_t* data, size_t len, WsFrameType ws_type)  override;

    void* nativeHandle() const  override
    {
        return reinterpret_cast<void*>(_server);
    }

private:
    httpd_handle_t                  _server;         ///< ESP32 HTTPS server handle
    httpd_ssl_config_t              _sslConfig;      ///< SSL configuration for the server
    std::vector<HttpUri*>           _registeredUris; ///< List of registered URIs
    SemaphoreHandle_t               _semaphore;      ///< Semaphore for thread safety
    bool                            _started;        ///< Flag indicating if the server is started
    const HttpServerStartOptions_t& _startOptions;   ///< Options used to start the server

    /**
     * @brief Populate server and SSL configurations from start options
     *
     * @param options start options provided by the user
     * @param serverConfig
     * @param sslConfig
     */
    static sys_error_t populate_config(const HttpServerStartOptions_t& options, httpd_ssl_config_t& sslConfig);
    static void        https_server_user_callback(esp_https_server_user_cb_arg_t* user_cb);
};