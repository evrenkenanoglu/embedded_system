#include "HttpsServer.hpp"
#include "Platform/Esp32/Protocol/HTTP/URIs/HttpUri.hpp" // platform IHttpUri implementation
#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include <cstring>

HttpsServer::HttpsServer() noexcept
    : _server(nullptr)
    , _sslConfig(HTTPD_SSL_CONFIG_DEFAULT())

{
}

HttpsServer::~HttpsServer()
{
    stop();
}

sys_error_t HttpsServer::populate_configs(const HttpServerStartOptions_t* options, httpd_ssl_config_t& sslConfig)
{

    // Validate options
    // TLS enabled but cert or key is null
    RETURN_IF_ERROR(
        options != nullptr && options->use_tls && (options->tls_cert_pem == nullptr || options->tls_key_pem == nullptr),
        ERROR_INVALID_ARG,
        "TLS is enabled but certificate or key is null");

    sslConfig.httpd = HTTPD_SSL_CONFIG_DEFAULT();

    // Populate SSL config based on provided options
    sslConfig.transport_mode         = options->use_tls ? HTTPD_SSL_TRANSPORT_SECURE : HTTPD_SSL_TRANSPORT_INSECURE;
    sslConfig.port_secure            = options->port;
    sslConfig.servercert             = reinterpret_cast<const uint8_t*>(options->tls_cert_pem);
    sslConfig.servercert_len         = options->tls_cert_pem ? std::strlen(options->tls_cert_pem) : 0;
    sslConfig.prvtkey_pem            = reinterpret_cast<const uint8_t*>(options->tls_key_pem);
    sslConfig.prvtkey_len            = options->tls_key_pem ? std::strlen(options->tls_key_pem) : 0;
    sslConfig.httpd.stack_size       = options->task_stack_size;
    sslConfig.httpd.task_priority    = options->task_priority;
    sslConfig.httpd.max_open_sockets = options->max_connections;

    // Set the user callback for HTTPS server
    sslConfig.user_cb = https_server_user_callback;

    return ERROR_SUCCESS;
}

sys_error_t HttpsServer::start(const HttpServerStartOptions_t* options) noexcept
{
    // Check if server is already started
    RETURN_IF_ERROR(_started != false, ERROR_ALREADY_INITIALIZED, SYS_LOG_E("Server already started!"));

    // Populate server and SSL configurations
    RETURN_ON_ERROR(populate_configs(options, _sslConfig), SYS_LOG_E("Failed to populate server configurations!"));

    // Start the HTTPS server
    RETURN_IF_ERROR(httpd_ssl_start(&_server, &_sslConfig) != ESP_OK, ERROR_FAIL, SYS_LOG_E("Failed to start HTTPS server!"));

    _started = true;

    SYS_LOG_I("HTTPS server started successfully!");

    // Register all previously added URIs
    for (auto uri : _registeredUris)
    {
        RETURN_ON_ERROR(httpd_register_uri_handler(_server, &uri.platformUri()) != ESP_OK, ERROR_FAIL, SYS_LOG_E("Failed to register URI handler!"));
        SYS_LOG_I("Registered URIs %s", uri.getPath());
    }

    // Register all previously added WebSocket URIs
    for (auto wsUri : _registeredWsUris)
    {
        RETURN_ON_ERROR(httpd_register_uri_handler(_server, &wsUri.platformWsUri()) != ESP_OK, ERROR_FAIL, SYS_LOG_E("Failed to register WebSocket URI handler!"));
        SYS_LOG_I("Registered WebSocket URIs %s", wsUri.getPath());
    }

    return ERROR_SUCCESS;
}

sys_error_t HttpsServer::stop() noexcept
{
    // Check if server is already stopped
    RETURN_IF_ERROR(_started == false, ERROR_NOT_INITIALIZED, SYS_LOG_W("Server already stopped or not started"));

    // Stop the HTTPS server
    RETURN_IF_ERROR(httpd_ssl_stop(_server) != ESP_OK, ERROR_FAIL, SYS_LOG_E("Failed to stop HTTPS server!"));

    _server  = nullptr;
    _started = false;

    SYS_LOG_I("HTTPS server stopped successfully!");

    return ERROR_SUCCESS;
}

