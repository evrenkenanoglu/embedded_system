/**
 * @file Proc_wifiConfigurationManager.hpp
 * @brief Header file for Proc_wifiConfigurationManager
 *
 * This file contains declarations for the Proc_wifiConfigurationManager class and related data types and functions.
 */

#ifndef Proc_wifiConfigurationManager_HPP
#define Proc_wifiConfigurationManager_HPP

#include "HAL/Platform/ESP32/cpx_wifi.h"
#include "Process/IProcess.hpp"
#include "Process/Proc_httpServer.hpp"
#include "System/system.h"
#include "wifiConfigEvents.hpp"

enum class ProgramState
{
    UNINITIALIZED = 0,
    INITIALIZED,
    CONNECTED,
    DISCONNECTED,
    STA_MODE,    // Station Mode
    AP_STA_MODE, // Access Point and Station Mode
    AWAITING_CREDENTIALS,
    CREDENTIALS_NOT_FOUND_OR_INVALID,
    TRY_CONNECT,
    RESTART, // Restart WiFi
    CONNECTION_FAILED,

};

class Proc_wifiConfigurationManager : public IProcess
{
private:
    cpx_wifi&           _wifiCpx;
    wifi_config_t&      _wifiConfig;
    TaskHandle_t        _xHandle;
    ProgramState        _programState;
    EventGroupHandle_t _wifiEventGroup;
    

public:
    Proc_wifiConfigurationManager(cpx_wifi& wifiCpx, wifi_config_t& wifiConfig);
    ~Proc_wifiConfigurationManager();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

public:
    /**
     * @brief Get the Program State object
     *
     * @return ProgramState
     */
    ProgramState getProgramState() const;

    /**
     * @brief Set the Program State object
     *
     * @param programState
     */
    void setProgramState(ProgramState programState);

    /**
     * @brief Get the Wifi Cpx object
     *
     * @return cpx_wifi&
     */
    cpx_wifi& getWifiCpx() const;

    /**
     * @brief Get the Wifi Event Group object
     *
     * @return EventGroupHandle_t
     */
    EventGroupHandle_t& getWifiEventGroup();

};

#endif /* Proc_wifiConfigurationManager_HPP */
