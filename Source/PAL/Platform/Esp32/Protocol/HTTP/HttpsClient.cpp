#include "HttpsClient.hpp"

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
    // Return success if client is already initialized
    RETURN_IF_ERROR((_client != nullptr), ERROR_SUCCESS);

    _options = options;

    esp_http_client_config_t config = {};

    _populate_config(_options, config);

    _client = esp_http_client_init(&config);
    RETURN_IF_ERROR((_client == nullptr), ERROR_FAIL, SYS_LOG_D("Failed to initialize HTTP client!"));

    _connected = true;
    return ERROR_SUCCESS;
}

sys_error_t HttpsClient::disconnect()
{
    if (_client)
    {
        esp_http_client_cleanup(_client);
        _client = nullptr;
    }
    _connected = false;
    _active_headers.clear();
    return ERROR_SUCCESS;
}

bool HttpsClient::isConnected() const
{
    return _connected;
}

sys_error_t HttpsClient::sendRequest(IHttpUri::HttpMethod           method,
                                     const std::string&             path,
                                     const std::vector<HttpHeader>& headers,
                                     const uint8_t*                 body,
                                     size_t                         body_len,
                                     int&                           out_status_code,
                                     HttpResponseStreamCb_t         on_data_received)
{

    RETURN_IF_ERROR((_client == nullptr), ERROR_NOT_INITIALIZED, SYS_LOG_D("HTTPS Client not initialized!"));

    _active_stream_cb = &on_data_received;
    _active_response_buf = nullptr;

    RETURN_ON_ERROR(
        _prepare_request(method, path, headers, body, body_len),    // Expression
        SYS_LOG_E("Failed to prepare request!");                    // Error Message
        _active_stream_cb = nullptr;                                // Clean up callback
    );

    esp_err_t esp_err = esp_http_client_perform(_client);

    for (const auto& key : _active_headers)
    {
        esp_http_client_delete_header(_client, key.c_str());
    }
    _active_headers.clear();

    _active_stream_cb = nullptr;

    RETURN_ON_ERROR(
        (esp_err != ESP_OK),                        // Expression
        ERROR_FAIL,                                 // Return Error Code
        SYS_LOG_E("Failed to perform request!")     // Log Message
    );

    out_status_code = esp_http_client_get_status_code(_client);
    on_data_received(nullptr, 0, true);

    return ERROR_SUCCESS;
}

sys_error_t HttpsClient::sendRequest(IHttpUri::HttpMethod           method,
                                     const std::string&             path,
                                     const std::vector<HttpHeader>& headers,
                                     const uint8_t*                 body,
                                     size_t                         body_len,
                                     int&                           out_status_code,
                                     std::vector<uint8_t>&          out_response_body)
{
    RETURN_IF_ERROR((_client == nullptr), ERROR_NOT_INITIALIZED, SYS_LOG_D("HTTPS Client not initialized!"));

    out_response_body.clear();
    _active_stream_cb = nullptr;
    _active_response_buf = &out_response_body;

    RETURN_ON_ERROR(
        _prepare_request(method, path, headers, body, body_len),        // Expression
        SYS_LOG_E("Failed to prepare request!");                        // Log Message
        _active_response_buf = nullptr;                                 // Clean up
    );

    esp_err_t esp_err = esp_http_client_perform(_client);

    for (const auto& key : _active_headers)
    {
        esp_http_client_delete_header(_client, key.c_str());
    }
    _active_headers.clear();

    _active_response_buf = nullptr;


    RETURN_IF_ERROR(
        (esp_err != ESP_OK),                        // Expression
        ERROR_FAIL,                                 // Return Error Code
        SYS_LOG_E("Failed to perform request!");     // Log Message
        out_response_body.clear();                  // Clean up
    );

    esp_err_t esp_err = esp_http_client_perform(_client);

    for (const auto& key : _active_headers)
    {
        esp_http_client_delete_header(_client, key.c_str());
    }
    _active_headers.clear();

    _active_response_buf = nullptr;

    if (esp_err != ESP_OK)
    {
        out_response_body.clear();
        return (_transaction_error != ERROR_SUCCESS) ? _transaction_error : ERROR_FAIL;
    }

    out_status_code = esp_http_client_get_status_code(_client);

    return ERROR_SUCCESS;
}

sys_error_t HttpsClient::_prepare_request(IHttpUri::HttpMethod           method,
                                          const std::string&             path,
                                          const std::vector<HttpHeader>& headers,
                                          const uint8_t*                 body,
                                          size_t                         body_len)
{
    esp_http_client_set_method(_client, HTTP_COMMON::_to_esp_method(method));
    esp_http_client_set_path(_client, path.c_str());

    if (body != nullptr && body_len > 0)
    {
        esp_http_client_set_post_field(_client, reinterpret_cast<const char*>(body), static_cast<int>(body_len));
    }
    else
    {
        esp_http_client_set_post_field(_client, nullptr, 0);
    }

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
            _transaction_error = ERROR_FAIL; // Map generic ESP-IDF error to ERROR_FAIL
            break;    
        case HTTP_EVENT_ON_CONNECTED:    
            _connected = true;
            break;      
        case HTTP_EVENT_HEADER_SENT:    
            // All headers have been sent successfully
            break;    
        case HTTP_EVENT_ON_HEADER_RECEIVED:    
            // HTTP headers have been received
            break;    
        case HTTP_EVENT_ON_DATA:    
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
                    try
                    {
                        _active_response_buf->insert(_active_response_buf->end(),
                                                     reinterpret_cast<const uint8_t*>(evt->data),
                                                     reinterpret_cast<const uint8_t*>(evt->data) + evt->data_len);
                    }
                    catch (...)
                    {
                        _transaction_error = ERROR_OUT_OF_MEMORY;
                        return ESP_FAIL;
                    }
                }
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            if (_active_stream_cb != nullptr)
            {
                (*_active_stream_cb)(nullptr, 0, true);
            }
            break;

        case HTTP_EVENT_ERROR:
            _transaction_error = ERROR_FAIL; // Map generic ESP-IDF error to ERROR_FAIL
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
        case IHttpUri::HttpMethod::DELETE_:
            return HTTP_METHOD_DELETE;
        case IHttpUri::HttpMethod::PATCH:
            return HTTP_METHOD_PATCH;
        default:
            return HTTP_METHOD_GET;
    }
}

void HttpsClient::_populate_config(const HttpClientOptions_t& options, esp_http_client_config_t& config)
{
    config.url               = options.uri.c_str();
    config.transport_type    = HTTP_TRANSPORT_OVER_SSL;
    config.event_handler     = _http_event_handler;
    config.user_data         = this;
    config.skip_cert_chain_validation = true;
    config.timeout_ms        = options.timeout_ms;
}