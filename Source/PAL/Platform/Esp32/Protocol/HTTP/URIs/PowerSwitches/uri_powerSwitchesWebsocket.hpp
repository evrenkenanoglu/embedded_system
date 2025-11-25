#ifndef URI_POWERSWITCHESWEBSOCKET_HPP
#define URI_POWERSWITCHESWEBSOCKET_HPP

#include "PAL/Protocols/HTTP/IHttpServer.hpp"
#include "PAL/Platform/Esp32/Protocol/HTTP/Serv_websockets.hpp"
#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriWebsocket.hpp"
#include "Process/Examples/Peripheral/Proc_Switches.hpp"
#include "System/system.h"

class UriPowerSwitchesWebSocket : public HttpUriWebsocket
{
public:
    UriPowerSwitchesWebSocket(IHttpServer& httpServer);
    ~UriPowerSwitchesWebSocket();
    // sys_error_t updateSwitchStates(uint16_t socketId, bool state);
    // sys_error_t sendAllSwitchStates(int clientId);

private:
    int  onOpen(void* user_ctx) const override;
    int  onMessage(const char* data, size_t len, void* user_ctx) const override;
    void onClose(void* user_ctx) const override;

private:
    IHttpServer& _httpServer;
};

#endif // URI_POWERSWITCHESWEBSOCKET_HPP