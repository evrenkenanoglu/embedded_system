/**
 * @file Serv_httpServer.cpp
 * @brief Source file for Serv_httpServer
 *
 * This file contains definitions for the Serv_httpServer class and related data types and functions.
 */

#include "Serv_httpServer.hpp"
#include "System/LogHandler.h"
#include "string.h"
#include <esp_log.h>
#include <sstream>

Serv_httpServer::Serv_httpServer()
    : _server(NULL)                   // Initialize the server handle
    , _config(HTTPD_DEFAULT_CONFIG()) // Initialize the server and configuration
    , _uriList()                      // Initialize the URI list
    , _websocketStartCb(nullptr)      // Initialize the WebSocket start callback
    , _websocketStopCb(nullptr)       // Initialize the WebSocket stop callback
{
    setStatus(Status::INITIALIZED);
}

Serv_httpServer::~Serv_httpServer()
{
    // destructor implementation
}

sys_error_t Serv_httpServer::init()
{
    return ERROR_SUCCESS;
}

sys_error_t Serv_httpServer::start()
{
    std::stringstream ss;
    ss << "Starting server on port: " << _config.server_port;
    logger().log(ILog::LogLevel::INFO, ss.str());

    _config.stack_size = 8192; // Set stack size for the server task

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

        if (_websocketStartCb != nullptr)
        {
            _websocketStartCb(_server);
        }
        else 
        {
            logger().log(ILog::LogLevel::WARNING, "Websocket start callback is not set");
        }
    }
    else
    {
        logger().log(ILog::LogLevel::ERROR, "Starting HTTP server failed!");
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t Serv_httpServer::stop()
{
    httpd_stop(_server);
    if (_websocketStopCb != nullptr)
    {
        _websocketStopCb();
    }
    setStatus(Status::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Serv_httpServer::restart()
{
    stop();
    start();
    return ERROR_SUCCESS;
}

sys_error_t Serv_httpServer::registerUri(const httpd_uri_t* uri)
{
    if (httpd_register_uri_handler(_server, uri) != ESP_OK)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to register URI handler");
        return ERROR_FAIL;
    }

    logger().log(ILog::LogLevel::INFO, "URI handler registered successfully");

    return ERROR_SUCCESS;
}

sys_error_t Serv_httpServer::unregisterUri(const httpd_uri_t* uri)
{
    if (httpd_unregister_uri_handler(_server, uri->uri, uri->method) != ESP_OK)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to unregister URI handler");
        return ERROR_FAIL;
    }

    logger().log(ILog::LogLevel::INFO, "URI handler unregistered successfully");
    return ERROR_SUCCESS;
}

void Serv_httpServer::registerWebsocketCbs(std::function<void(httpd_handle_t _server)> startCb, std::function<void()> stopCb)
{
    _websocketStartCb = startCb;
    _websocketStopCb  = stopCb;
}
