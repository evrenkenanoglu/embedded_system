#include "uri_powerSwitchesWebsocket.hpp"

#include "HAL/Platform/ESP32/library/logImpl.h"
#include "Library/UI/HTTP/ui_wifi_power_sockets.h"
#include <esp_http_server.h>

namespace
{
constexpr uint16_t powerSwitchesUpdateTaskStackSize = 4096; // bytes
constexpr uint8_t  powerSwitchesUpdateTaskPriority  = 5;
constexpr char     powerSwitchesUpdateTaskName[]    = "powerSwitchesUpdateUpdate";
constexpr uint16_t programRoutineTaskDelay          = 50; // milliseconds
constexpr uint8_t  powerSwitchesUpdateQueueSize     = sizeof(Proc_Switches::SwitchQueue_t);
} // namespace

static esp_err_t webSocketHandler(httpd_req_t* req);

UriPowerSwitchesWebSocket::UriPowerSwitchesWebSocket(Serv_websockets& websocketServer)
    : HttpUriWebsocket("/powerSwitchesWs", webSocketHandler, this, nullptr)
    , _websocketServer(websocketServer)
{
}

UriPowerSwitchesWebSocket::~UriPowerSwitchesWebSocket() {}

static esp_err_t webSocketHandler(httpd_req_t* req)
{
    if (req->method == HTTP_GET)
    {
        httpd_resp_set_type(req, "application/json");
        return ESP_OK;
    }
    // httpd_ws_frame_t ws_pkt;
    // memset(&ws_pkt, 0, sizeof(ws_pkt));
    // ws_pkt.type    = HTTPD_WS_TYPE_TEXT;
    // ws_pkt.payload = NULL;
    // ws_pkt.len     = 0;
    // httpd_ws_recv_frame(req, &ws_pkt, 0);
    // ws_pkt.payload = (uint8_t*)malloc(ws_pkt.len + 1);
    // httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    // ws_pkt.payload[ws_pkt.len] = 0;
    // ESP_LOGI("TAG", "Received: %s", ws_pkt.payload);

    // // Echo back to client
    // char reply[64];
    // snprintf(reply, sizeof(reply), "ESP32 received: %s", ws_pkt.payload);
    // httpd_ws_frame_t ws_res = {.final = true, .fragmented = false, .type = HTTPD_WS_TYPE_TEXT, .payload = (uint8_t*)reply, .len = strlen(reply)};
    // httpd_ws_send_frame(req, &ws_res);
    // // print socket descriptor info

    // ESP_LOGI("TAG", "Client FD: %d, Info: %d", httpd_req_to_sockfd(req), (int)httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)));

    // // httpd_ws_send_frame_async(server, httpd_req_to_sockfd(req), &ws_res);
    // free(ws_pkt.payload);

    return ESP_OK;
}

esp_err_t UriPowerSwitchesWebSocket::updateSwitchStates(uint16_t socketId, bool state)
{
    char message[50];
    std::sprintf(message, "{\"socketId\": %d, \"state\": %d}", socketId, static_cast<uint8_t>(state));

    RETURN_ON_ERROR(_websocketServer.broadcast(reinterpret_cast<uint8_t*>(message), sizeof(message), HTTPD_WS_TYPE_TEXT));

    return ERROR_SUCCESS;
}