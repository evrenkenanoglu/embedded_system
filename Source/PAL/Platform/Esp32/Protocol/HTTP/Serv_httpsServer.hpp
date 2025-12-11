/**
 * @file Serv_httpsServer.hpp
 * @brief HTTPS server implementation for ESP32
 *
 * This file contains declarations for the Serv_httpsServer class which provides
 * a secure HTTPS server with TLS/SSL support for ESP32 devices. The server supports
 * URI handlers, WebSocket connections, and certificate-based authentication.
 *
 * @note This implementation uses the ESP32 HTTPS server component and mbedTLS for SSL/TLS.
 */

#ifndef PAL_HTTPS_SERVER_HPP
#define PAL_HTTPS_SERVER_HPP

#include "HAL/IHAL/IHal.h"
#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "PAL/Pal.h"
#include "Process/Process.hpp"
#include <esp_https_server.h>
#include <functional>
#include <vector>

class Serv_httpsServer : public PAL_Service
{
private:
    httpd_handle_t            _server;    ///< ESP32 HTTPS server handle
    httpd_ssl_config_t        _sslConfig; ///< SSL configuration for the server
    std::vector<httpd_uri_t*> _uriList;   ///< List of registered URI handlers

    /** @brief WebSocket server start callback function */
    std::function<void(httpd_handle_t _server)> _websocketStartCb;

    /** @brief WebSocket server stop callback function */
    std::function<void()> _websocketStopCb;

    // Default certificate and private key for HTTPS server
    const uint8_t* _serverCert;    ///< Server certificate in DER format
    const size_t   _serverCertLen; ///< Length of server certificate in bytes
    const uint8_t* _privateKey;    ///< Server private key in DER format
    const size_t   _privateKeyLen; ///< Length of private key in bytes

public:
    Serv_httpsServer(const uint8_t* serverCert, const size_t serverCertLen, const uint8_t* privateKey, const size_t privateKeyLen);
    ~Serv_httpsServer();

    sys_error_t init() override;

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t restart() override;

public:
    /**
     * @brief Registers a URI handler with the server
     *
     * @param uri Pointer to a URI handler configuration
     * @return ERROR_SUCCESS on successful registration, ERROR_FAIL otherwise
     *
     * @note If the server is already running, the URI handler is registered immediately.
     *       Otherwise, it's stored for registration when the server starts.
     */
    sys_error_t registerUri(const httpd_uri_t* uri);

    /**
     * @brief Unregisters a URI handler from the server
     *
     * @param uri Pointer to the URI handler to unregister
     * @return ERROR_SUCCESS on successful unregistration, ERROR_FAIL otherwise
     */
    sys_error_t unregisterUri(const httpd_uri_t* uri);

    /**
     * @brief Registers WebSocket callbacks for server start and stop events
     *
     * @param startCb Callback function invoked when the server starts
     * @param stopCb Callback function invoked when the server stops
     *
     * @note These callbacks are used to initialize and clean up WebSocket handlers
     */
    void registerWebsocketCbs(std::function<void(httpd_handle_t _server)> startCb, std::function<void()> stopCb);
};

#endif /* PAL_HTTPS_SERVER_HPP */
