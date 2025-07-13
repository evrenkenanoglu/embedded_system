/**
 * @file Serv_websockets.cpp
 * @brief Source file for Serv_websockets
 *
 * This file contains definitions for the Serv_websockets class and related data types and functions.
 */

#include "Serv_websockets.hpp"
#include "System/LogHandler.h"
#include "cJSON.h"
#include "sdkconfig.h"

/**
 * @brief Event handler for WebSocket events
 *
 * @param handler_args The handler arguments
 * @param base The event base
 * @param event_id The event ID
 * @param event_data The event data
 */
static void websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data);

Serv_websockets::Serv_websockets()
    : _server(NULL)
{
}

Serv_websockets::~Serv_websockets()
{
    stop();
}

sys_error_t Serv_websockets::init()
{
    return ERROR_SUCCESS;
}

sys_error_t Serv_websockets::start()
{
    return ERROR_SUCCESS;
}

sys_error_t Serv_websockets::stop()
{
    return ERROR_SUCCESS;
}

sys_error_t Serv_websockets::restart()
{
    RETURN_ON_ERROR(stop());
    RETURN_ON_ERROR(init());
    return start();
}

void websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {}

void Serv_websockets::startWebSocketServer(httpd_handle_t server)
{
    if (server == NULL)
    {
        SYS_LOG_E("Server is NULL, cannot start websocket server");
        return;
    }
    _server = server;

    SYS_LOG_I("Starting websocket server");
}
void Serv_websockets::stopWebSocketServer()
{
    _server = NULL;
}

sys_error_t Serv_websockets::sendMessage(int client_fd, uint8_t* payload, size_t len, httpd_ws_type_t type)
{
    if (_server == NULL)
    {
        SYS_LOG_E("Server is NULL, cannot send message");
        return ERROR_FAIL;
    }

    // send to a specific client
    httpd_ws_frame_t ws_frame;
    ws_frame.payload = payload;
    ws_frame.len     = len;
    ws_frame.type    = type;

    // print the message
    SYS_LOG_D("Sending message: " + std::string(reinterpret_cast<char*>(payload), len) + ", type: " + std::to_string(type) + ", len: " + std::to_string(len));

    if (httpd_ws_get_fd_info(_server, client_fd) == HTTPD_WS_CLIENT_WEBSOCKET)
    {
        // print the message

        error_t error = httpd_ws_send_frame_async(_server, client_fd, &ws_frame);
        SYS_LOG_I("Sending message to client fd: " + std::to_string(client_fd) + ", type: " + std::to_string(type) + ", len: " + std::to_string(len));
        if (error != ESP_OK)
        {
            SYS_LOG_E("Failed to send message to client fd: " + std::to_string(client_fd));
            return ERROR_FAIL;
        }
    }
    else
    {
        SYS_LOG_E("Client fd: " + std::to_string(client_fd) + " is not a websocket client");
        return ERROR_FAIL;
    }

    return ERROR_SUCCESS;
}

sys_error_t Serv_websockets::broadcast(uint8_t* payload, size_t len, httpd_ws_type_t type)
{
    if (_server == NULL)
    {
        SYS_LOG_E("Server is NULL, cannot send message");
        return ERROR_FAIL;
    }

    static size_t maxClients = CONFIG_LWIP_MAX_LISTENING_TCP;
    size_t        fds        = maxClients;
    int           client_fds[maxClients];

    error_t error = ESP_OK;
    error         = httpd_get_client_list(_server, &fds, client_fds);

    if (error != ESP_OK)
    {
        SYS_LOG_E("Failed to get client list");
        return ERROR_FAIL;
    }

    for (uint8_t i = 0; i < fds; i++)
    {
        if (httpd_ws_get_fd_info(_server, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET)
        {
            sendMessage(client_fds[i], payload, len, type);
        }
    }

    return ERROR_SUCCESS;
}
