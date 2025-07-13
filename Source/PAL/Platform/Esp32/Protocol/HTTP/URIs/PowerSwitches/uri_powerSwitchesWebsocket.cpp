#include "uri_powerSwitchesWebsocket.hpp"

#include "Library/UI/HTTP/PowerSwitches/ui_wifi_power_sockets/output/header/ui_wifi_power_sockets.h"
#include "System/LogHandler.h"
#include <esp_http_server.h>
#include <esp_log.h>

namespace
{
constexpr uint16_t powerSwitchesUpdateTaskStackSize = 4096; // bytes
constexpr uint8_t  powerSwitchesUpdateTaskPriority  = 5;
constexpr char     powerSwitchesUpdateTaskName[]    = "powerSwitchesUpdateUpdate";
constexpr uint16_t programRoutineTaskDelay          = 50; // milliseconds
constexpr uint8_t  powerSwitchesUpdateQueueSize     = sizeof(Proc_Switches::SwitchQueue_t);
} // namespace

static esp_err_t webSocketHandler(httpd_req_t* req);

UriPowerSwitchesWebSocket::UriPowerSwitchesWebSocket(Serv_websockets& websocketServer, Proc_Switches& procSwitches)
    : HttpUriWebsocket("/powerSwitchesWs", webSocketHandler, this, nullptr)
    , _websocketServer(websocketServer)
    , _procSwitches(procSwitches)
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

    // Receive WebSocket frame
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(ws_pkt));
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        SYS_LOG_E("Failed to receive WebSocket frame");
        return ret;
    }
    ws_pkt.payload = (uint8_t*)malloc(ws_pkt.len + 1);
    httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    ws_pkt.payload[ws_pkt.len] = 0; // Null-terminate the payload
    SYS_LOG_I("Received: %s", ws_pkt.payload);

    // Send status of all switches to the connected client
    UriPowerSwitchesWebSocket* self = static_cast<UriPowerSwitchesWebSocket*>(req->user_ctx);

    if (self == nullptr)
    {
        SYS_LOG_E("WebSocket context is null");
        free(ws_pkt.payload);
        return ESP_FAIL;
    }

    int clientId = httpd_req_to_sockfd(req);
    self->sendAllSwitchStates(clientId);

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

    sys_error_t error = _websocketServer.broadcast(reinterpret_cast<uint8_t*>(message.data()), message.size(), HTTPD_WS_TYPE_TEXT);
    SYS_LOG_D("Updating switch state!");
    return error;
}

// Serv_websockets& UriPowerSwitchesWebSocket::getWebsocketServer()
// {
//     return _websocketServer;
// }

// Proc_Switches& UriPowerSwitchesWebSocket::getProcSwitches()
// {
//     return _procSwitches;
// }

sys_error_t UriPowerSwitchesWebSocket::sendAllSwitchStates(int clientId)
{
    std::vector<Proc_Switches::Switch_t>* switches = _procSwitches.getSwitches();
    if (switches->empty())
    {
        SYS_LOG_W("UriPowerSwitchesWebSocket", "No switches to send states for.");
        return ERROR_SUCCESS;
    }

    std::string message;
    message.reserve(100); // Pre-allocate enough space for the typical JSON message
    message += "{\"socketId\": ";
    message += std::to_string(ALL_SWITCHES); // Assuming ALL_SWITCHES is a constant defined in Proc_Switches
    message += ", \"state\": [";
    for (size_t i = 0; i < switches->size(); ++i)
    {
        message += std::to_string(switches->at(i).state ? 1 : 0);
        if (i < switches->size() - 1)
        {
            message += ", ";
        }
    }
    message += "]}";

    sys_error_t error = _websocketServer.sendMessage(clientId, reinterpret_cast<uint8_t*>(message.data()), message.size(), HTTPD_WS_TYPE_TEXT);

    SYS_LOG_D("Sending all switch states!");
    return error;
}