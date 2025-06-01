#ifndef URI_POWERSWITCHESWEBSOCKET_HPP
#define URI_POWERSWITCHESWEBSOCKET_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/Serv_websockets.hpp"
#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriWebsocket.hpp"
#include "Process/Examples/Peripheral/Proc_Switches.hpp"
#include "System/system.h"

class UriPowerSwitchesWebSocket : public HttpUriWebsocket
{
public:
    UriPowerSwitchesWebSocket(Serv_websockets& websocketServer, Proc_Switches& procSwitches);
    ~UriPowerSwitchesWebSocket();
    sys_error_t start();

    error_t updateSwitchStates(uint16_t socketId, bool state);

    // Serv_websockets& getWebsocketServer();
    // Proc_Switches& getProcSwitches();
    sys_error_t sendAllSwitchStates(int clientId);
private:
    Serv_websockets& _websocketServer;
    Proc_Switches&   _procSwitches;

};

#endif // URI_POWERSWITCHESWEBSOCKET_HPP