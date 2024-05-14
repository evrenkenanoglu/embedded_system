/**
 * @file Proc_httpServer.hpp
 * @brief Header file for Proc_httpServer
 *
 * This file contains declarations for the Proc_httpServer class and related data types and functions.
 */

#ifndef PROC_HTTPSERVER_HPP
#define PROC_HTTPSERVER_HPP

#include "IProcess.hpp"
#include <esp_http_server.h>

class Proc_httpServer : public IProcess
{
private:
    httpd_handle_t _server;
    httpd_config_t _config;

public:
    Proc_httpServer();
    ~Proc_httpServer();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;
};

#endif /* PROC_HTTPSERVER_HPP */
