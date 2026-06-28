#include "HttpsClient.hpp"
#include <cstring>

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"

HttpsClient::HttpsClient()
    : _client(nullptr)
    , _connected(false)
    , _options()
    , _active_headers()
    , _active_stream_cb(nullptr)
    , _active_response_buf(nullptr)
    , _transaction_error(ERROR_SUCCESS)
{
}

HttpsClient::~HttpsClient()
{
    disconnect();
}

sys_error_t HttpsClient::connect(const HttpClientOptions_t& options)
{
    /// Pre-Initialization Check
    /// If the native client is already active, return success immediately to prevent re-allocation.
    RETURN_IF_ERROR(
        (_client != nullptr), // Expression
        ERROR_SUCCESS         // Error code
    );

    _options = options;

    esp_http_client_config_t config = {};

    /// Configuration Mapping
    /// Convert class options to raw ESP-IDF configuration parameters.
    _populate_config(_options, config);

    /// Native Handle Allocation
    /// Initialize the underlying SDK HTTP client instance.
    _client = esp_http_client_init(&config);
    RETURN_IF_ERROR(
        (_client == nullptr),                          // Expression
        ERROR_FAIL,                                    // Error code
        SYS_LOG_D("Failed to initialize HTTP client!") // Error message
    );

    _connected = true;
    return ERROR_SUCCESS;
}

sys_error_t HttpsClient::disconnect()
{
    /// Native Handle Cleanup
    /// Safely release raw ESP-IDF client resources if allocated.
    if (_client)
    {
        esp_http_client_cleanup(_client);
        _client = nullptr;
    }

    /// State Reset
    /// Reset active connection flags and header memory contexts.
    _connected = false;
    _active_headers.clear();
    return ERROR_SUCCESS;
}

bool HttpsClient::isConnected() const
{
    return _connected;
}

sys_error_t HttpsClient::sendRequest(
    IHttpUri::HttpMethod           method,          //
    const std::string&             path,            //
    const std::vector<HttpHeader>& headers,         //
    const uint8_t*                 body,            //
    size_t                         body_len,        //
    int&                           out_status_code, //
    HttpResponseStreamCb_t         on_data_received //
)
{
    /// Initialization Verification
    RETURN_IF_ERROR(
        (_client == nullptr),                      // Expression
        ERROR_NOT_INITIALIZED,                     // Error code
        SYS_LOG_D("HTTPS Client not initialized!") // Error message
    );

    /// Context Binding
    /// Map active callbacks and reset transaction status parameters.
    _transaction_error   = ERROR_SUCCESS;
    _active_stream_cb    = &on_data_received;
    _active_response_buf = nullptr;

    /// Payload Preparation
    RETURN_ON_ERROR(_prepare_request(method, path, headers, body, body_len), // Expression
                    SYS_LOG_E("Failed to prepare request!");                 // Error Message
                    _active_stream_cb = nullptr;                             // Cleanup
    );

    /// Synchronous Execution
    /// Execute the HTTP transaction over network interface.
    const esp_err_t esp_err = esp_http_client_perform(_client);

    /// Clean Up Shared Memory
    /// Delete active headers and unbind dynamic stream pointers.
    for (const auto& key : _active_headers)
    {
        esp_http_client_delete_header(_client, key.c_str());
    }
    _active_headers.clear();
    _active_stream_cb = nullptr;

    /// Status Code Validation
    RETURN_IF_ERROR(
        (esp_err != ESP_OK),                                                     // Expression
        (_transaction_error != ERROR_SUCCESS) ? _transaction_error : ERROR_FAIL, // Error code
        SYS_LOG_E("Failed to perform request!")                                  // Error message
    );

    out_status_code = esp_http_client_get_status_code(_client);

    return ERROR_SUCCESS;
}

sys_error_t HttpsClient::sendRequest(
    IHttpUri::HttpMethod           method,           //
    const std::string&             path,             //
    const std::vector<HttpHeader>& headers,          //
    const uint8_t*                 body,             //
    size_t                         body_len,         //
    int&                           out_status_code,  //
    std::vector<uint8_t>&          out_response_body //
)
{
    /// Initialization Verification
    RETURN_IF_ERROR(
        (_client == nullptr),                      // Expression
        ERROR_NOT_INITIALIZED,                     // Error code
        SYS_LOG_D("HTTPS Client not initialized!") // Error message
    );

    /// Context Binding
    /// Map target buffers and reset transaction status parameters.
    _transaction_error = ERROR_SUCCESS;
    out_response_body.clear();
    _active_stream_cb    = nullptr;
    _active_response_buf = &out_response_body;

    /// Payload Preparation
    RETURN_ON_ERROR(_prepare_request(method, path, headers, body, body_len), // Expression
                    SYS_LOG_E("Failed to prepare request!");                 // Error Message
                    _active_response_buf = nullptr;                          // Cleanup
    );

    /// Synchronous Execution
    /// Execute the HTTP transaction over network interface.
    const esp_err_t esp_err = esp_http_client_perform(_client);

    /// Clean Up Shared Memory
    /// Delete active headers and unbind response buffer pointers.
    for (const auto& key : _active_headers)
    {
        esp_http_client_delete_header(_client, key.c_str());
    }
    _active_headers.clear();
    _active_response_buf = nullptr;

    /// Status Code Validation
    if (esp_err != ESP_OK)
    {
        out_response_body.clear();
        return (_transaction_error != ERROR_SUCCESS) ? _transaction_error : ERROR_FAIL;
    }

    out_status_code = esp_http_client_get_status_code(_client);

    return ERROR_SUCCESS;
}

