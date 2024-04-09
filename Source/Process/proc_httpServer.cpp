/**
 * @file proc_httpServer.cpp
 * @brief Source file for proc_httpServer
 *
 * This file contains definitions for the proc_httpServer class and related data types and functions.
 */

#include "proc_httpServer.hpp"
#include "Library/UI/HTTP/ui_welcome_wifi_connect.h"
// #include "Library/UI/HTTP/output_test1.h"
#include "protocol_examples_utils.h"
#include <esp_log.h>
#include <sstream>
#include <stdlib.h>

static const char* TAG = "example";
#define MIN(x, y)                      ((x) < (y) ? (x) : (y))
#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

static esp_err_t welcome_get_handler(httpd_req_t* req);
static esp_err_t connect_post_handler(httpd_req_t* req);
static esp_err_t ctrl_put_handler(httpd_req_t* req);

static const httpd_uri_t welcome = {.uri     = "/welcome",
                                    .method  = HTTP_GET,
                                    .handler = welcome_get_handler,
                                    /* Let's pass response string in user
                                     * context to demonstrate it's usage */
                                    .user_ctx = (void*)(HTML_UI_WELCOME_WIFI_CONNECT_CONTENT)};

static const httpd_uri_t connect = {.uri = "/connect", .method = HTTP_POST, .handler = connect_post_handler, .user_ctx = NULL};

static const httpd_uri_t ctrl = {.uri = "/ctrl", .method = HTTP_PUT, .handler = ctrl_put_handler, .user_ctx = NULL};

proc_httpServer::proc_httpServer()
{
    _server                  = NULL;
    _config                  = HTTPD_DEFAULT_CONFIG();
    _config.lru_purge_enable = true;
    setState(IProcess::State::INITIALIZED);
}

proc_httpServer::~proc_httpServer()
{
    // destructor implementation
}

sys_error_t proc_httpServer::start()
{
    std::stringstream ss;
    ss << "Starting server on port: " << _config.server_port;
    std::cout << ss.str() << std::endl;

    if (httpd_start(&_server, &_config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(_server, &welcome);
        httpd_register_uri_handler(_server, &connect);
        httpd_register_uri_handler(_server, &ctrl);
        setState(IProcess::State::RUNNING);
    }
    else
    {
        std::cout << "Starting HTTP server failed!" << std::endl;
    }

    return ERROR_SUCCESS;
}

sys_error_t proc_httpServer::stop()
{
    // // Stop the httpd server
    // httpd_stop(_server);
    return ERROR_SUCCESS;
}

sys_error_t proc_httpServer::pause()
{
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t proc_httpServer::resume()
{
    return ERROR_NOT_IMPLEMENTED;
}

/* An HTTP GET handler */
static esp_err_t welcome_get_handler(httpd_req_t* req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1)
    {
        buf = (char*)(malloc(buf_len));
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1)
    {
        buf = (char*)(malloc(buf_len));
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1)
    {
        buf = (char*)(malloc(buf_len));
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char*)(malloc(buf_len));
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*)req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0)
    {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

/* An HTTP POST handler */
static esp_err_t connect_post_handler(httpd_req_t* req)
{
    char ssid[32], password[32];

    ESP_LOGI(TAG, "POST HANDLER TRIGGERED");
    char content[256];
    int  ret, remaining = req->content_len;
    while (remaining > 0)
    {
        if ((ret = httpd_req_recv(req, content, MIN(remaining, sizeof(content)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    content[req->content_len] = '\0'; // null-terminate the received data

    if (httpd_query_key_value(content, "ssid", ssid, sizeof(ssid)) == ESP_OK && httpd_query_key_value(content, "password", password, sizeof(password)) == ESP_OK)
    {
        // Do something with the received data, e.g. connect to the specified WiFi network
        ESP_LOGI(TAG, "Received SSID: %s, password: %s", ssid, password);
        // ...
        httpd_resp_send(req, "Connected", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    else
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    return ESP_OK;
}
// httpd_query_key_value(req->uri, "ssid", ssid, sizeof(ssid));
// httpd_query_key_value(req->uri, "password", password, sizeof(password));

// ESP_LOGI(TAG, "=========== RECEIVED DATA ENTER ==========");
// ESP_LOGI(TAG, "SSID     : %s", ssid);
// ESP_LOGI(TAG, "Password : %s", password);
// ESP_LOGI(TAG, "====================================");

// ESP_LOGI(TAG, "=========== RECEIVED DATA ENTER ==========");

// char buf[100];
// int  ret, remaining = req->content_len;

// while (remaining > 0)
// {
//     /* Read the data for the request */
//     if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0)
//     {
//         if (ret == HTTPD_SOCK_ERR_TIMEOUT)
//         {
//             /* Retry receiving if timeout occurred */
//             continue;
//         }
//         return ESP_FAIL;
//     }

//     /* Send back the same data */
//     httpd_resp_send_chunk(req, buf, ret);
//     remaining -= ret;

//     /* Log data received */
//     ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
//     ESP_LOGI(TAG, "%.*s", ret, buf);
//     ESP_LOGI(TAG, "====================================");
// }

// // End response
// httpd_resp_send_chunk(req, NULL, 0);

/* This handler allows the custom error handling functionality to be
 * tested from client side. For that, when a PUT request 0 is sent to
 * URI /ctrl, the /welcome and /connect URIs are unregistered and following
 * custom error handler http_404_error_handler() is registered.
 * Afterwards, when /welcome or /connect is requested, this custom error
 * handler is invoked which, after sending an error message to client,
 * either closes the underlying socket (when requested URI is /connect)
 * or keeps it open (when requested URI is /welcome). This allows the
 * client to infer if the custom error handler is functioning as expected
 * by observing the socket state.
 */
esp_err_t http_404_error_handler(httpd_req_t* req, httpd_err_code_t err)
{
    if (strcmp("/welcome", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/welcome URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    else if (strcmp("/connect", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/connect URI is not available");
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
static esp_err_t ctrl_put_handler(httpd_req_t* req)
{
    char buf;
    int  ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0')
    {
        /* URI handlers can be unregistered using the uri string */
        ESP_LOGI(TAG, "Unregistering /welcome and /connect URIs");
        httpd_unregister_uri(req->handle, "/welcome");
        httpd_unregister_uri(req->handle, "/connect");
        /* Register the custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else
    {
        ESP_LOGI(TAG, "Registering /welcome and /connect URIs");
        httpd_register_uri_handler(req->handle, &welcome);
        httpd_register_uri_handler(req->handle, &connect);
        /* Unregister custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}
