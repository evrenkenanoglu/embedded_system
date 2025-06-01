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

    // Recieive WebSocket frame
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt));
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE("UriPowerSwitchesWebSocket", "Failed to receive WebSocket frame");
        return ret;
    }
    ws_pkt.payload = (uint8_t*)malloc(ws_pkt.len + 1);
    httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    ws_pkt.payload[ws_pkt.len] = 0; // Null-terminate the payload
    ESP_LOGI("UriPowerSwitchesWebSocket", "Received: %s", ws_pkt.payload);

    UriPowerSwitchesWebSocket* self = static_cast<UriPowerSwitchesWebSocket*>(req->user_ctx);

    // Send dummy switches data to the client : socketId 0, state false

    std::string message1 = "{\"socketId\": 0, \"state\": false}";
    int         clientId = httpd_req_to_sockfd(req);
    self->getWebsocketServer().sendMessage(clientId, reinterpret_cast<uint8_t*>(message1.data()), message1.size(), HTTPD_WS_TYPE_TEXT);

    return ESP_OK;
}

esp_err_t UriPowerSwitchesWebSocket::updateSwitchStates(uint16_t socketId, bool state)
{
    // Use string formatting directly with a reserve to avoid reallocations
    std::string message;
    message.reserve(40); // Pre-allocate enough space for the typical JSON message
    message = "{\"socketId\": ";
    message += std::to_string(socketId);
    message += ", \"state\": ";
    message += state ? "1" : "0";
    message += "}";

    ESP_LOGI("UriPowerSwitchesWebSocket", "Updating switch state: %s", message.c_str());

    // Use const_cast to avoid the reinterpret_cast which is less type-safe
    return _websocketServer.broadcast(reinterpret_cast<uint8_t*>(message.data()), message.size(), HTTPD_WS_TYPE_TEXT);
}

Serv_websockets& UriPowerSwitchesWebSocket::getWebsocketServer()
{
    return _websocketServer;
}