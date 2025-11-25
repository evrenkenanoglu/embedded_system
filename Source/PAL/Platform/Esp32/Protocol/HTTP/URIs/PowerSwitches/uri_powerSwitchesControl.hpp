#ifndef URI_POWERSWITCHESCONTROL_HPP
#define URI_POWERSWITCHESCONTROL_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriPut.hpp"

class UriPowerSwitchesControl : public HttpUriPut
{
public:
    UriPowerSwitchesControl();
    ~UriPowerSwitchesControl();

private:
    uint16_t handler(const char* req_ptr, size_t req_len, char* resp_buf, size_t resp_buf_len, void* user_ctx) override;
};

#endif // URI_POWERSWITCHESCONTROL_HPP