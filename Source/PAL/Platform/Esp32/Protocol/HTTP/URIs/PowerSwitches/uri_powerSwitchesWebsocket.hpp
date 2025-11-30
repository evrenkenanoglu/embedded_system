#ifndef URI_POWERSWITCHESWEBSOCKET_HPP
#define URI_POWERSWITCHESWEBSOCKET_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriWebsocket.hpp"

namespace PowerSwitchesWs
{
enum class EventType : uint8_t
{
    CLIENT_CONNECTED,
    CLIENT_DISCONNECTED,
};

typedef struct
{
    EventType eventType;
    int       clientId;
} EventData_t;

constexpr uint8_t EVENT_QUEUE_SIZE = 5;

}; // namespace PowerSwitchesWs

class UriPowerSwitchesWebSocket : public HttpUriWebsocket
{
public:
    UriPowerSwitchesWebSocket(QueueHandle_t eventQueuePowerSwitchesWs);
    ~UriPowerSwitchesWebSocket();

private:
    int  onOpen(void* user_ctx) const override;
    int  onMessage(const char* data, size_t len, void* user_ctx) const override;
    void onClose(void* user_ctx) const override;

private:
    QueueHandle_t _eventQueuePowerSwitchesWs;
};

#endif // URI_POWERSWITCHESWEBSOCKET_HPP