#ifndef URI_POWERSWITCHESCONTROL_HPP
#define URI_POWERSWITCHESCONTROL_HPP

#include "PAL/Platform/Esp32/Protocol/HTTP/URIs/HttpUriPut.hpp"

class UriPowerSwitchesControl : public HttpUriPut
{
public:
    UriPowerSwitchesControl(QueueHandle_t switchesQueue);
    ~UriPowerSwitchesControl();

    /**
     * @brief Get the Switches Queue object
     *
     * @return QueueHandle_t
     */
    QueueHandle_t getSwitchesQueue() const;

private:
    QueueHandle_t  _switchesQueue;
};

#endif // URI_POWERSWITCHESCONTROL_HPP