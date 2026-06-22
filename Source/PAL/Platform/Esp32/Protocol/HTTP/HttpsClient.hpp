#pragma once

#include "PAL/Protocols/HTTP/IHttpClient.hpp"
#include <esp_http_client.h>
#include <string>
#include <vector>

class HttpsClient : public IHttpClient
{
public:
    HttpsClient();
    ~HttpsClient() override;

    sys_error_t connect(const HttpClientOptions_t& options) override;
    sys_error_t disconnect() override;
    bool        isConnected() const override;

    sys_error_t sendRequest(IHttpUri::HttpMethod           method,
                            const std::string&             path,
                            const std::vector<HttpHeader>& headers,
                            const uint8_t*                 body,
                            size_t                         body_len,
                            int&                           out_status_code,
                            HttpResponseStreamCb_t         on_data_received) override;

    sys_error_t sendRequest(IHttpUri::HttpMethod           method,
                            const std::string&             path,
                            const std::vector<HttpHeader>& headers,
                            const uint8_t*                 body,
                            size_t                         body_len,
                            int&                           out_status_code,
                            std::vector<uint8_t>&          out_response_body) override;

    void* nativeHandle() const override
    {
        return reinterpret_cast<void*>(_client);
    }

private:
    esp_http_client_handle_t _client;
    bool                     _connected;
    HttpClientOptions_t      _options;
    std::vector<std::string> _active_headers;

    // Transaction parameters to communicate with the static ESP-IDF callback
    HttpResponseStreamCb_t*  _active_stream_cb;
    std::vector<uint8_t>*    _active_response_buf;
    sys_error_t              _transaction_error;

    /// Event Handler
    static esp_err_t _http_event_handler(esp_http_client_event_t* evt);

    esp_err_t        _handle_event(esp_http_client_event_t* evt);
    
    /// Request Preparation Members
    sys_error_t _prepare_request(IHttpUri::HttpMethod           method, 
                                 const std::string&             path, 
                                 const std::vector<HttpHeader>& headers, 
                                 const uint8_t*                 body, 
                                 size_t                         body_len);

    void _populate_config(const HttpClientOptions_t& options, esp_http_client_config_t& config);
    esp_http_client_method_t _to_esp_method(IHttpUri::HttpMethod method);
    
};