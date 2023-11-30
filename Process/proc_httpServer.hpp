/**
 * @file proc_httpServer.hpp
 * @brief Header file for proc_httpServer
 *
 * This file contains declarations for the proc_httpServer class and related data types and functions.
 */

#ifndef PROC_HTTPSERVER_HPP
#define PROC_HTTPSERVER_HPP

#include "IProcess.hpp"
#include <esp_http_server.h>

class proc_httpServer : public IProcess
{
private:
    httpd_handle_t _server;
    httpd_config_t _config;

public:
    proc_httpServer();
    ~proc_httpServer();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;
};

#endif /* PROC_HTTPSERVER_HPP */
