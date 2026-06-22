#pragma once

#include "System/system.h"
#include "IHttpUri.hpp"
#include <string>
#include <vector>
#include <functional>

/// Key-Value metadata structure for HTTP Headers
struct HttpHeader
{
    std::string key;
    std::string value;
};

/// Connection configurations for the HTTP Client
struct HttpClientOptions_t
{
    std::string host;
    uint16_t    port            = 80;
    bool        use_tls         = false;
    const char* server_cert_pem = nullptr; // CA Certificate for TLS validation
    size_t      server_cert_len = 0;
    const char* client_cert_pem = nullptr; // Client certificate for mTLS
    size_t      client_cert_len = 0;
    const char* client_key_pem  = nullptr; // Client private key for mTLS
    size_t      client_key_len  = 0;
    uint32_t    timeout_ms      = 5000;
    bool        keep_alive      = false;
};

/**
 * @brief Callback signature for streaming HTTP response data.
 * 
 * @param chunk Pointer to the data chunk received from the TCP stream.
 * @param len Size of the incoming chunk.
 * @param is_last Flag indicating if this is the final block of the response body.
 * @return sys_error_t Return ERROR_SUCCESS, or return an error code to abort the connection.
 */
using HttpResponseStreamCb_t = std::function<sys_error_t(const uint8_t* chunk, size_t len, bool is_last)>;

class IHttpClient
{
public:
    virtual ~IHttpClient() = default;

    /**
     * @brief Configure and connect to the remote host.
     */
    virtual sys_error_t connect(const HttpClientOptions_t& options) = 0;

    /**
     * @brief Close the connection.
     */
    virtual sys_error_t disconnect() = 0;

    /**
     * @brief Retrieve the connection status.
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Sends an HTTP request and streams the response body via a callback.
     * Use this method for OTA updates or large REST payloads.
     *
     * @param method HTTP method (GET, POST, PUT, etc.) from IHttpUri::HttpMethod
     * @param path Target request path
     * @param headers Collection of custom headers to append
     * @param body Pointer to optional request body buffer
     * @param body_len Length of the request body
     * @param out_status_code Output variable to capture the HTTP response status code
     * @param on_data_received Callback fired as chunks are received from the network
     */
    virtual sys_error_t sendRequest(IHttpUri::HttpMethod           method,
                                    const std::string&             path,
                                    const std::vector<HttpHeader>& headers,
                                    const uint8_t*                 body,
                                    size_t                         body_len,
                                    int&                           out_status_code,
                                    HttpResponseStreamCb_t         on_data_received) = 0;

    /**
     * @brief Buffered overload. Used when the response is guaranteed to fit within memory limits.
     *
     * @param method HTTP method
     * @param path Target request path
     * @param headers Collection of custom headers
     * @param body Pointer to optional request body buffer
     * @param body_len Length of the request body
     * @param out_status_code Output variable to capture the HTTP response status code
     * @param out_response_body Vector populated with the entire response payload
     */
    virtual sys_error_t sendRequest(IHttpUri::HttpMethod           method,
                                    const std::string&             path,
                                    const std::vector<HttpHeader>& headers,
                                    const uint8_t*                 body,
                                    size_t                         body_len,
                                    int&                           out_status_code,
                                    std::vector<uint8_t>&          out_response_body) = 0;

    /**
     * @brief Exposes the platform-specific lower level handle (e.g., esp_http_client_handle_t)
     */
    virtual void* nativeHandle() const 
    {
        return nullptr;
    }
};