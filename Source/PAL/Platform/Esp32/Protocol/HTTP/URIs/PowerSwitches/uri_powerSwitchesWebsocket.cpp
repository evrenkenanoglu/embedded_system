#include "uri_powerSwitchesWebsocket.hpp"
#include "Library/UI/HTTP/PowerSwitches/ui_wifi_power_sockets/output/header/ui_wifi_power_sockets.h"

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"
namespace
{
} // namespace

UriPowerSwitchesWebSocket::UriPowerSwitchesWebSocket(QueueHandle_t eventQueuePowerSwitchesWs)
    : HttpUriWebsocket("/powerSwitchesWs", this, nullptr, nullptr, nullptr)
    , _eventQueuePowerSwitchesWs(eventQueuePowerSwitchesWs)
{
}

UriPowerSwitchesWebSocket::~UriPowerSwitchesWebSocket() {}

int UriPowerSwitchesWebSocket::onOpen(void* user_ctx) const
{
    SYS_LOG_I("PowerSwitches WebSocket opened");
    UriPowerSwitchesWebSocket* self = static_cast<UriPowerSwitchesWebSocket*>(user_ctx);
    RETURN_IF_ERROR(self == nullptr, -1, SYS_LOG_E("WebSocket context is null"));

    // Notify the event manager about the new connection
    // Prepare event data
    PowerSwitchesWs::EventData_t eventData;
    eventData.eventType = PowerSwitchesWs::EventType::CLIENT_CONNECTED;
    eventData.clientId  = getClientId();
    // Send event to the queue
    xQueueSend(self->_eventQueuePowerSwitchesWs, &eventData, 0);
    return 0;
}

int UriPowerSwitchesWebSocket::onMessage(const char* data, size_t len, void* user_ctx) const
{
    SYS_LOG_I("PowerSwitches WebSocket message received: %.*s", static_cast<int>(len), data);
    return 0;
}

void UriPowerSwitchesWebSocket::onClose(void* user_ctx) const
{
    SYS_LOG_I("PowerSwitches WebSocket closed");
}