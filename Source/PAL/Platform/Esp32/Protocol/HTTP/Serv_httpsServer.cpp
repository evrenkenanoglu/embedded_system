/**
 * @file Serv_httpsServer.cpp
 * @brief Source file for Serv_httpsServer
 *
 * This file contains definitions for the Serv_httpsServer class and related data types and functions.
 */

#include "Serv_httpsServer.hpp"
#include "HAL/Platform/ESP32/library/logImpl.h"
#include "string.h"
#include <esp_log.h>
#include <sstream>

Serv_httpsServer::Serv_httpsServer(const uint8_t* serverCert, const uint8_t* privateKey)
    : _server(NULL)                          // Initialize the server handle
    , _sslConfig(HTTPD_SSL_CONFIG_DEFAULT()) // Initialize the server and configuration
    , _uriList()                             // Initialize the URI list
    , _websocketStartCb(nullptr)             // Initialize the WebSocket start callback
    , _websocketStopCb(nullptr)              // Initialize the WebSocket stop callback
    , _serverCert(serverCert)                // Set the server certificate
    , _privateKey(privateKey)                // Set the private key
{
}

Serv_httpsServer::~Serv_httpsServer()
{
    // destructor implementation
}

sys_error_t Serv_httpsServer::init()
{
    setStatus(Status::INITIALIZED);
    return ERROR_SUCCESS;
}

sys_error_t Serv_httpsServer::start()
{
    std::stringstream ss;
    ss << "Starting server on port: " << _sslConfig.port_secure;
    logger().log(ILog::LogLevel::INFO, ss.str());

    _sslConfig.httpd.stack_size = 8192; // Set stack size for the server task

    // Check if server certificate and private key are provided
    if (_serverCert == nullptr || _privateKey == nullptr)
    {
        logger().log(ILog::LogLevel::ERROR, "Server certificate or private key is not set");
        return ERROR_FAIL;
    }

    // Set the server certificate and private key
    _sslConfig.servercert     = _serverCert;
    _sslConfig.servercert_len = strlen(reinterpret_cast<const char*>(_serverCert));
    _sslConfig.prvtkey_pem    = _privateKey;
    _sslConfig.prvtkey_len    = strlen(reinterpret_cast<const char*>(_privateKey));

    // Set the user callback if needed
#if CONFIG_EXAMPLE_ENABLE_HTTPS_USER_CALLBACK
    _sslConfig.user_cb = https_server_user_callback; // Set the user callback for HTTPS server
#else
    _sslConfig.user_cb = nullptr; // No user callback set
#endif

    if (httpd_ssl_start(&_server, &_sslConfig) == ESP_OK)
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

sys_error_t Serv_httpsServer::stop()
{
    httpd_ssl_stop(_server);
    if (_websocketStopCb != nullptr)
    {
        _websocketStopCb();
    }
    setStatus(Status::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Serv_httpsServer::restart()
{
    stop();
    start();
    return ERROR_SUCCESS;
}

sys_error_t Serv_httpsServer::registerUri(const httpd_uri_t* uri)
{
    if (httpd_register_uri_handler(_server, uri) != ESP_OK)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to register URI handler");
        return ERROR_FAIL;
    }

    logger().log(ILog::LogLevel::INFO, "URI handler registered successfully");

    return ERROR_SUCCESS;
}

sys_error_t Serv_httpsServer::unregisterUri(const httpd_uri_t* uri)
{
    if (httpd_unregister_uri_handler(_server, uri->uri, uri->method) != ESP_OK)
    {
        logger().log(ILog::LogLevel::ERROR, "Failed to unregister URI handler");
        return ERROR_FAIL;
    }

    logger().log(ILog::LogLevel::INFO, "URI handler unregistered successfully");
    return ERROR_SUCCESS;
}

void Serv_httpsServer::registerWebsocketCbs(std::function<void(httpd_handle_t _server)> startCb, std::function<void()> stopCb)
{
    _websocketStartCb = startCb;
    _websocketStopCb  = stopCb;
}
