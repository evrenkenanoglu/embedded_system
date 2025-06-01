#ifndef URI_POWERSWITCHESWEBSOCKET_HPP
#define URI_POWERSWITCHESWEBSOCKET_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/Serv_websockets.hpp"
#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriWebsocket.hpp"
#include "Process/Examples/Peripheral/Proc_Switches.hpp"
#include "System/system.h"

class UriPowerSwitchesWebSocket : public HttpUriWebsocket
{
public:
    UriPowerSwitchesWebSocket(Serv_websockets& websocketServer);
    ~UriPowerSwitchesWebSocket();
    sys_error_t start();

    error_t updateSwitchStates(uint16_t socketId, bool state);

    Serv_websockets& getWebsocketServer();
private:
    Serv_websockets& _websocketServer;
};

#endif // URI_POWERSWITCHESWEBSOCKET_HPP