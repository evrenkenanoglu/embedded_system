/**
 * @file Serv_websockets.hpp
 * @brief Header file for Serv_websockets
 *
 * This file contains declarations for the Serv_websockets class and related data types and functions.
 */

#ifndef SERV_WEBSOCKETS_HPP
#define SERV_WEBSOCKETS_HPP

#include "PAL/Pal.h"
#include "URIs/HttpUriWebsocket.hpp"
#include <esp_http_server.h>

class Serv_websockets : public PAL_Service
{
private:
    httpd_handle_t _server;

    // private members

public:
    Serv_websockets();
    ~Serv_websockets();

    sys_error_t init() override;

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t restart() override;

public:
    // Register the websocket server callback function
    void startWebSocketServer(httpd_handle_t _server);

    // Stop the websocket server
    void stopWebSocketServer();

    // Send a message to the websocket
    sys_error_t sendMessage(int client_fd, uint8_t* payload, size_t len, httpd_ws_type_t type);

    // Broadcast a message to all connected websocket clients
    sys_error_t broadcast(uint8_t* payload, size_t len, httpd_ws_type_t type);
    
};
#endif /* SERV_WEBSOCKETS_HPP */