sys_error_t HttpsClient::_prepare_request(
    IHttpUri::HttpMethod           method,  //
    const std::string&             path,    //
    const std::vector<HttpHeader>& headers, //
    const uint8_t*                 body,    //
    size_t                         body_len //
)
{
    /// Target Endpoint Configuration
    /// Dynamically update request path using standard URL utility.
    esp_http_client_set_method(_client, _to_esp_method(method));
    esp_http_client_set_url(_client, path.c_str());

    /// Payload Body Insertion
    if (body != nullptr && body_len > 0)
    {
        esp_http_client_set_post_field(_client, reinterpret_cast<const char*>(body), static_cast<int>(body_len));
    }
    else
    {
        esp_http_client_set_post_field(_client, nullptr, 0);
    }

    /// Custom Header Registration
    for (const auto& header : headers)
    {
        esp_http_client_set_header(_client, header.key.c_str(), header.value.c_str());
        _active_headers.push_back(header.key);
    }

    return ERROR_SUCCESS;
}

esp_err_t HttpsClient::_http_event_handler(esp_http_client_event_t* evt)
{
    auto* self = static_cast<HttpsClient*>(evt->user_data);
    if (self != nullptr)
    {
        return self->_handle_event(evt);
    }
    return ESP_FAIL;
}

esp_err_t HttpsClient::_handle_event(esp_http_client_event_t* evt)
{
    switch (evt->event_id)
    {
        case HTTP_EVENT_ERROR:
        {
            _transaction_error = ERROR_FAIL; // Map generic ESP-IDF error to ERROR_FAIL
        }
        break;

        case HTTP_EVENT_ON_CONNECTED:
        {
            _connected = true;
        }
        break;

        case HTTP_EVENT_DISCONNECTED:
        {
            _connected = false;
        }
        break;

        case HTTP_EVENT_HEADER_SENT:
        { // All headers have been sent successfully
        }
        break;

        case HTTP_EVENT_ON_DATA:
        {
            /// Process chunked socket streams by prioritizing stream callbacks or appending to buffers.
            if (evt->data_len > 0)
            {
                if (_active_stream_cb != nullptr)
                {
                    sys_error_t err = (*_active_stream_cb)(reinterpret_cast<const uint8_t*>(evt->data), evt->data_len, false);
                    if (err != ERROR_SUCCESS)
                    {
                        _transaction_error = err;
                        return ESP_FAIL; // Stop execution
                    }
                }
                else if (_active_response_buf != nullptr)
                {
                    /// Exception handling is disabled. Memory allocation failure in std::vector 
                    /// will trigger standard system termination handlers automatically.
                    _active_response_buf->insert(
                        _active_response_buf->end(), 
                        reinterpret_cast<const uint8_t*>(evt->data), 
                        reinterpret_cast<const uint8_t*>(evt->data) + evt->data_len
                    );
                }
            }
        }
        break;

        case HTTP_EVENT_ON_FINISH:
        {
            /// Flush stream contexts on transaction completion events.
            if (_active_stream_cb != nullptr)
            {
                (*_active_stream_cb)(nullptr, 0, true);
            }
        }
        break;

        default:
            break;
    }

    return ESP_OK;
}

esp_http_client_method_t HttpsClient::_to_esp_method(IHttpUri::HttpMethod method)
{
    switch (method)
    {
        case IHttpUri::HttpMethod::GET:
            return HTTP_METHOD_GET;
        case IHttpUri::HttpMethod::POST:
            return HTTP_METHOD_POST;
        case IHttpUri::HttpMethod::PUT:
            return HTTP_METHOD_PUT;
        case IHttpUri::HttpMethod::DELETE:
            return HTTP_METHOD_DELETE;
        case IHttpUri::HttpMethod::PATCH:
            return HTTP_METHOD_PATCH;
        default:
            return HTTP_METHOD_GET;
    }
}

void HttpsClient::_populate_config(const HttpClientOptions_t& options, esp_http_client_config_t& config)
{
    // Zero-initialize the structure to ensure all unmapped fields default to safe values
    std::memset(&config, 0, sizeof(esp_http_client_config_t));

    // Populate standard network parameters
    config.host              = options.host.c_str();
    config.port              = options.port;
    config.transport_type    = options.use_tls ? HTTP_TRANSPORT_OVER_SSL : HTTP_TRANSPORT_OVER_TCP;
    config.timeout_ms        = options.timeout_ms;
    config.keep_alive_enable = options.keep_alive;

    // Populate transaction callbacks
    config.event_handler = &HttpsClient::_http_event_handler;
    config.user_data     = this;

    // Populate TLS configurations if enabled
    if (options.use_tls)
    {
        config.cert_pem                    = options.server_cert_pem;
        config.cert_len                    = options.server_cert_len;
        config.client_cert_pem             = options.client_cert_pem;
        config.client_cert_len             = options.client_cert_len;
        config.client_key_pem              = options.client_key_pem;
        config.client_key_len              = options.client_key_len;
        config.skip_cert_common_name_check = true;
    }
}