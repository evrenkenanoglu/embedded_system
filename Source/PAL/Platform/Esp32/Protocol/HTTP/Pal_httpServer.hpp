/**
 * @file Pal_httpServer.hpp
 * @brief Header file for Pal_httpServer
 *
 * This file contains declarations for the Pal_httpServer class and related data types and functions.
 */

#ifndef PAL_HTTPSERVER_HPP
#define PAL_HTTPSERVER_HPP

#include "HAL/IHal.h"
#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "PAL/Pal.h"
#include "Process/Process.hpp"
#include <esp_http_server.h>
#include <vector>

class Pal_httpServer : public PAL_NetworkService
{
private:
    httpd_handle_t _server;
    httpd_config_t _config;
    std::vector<httpd_uri_t*> _uriList;
    // sys_error_t (*registerUri)(void* params);

public:
    Pal_httpServer();
    ~Pal_httpServer();

    sys_error_t init() override;

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t restart() override;

public:
    sys_error_t registerUri(const httpd_uri_t *uri);
    sys_error_t unregisterUri(const httpd_uri_t *uri);
};

#endif /* PAL_HTTPSERVER_HPP */
