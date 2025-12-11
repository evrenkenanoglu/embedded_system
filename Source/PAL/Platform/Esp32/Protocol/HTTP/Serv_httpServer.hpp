/**
 * @file Serv_httpServer.hpp
 * @brief Header file for Serv_httpServer
 *
 * This file contains declarations for the Serv_httpServer class and related data types and functions.
 */

#ifndef PAL_HTTP_SERVER_HPP
#define PAL_HTTP_SERVER_HPP

#include "HAL/IHAL/IHal.h"
#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "PAL/Pal.h"
#include "Process/Process.hpp"
#include <esp_http_server.h>
#include <functional>
#include <vector>


class Serv_httpServer : public PAL_Service
{
private:
    httpd_handle_t            _server;
    httpd_config_t            _config;
    std::vector<httpd_uri_t*> _uriList;

    // Websocket callback function for starting the Websocket server
    std::function<void(httpd_handle_t _server)> _websocketStartCb;

    // Websocket callback function for stopping the Websocket server
    std::function<void()> _websocketStopCb;



public:
    Serv_httpServer();
    ~Serv_httpServer();

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

#endif /* PAL_HTTP_SERVER_HPP */
