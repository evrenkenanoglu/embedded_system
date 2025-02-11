/**
 * @file Pal_httpServer.cpp
 * @brief Source file for Pal_httpServer
 *
 * This file contains definitions for the Pal_httpServer class and related data types and functions.
 */

#include "Pal_httpServer.hpp"
#include "HAL/Platform/ESP32/library/logImpl.h"
#include "string.h"
#include <esp_log.h>
#include <sstream>

Pal_httpServer::Pal_httpServer()
    : _server(NULL)                   // Initialize the server handle
    , _config(HTTPD_DEFAULT_CONFIG()) // Initialize the server and configuration
    , _uriList()                      // Initialize the URI list
{
    setStatus(Status::INITIALIZED);
}

Pal_httpServer::~Pal_httpServer()
{
    // destructor implementation
}

sys_error_t Pal_httpServer::init()
{
    return ERROR_SUCCESS;
}

sys_error_t Pal_httpServer::start()
{
    std::stringstream ss;
    ss << "Starting server on port: " << _config.server_port;
    logger().log(ILog::LogLevel::INFO, ss.str());

    if (httpd_start(&_server, &_config) == ESP_OK)
    {
        logger().log(ILog::LogLevel::INFO, "HTTP server started successfully");
        setStatus(Status::STARTED);

        for (auto uri : _uriList)
        {
            if (httpd_register_uri_handler(_server, uri) != ESP_OK)
            {
                logger().log(ILog::LogLevel::ERROR, "Failed to register URI handler");
                return ERROR_FAIL;
            }
        }
    }
    else
    {
        logger().log(ILog::LogLevel::ERROR, "Starting HTTP server failed!");
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t Pal_httpServer::stop()
{
    httpd_stop(_server);
    setStatus(Status::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Pal_httpServer::restart()
{
    stop();
    start();
    return ERROR_SUCCESS;
}

sys_error_t Pal_httpServer::registerUri(const httpd_uri_t *uri)
{
    if (httpd_register_uri_handler(_server, uri) != ESP_OK)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to register URI handler");
        return ERROR_FAIL;
    }

    logger().log(ILog::LogLevel::INFO, "URI handler registered successfully");

    return ERROR_SUCCESS;
}

sys_error_t Pal_httpServer::unregisterUri(const httpd_uri_t *uri)
{
    if (httpd_unregister_uri_handler(_server, uri->uri, uri->method) != ESP_OK)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to unregister URI handler");
        return ERROR_FAIL;
    }

    logger().log(ILog::LogLevel::INFO, "URI handler unregistered successfully");
    return ERROR_SUCCESS;
}