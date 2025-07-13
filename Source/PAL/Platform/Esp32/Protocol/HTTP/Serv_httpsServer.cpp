/**
 * @file Serv_httpsServer.cpp
 * @brief Source file for Serv_httpsServer
 *
 * This file contains definitions for the Serv_httpsServer class and related data types and functions.
 */

#include "Serv_httpsServer.hpp"
#include "System/LogHandler.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"
#include "string.h"
#include <cstdlib>
#include <esp_log.h>
#include <sstream>

/**
 * @brief Extracts and logs certificate information from client connections
 *
 * This function retrieves the X.509 certificate from the client's SSL context,
 * formats the certificate information, and logs it to the application logger.
 * The certificate information includes subject, issuer, validity dates, and
 * other certificate fields.
 *
 * @param ssl Pointer to the mbedTLS SSL context containing the peer certificate
 *
 * @note If the peer did not provide a certificate or memory allocation fails,
 *       appropriate warning messages will be logged.
 */
static void print_peer_cert_info(const mbedtls_ssl_context* ssl);

/**
 * @brief HTTPS server lifecycle event callback
 *
 * Handles HTTPS server session events including:
 * - Session creation: Logs connection details (socket FD) and SSL cipher information
 * - Session closure: Logs peer certificate information using print_peer_cert_info()
 *
 * This callback provides visibility into the SSL/TLS handshake process and can be
 * used for connection monitoring, debugging, or implementing security policies.
 *
 * @param user_cb Callback structure containing event type and connection details
 *                - user_cb->user_cb_state: Event type (creation/closure)
 *                - user_cb->tls: TLS connection handle
 *
 * @warning This function must be registered with the HTTPS server configuration
 *          before starting the server using httpd_ssl_start()
 */
static void https_server_user_callback(esp_https_server_user_cb_arg_t* user_cb);