sys_error_t HttpsServer::registerUri(IHttpUri& uri) noexcept
{
    // Check if server is started
    RETURN_IF_ERROR(_started == false, ERROR_NOT_INITIALIZED, SYS_LOG_E("Server not started!"));

    // Check if URI is already registered
    for (const auto& registeredUri : _registeredUris)
    {
        if (registeredUri == &uri)
        {
            return ERROR_ALREADY_INITIALIZED;
        }
    }

    // Register the URI handler with the server
    RETURN_IF_ERROR(httpd_register_uri_handler(_server, &static_cast<HttpUri&>(uri).platformUri(), ERROR_FAIL, SYS_LOG_E("Failed to register URI handler!")));

    // Add the URI to the list of registered URIs
    _registeredUris.push_back(static_cast<HttpUri*>(&uri));

    SYS_LOG_I("Registered URI %s", uri.getPath());

    return ERROR_SUCCESS;
}

sys_error_t HttpsServer::unregisterUri(IHttpUri& uri) noexcept
{
    // Check if server is started
    RETURN_IF_ERROR(_started == false, ERROR_NOT_INITIALIZED, SYS_LOG_E("Server not started!"));

    // Find and remove the URI from the list of registered URIs
    auto it = std::find(_registeredUris.begin(), _registeredUris.end(), static_cast<HttpUri*>(&uri));
    RETURN_IF_ERROR(it == _registeredUris.end(), ERROR_NOT_FOUND, SYS_LOG_E("URI not found!"));

    // Unregister the URI handler from the server
    RETURN_IF_ERROR(
        httpd_unregister_uri_handler(_server, uri.getPath(), to_httpd_method(uri.getMethod())) != ESP_OK, ERROR_FAIL, SYS_LOG_E("Failed to unregister URI handler!"));

    _registeredUris.erase(it);

    SYS_LOG_I("Unregistered URI %s", uri.getPath());

    return ERROR_SUCCESS;
}

sys_error_t HttpsServer::sendWsMessage(int clientId_sockfd, const uint8_t* data, std::size_t len, WsFrameType ws_type) noexcept
{
    RETURN_IF_ERROR((!_started || _server == NULL), ERROR_NOT_INITIALIZED, SYS_LOG_E("Server not started!"));

    // send to a specific client
    httpd_ws_frame_t ws_frame = {};
    ws_frame.payload          = data;
    ws_frame.len              = len;
    ws_frame.type             = ws_type;

    // print the message
    SYS_LOG_D("Sending message: " + std::string(reinterpret_cast<char*>(data), len) + ", type: " + std::to_string(ws_type) + ", len: " + std::to_string(len));

    if (httpd_ws_get_fd_info(_server, clientId_sockfd) == HTTPD_WS_CLIENT_WEBSOCKET)
    {
        error_t error = httpd_ws_send_frame_async(_server, clientId_sockfd, &ws_frame);
        SYS_LOG_I("Sending message to client fd: " + std::to_string(clientId_sockfd) + ", type: " + std::to_string(ws_type) + ", len: " + std::to_string(len));
        if (error != ESP_OK)
        {
            SYS_LOG_E("Failed to send message to client fd: " + std::to_string(clientId_sockfd));
            return ERROR_FAIL;
        }
    }
    else
    {
        SYS_LOG_E("Client fd: " + std::to_string(clientId_sockfd) + " is not a websocket client");
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}
sys_error_t HttpsServer::broadcastWs(const uint8_t* payload, std::size_t len, WsFrameType ws_type) noexcept
{
    RETURN_IF_ERROR((!_started || _server == NULL), ERROR_NOT_INITIALIZED, SYS_LOG_E("Server not started!"));

    static size_t maxClients = CONFIG_LWIP_MAX_LISTENING_TCP;
    size_t        fds        = maxClients;
    int           client_fds[maxClients];

    error_t error = ESP_OK;
    error         = httpd_get_client_list(_server, &fds, client_fds);

    RETURN_IF_ERROR(error != ESP_OK, ERROR_FAIL, SYS_LOG_E("Failed to get client list!"));

    for (uint8_t i = 0; i < fds; i++)
    {
        if (httpd_ws_get_fd_info(_server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET)
        {
            sendWsMessage(client_fds[i], payload, len, ws_type);
        }
    }

    return ERROR_SUCCESS;
}

// Example of sending (the server needs a way to find the FD for a client)
sys_error_t HttpsServer::sendWsMessage(int clientId_sockfd, const uint8_t* data, std::size_t len, WsFrameType ws_type) noexcept {}

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

void HttpsServer::https_server_user_callback(esp_https_server_user_cb_arg_t* user_cb)
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