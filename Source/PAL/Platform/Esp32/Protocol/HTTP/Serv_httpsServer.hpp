/**
 * @file Serv_httpsServer.hpp
 * @brief Header file for Serv_httpsServer
 *
 * This file contains declarations for the Serv_httpsServer class and related data types and functions.
 */

#ifndef PAL_HTTPS_SERVER_HPP
#define PAL_HTTPS_SERVER_HPP

#include "HAL/IHal.h"
#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "PAL/Pal.h"
#include "Process/Process.hpp"
#include <esp_https_server.h>
#include <functional>
#include <vector>

class Serv_httpsServer : public PAL_Service
{
private:
    httpd_handle_t            _server;
    httpd_ssl_config_t        _sslConfig;
    std::vector<httpd_uri_t*> _uriList;

    // Websocket callback function for starting the Websocket server
    std::function<void(httpd_handle_t _server)> _websocketStartCb;

    // Websocket callback function for stopping the Websocket server
    std::function<void()> _websocketStopCb;

    // Default certificate and private key for HTTPS server
    const uint8_t* _serverCert;
    const uint8_t* _privateKey;

public:
    Serv_httpsServer(const uint8_t* serverCert, const uint8_t* privateKey);
    ~Serv_httpsServer();

    sys_error_t init() override;

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t restart() override;

public:
    sys_error_t registerUri(const httpd_uri_t* uri);
    sys_error_t unregisterUri(const httpd_uri_t* uri);

    // Register the Websocket server callback function
    void registerWebsocketCbs(std::function<void(httpd_handle_t _server)> startCb, std::function<void()> stopCb);
};

#endif /* PAL_HTTPS_SERVER_HPP */
