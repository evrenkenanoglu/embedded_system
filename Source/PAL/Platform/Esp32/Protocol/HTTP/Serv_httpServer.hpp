/**
 * @file Serv_httpServer.hpp
 * @brief Header file for Serv_httpServer
 *
 * This file contains declarations for the Serv_httpServer class and related data types and functions.
 */

#ifndef PAL_HTTPSERVER_HPP
#define PAL_HTTPSERVER_HPP

#include "HAL/IHal.h"
#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "PAL/Pal.h"
#include "Process/Process.hpp"
#include <esp_http_server.h>
#include <vector>

class Serv_httpServer : public PAL_Service
{
private:
    httpd_handle_t _server;
    httpd_config_t _config;
    std::vector<httpd_uri_t*> _uriList;
    // sys_error_t (*registerUri)(void* params);

public:
    Serv_httpServer();
    ~Serv_httpServer();

    sys_error_t init() override;

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t restart() override;

public:
    sys_error_t registerUri(const httpd_uri_t *uri);
    sys_error_t unregisterUri(const httpd_uri_t *uri);
};

#endif /* PAL_HTTPSERVER_HPP */
