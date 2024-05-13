/**
 * @file proc_httpServer.cpp
 * @brief Source file for proc_httpServer
 *
 * This file contains definitions for the proc_httpServer class and related data types and functions.
 */

#include "proc_httpServer.hpp"
#include "HAL/Platform/ESP32/library/logImpl.h"
#include "Library/UI/HTTP/ui_welcome_wifi_connect.h"
#include "cJSON.h"
#include "protocol_examples_utils.h"
#include "string.h"
#include <esp_log.h>
#include <sstream>
#include <stdlib.h>

static const char* TAG = "example";
#define MIN(x, y)                      ((x) < (y) ? (x) : (y))
#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

static esp_err_t welcome_get_handler(httpd_req_t* req);
static esp_err_t connect_post_handler(httpd_req_t* req);
static esp_err_t ctrl_put_handler(httpd_req_t* req);
static esp_err_t scan_get_handler(httpd_req_t* req);

static const httpd_uri_t welcome = {.uri     = "/welcome",
                                    .method  = HTTP_GET,
                                    .handler = welcome_get_handler,
                                    /* Let's pass response string in user
                                     * context to demonstrate it's usage */
                                    .user_ctx = (void*)(HTML_UI_WELCOME_WIFI_CONNECT_CONTENT)};

static const httpd_uri_t scan = {.uri = "/scan", .method = HTTP_GET, .handler = scan_get_handler, .user_ctx = NULL};

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
        httpd_register_uri_handler(_server, &scan);
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
    char content[256];

    // Read the content of the POST request
    int ret = httpd_req_recv(req, content, sizeof(content));

    if (ret <= 0)
    { // 0 return value indicates connection closed
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Null terminate the content string
    content[ret] = '\0';

    // Parse the JSON data
    cJSON* json = cJSON_Parse(content);
    if (json == NULL)
    {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            ESP_LOGE(TAG, "Error before: %s", error_ptr);
        }
        return ESP_FAIL;
    }

    // Get the SSID and password from the JSON data
    cJSON* json_ssid     = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    cJSON* json_password = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (cJSON_IsString(json_ssid) && (json_ssid->valuestring != NULL) && cJSON_IsString(json_password) && (json_password->valuestring != NULL))
    {
        ESP_LOGI(TAG, "SSID: %s", json_ssid->valuestring);
        ESP_LOGI(TAG, "Password: %s", json_password->valuestring);
    }

    // Send back the SSID and password
    cJSON* response             = cJSON_CreateObject();
    char   responseMessage[128] = "Received SSID and password. Connecting to ";

    strcat(responseMessage, json_ssid->valuestring);
    strcat(responseMessage, "...");
    printf("responseMessage: %s\n", responseMessage);
    cJSON_AddStringToObject(response, "message", responseMessage);

    char* response_str = cJSON_PrintUnformatted(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));

    cJSON_Delete(json);
    cJSON_Delete(response);
    free(response_str);

    return ESP_OK;
}

static esp_err_t scan_get_handler(httpd_req_t* req)
{
    logger().log(ILog::LogLevel::INFO, "Scan request received!");
    // Create a JSON array of APs
    cJSON* ap_array = cJSON_CreateArray();

    for(int i = 0; i < 10; i++)
    {
        cJSON* ap = cJSON_CreateObject();
        cJSON_AddStringToObject(ap, "ssid", "TEST SSID");
        cJSON_AddItemToArray(ap_array, ap);
        // cJSON_Delete(ap); // Do not delete the object, it will be deleted when the array is deleted
    }

    // Send the JSON array
    char* response_str = cJSON_PrintUnformatted(ap_array);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));
    
    cJSON_Delete(ap_array); 
    
    return ERROR_SUCCESS;
}

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

// [{"ssid":"TEST SSID"},{"ssid":"TEST SSID"},{"ssid":"TEST SSID"},{"ssid":"TEST SSID"},{"ssid":"TEST SSID"},{"ssid":"TEST SSID"},{"ssid":"TEST SSID"},{"ssid":"TEST SSID"},{"ssid":"TEST SSID"},{"ssid":"TEST SSID"}]