Serv_httpsServer::Serv_httpsServer(const uint8_t* serverCert, const size_t serverCertLen, const uint8_t* privateKey, const size_t privateKeyLen)
    : _server(NULL)                          // Initialize the server handle
    , _sslConfig(HTTPD_SSL_CONFIG_DEFAULT()) // Initialize the server and configuration
    , _uriList()                             // Initialize the URI list
    , _websocketStartCb(nullptr)             // Initialize the WebSocket start callback
    , _websocketStopCb(nullptr)              // Initialize the WebSocket stop callback
    , _serverCert(serverCert)                // Set the server certificate
    , _serverCertLen(serverCertLen)          // Set the server certificate length
    , _privateKey(privateKey)                // Set the private key
    , _privateKeyLen(privateKeyLen)          // Set the private key length
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
    SYS_LOG_I(ss.str());

    _sslConfig.httpd.stack_size = 8192; // Set stack size for the server task

    // Check if server certificate and private key are provided
    if (_serverCert == nullptr || _privateKey == nullptr)
    {
        SYS_LOG_E("Server certificate or private key is not set");
        return ERROR_FAIL;
    }

    // Set the server certificate and private key
    _sslConfig.servercert     = _serverCert;
    _sslConfig.servercert_len = _serverCertLen;
    _sslConfig.prvtkey_pem    = _privateKey;
    _sslConfig.prvtkey_len    = _privateKeyLen;

    // Set the user callback for HTTPS server
    _sslConfig.user_cb = https_server_user_callback;

    if (httpd_ssl_start(&_server, &_sslConfig) == ESP_OK)
    {
        SYS_LOG_I("HTTP server started successfully");
        setStatus(Status::STARTED);

        for (auto uri : _uriList)
        {
            if (httpd_register_uri_handler(_server, uri) != ESP_OK)
            {
                SYS_LOG_E("Failed to register URI handler");
                return ERROR_FAIL;
            }
        }

        if (_websocketStartCb != nullptr)
        {
            _websocketStartCb(_server);
        }
        else
        {
            SYS_LOG_W("Websocket start callback is not set");
        }
    }
    else
    {
        SYS_LOG_E("Starting HTTP server failed!");
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t Serv_httpsServer::stop()
{
    if (_server == NULL)
    {
        SYS_LOG_W("Server already stopped or not started");
        return ERROR_SUCCESS;
    }

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
    sys_error_t err = stop();
    if (err != ERROR_SUCCESS)
    {
        SYS_LOG_E("Failed to stop server during restart");
        return err;
    }

    return start();
}

sys_error_t Serv_httpsServer::registerUri(const httpd_uri_t* uri)
{
    if (httpd_register_uri_handler(_server, uri) != ESP_OK)
    {
        SYS_LOG_E("Failed to register URI handler");
        return ERROR_FAIL;
    }

    SYS_LOG_I("URI handler registered successfully");

    return ERROR_SUCCESS;
}

sys_error_t Serv_httpsServer::unregisterUri(const httpd_uri_t* uri)
{
    if (httpd_unregister_uri_handler(_server, uri->uri, uri->method) != ESP_OK)
    {
        SYS_LOG_E("Failed to unregister URI handler");
        return ERROR_FAIL;
    }

    SYS_LOG_I("URI handler unregistered successfully");
    return ERROR_SUCCESS;
}

void Serv_httpsServer::registerWebsocketCbs(std::function<void(httpd_handle_t _server)> startCb, std::function<void()> stopCb)
{
    _websocketStartCb = startCb;
    _websocketStopCb  = stopCb;
}

static void print_peer_cert_info(const mbedtls_ssl_context* ssl)
{

    const mbedtls_x509_crt* cert = mbedtls_ssl_get_peer_cert(ssl);
    if (cert == NULL)
    {
        SYS_LOG_W("Could not obtain the peer certificate!");
        return;
    }

    const size_t            buf_size = 1024;
    std::unique_ptr<char[]> buf(new (std::nothrow) char[buf_size]());
    if (!buf)
    {
        SYS_LOG_E("Out of memory - Callback execution failed!");
        return;
    }

    mbedtls_x509_crt_info(buf.get(), buf_size - 1, "    ", cert);
    SYS_LOG_I("Peer certificate info:");
    SYS_LOG_I(buf.get());
}

static void https_server_user_callback(esp_https_server_user_cb_arg_t* user_cb)
{
    SYS_LOG_I("User callback invoked!");

    mbedtls_ssl_context* ssl_ctx = NULL;

    switch (user_cb->user_cb_state)
    {
        case HTTPD_SSL_USER_CB_SESS_CREATE:
        {
            std::cout << "At session creation" << std::endl;

            // Logging the socket FD
            int       sockfd = -1;
            esp_err_t esp_ret;
            esp_ret = esp_tls_get_conn_sockfd(user_cb->tls, &sockfd);
            if (esp_ret != ESP_OK)
            {
                SYS_LOG_E("Error in obtaining the sockfd from tls context");
                break;
            }
            SYS_LOG_I("Socket FD: " + std::to_string(sockfd));

            ssl_ctx = (mbedtls_ssl_context*)esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL)
            {
                SYS_LOG_E("Error in obtaining ssl context");
                break;
            }
            // Logging the current ciphersuite
            SYS_LOG_I("Current Ciphersuite: " + std::string(mbedtls_ssl_get_ciphersuite(ssl_ctx)));
        }
        break;

        case HTTPD_SSL_USER_CB_SESS_CLOSE:
        {
            std::cout << "At session close" << std::endl;

            // Logging the peer certificate
            ssl_ctx = (mbedtls_ssl_context*)esp_tls_get_ssl_context(user_cb->tls);
            if (ssl_ctx == NULL)
            {
                SYS_LOG_E("Error in obtaining ssl context");
                break;
            }
            print_peer_cert_info(ssl_ctx);
        }
        break;
        default:
        {
            SYS_LOG_E("Illegal state!");
            return;
        }
    }
}