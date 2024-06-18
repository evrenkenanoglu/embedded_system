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
// #include "wifiConfigEvents.hpp"

#include "esp_bit_defs.h"

// Define event bits
#define WIFI_CONFIG_AP_SETUP_READY          BIT0 // Access Point Setup Ready
#define WIFI_CONFIG_AP_SETUP_FINISH         BIT1 // Access Point Setup Shutdown

#define WIFI_CONFIG_STA_SETUP_READY         BIT2 // Station Setup Ready
#define WIFI_CONFIG_STA_SETUP_FINISH        BIT3 // Station Setup Shutdown

#define WIFI_CONFIG_SCAN_REQUESTED          BIT4 //
#define WIFI_CONFIG_SCAN_DONE               BIT5

#define WIFI_CONFIG_CONNECTED_TO_AP         BIT6
#define WIFI_CONFIG_DISCONNECTED_FROM_AP    BIT7

#define WIFI_CONFIG_TRY_CONNECT             BIT2
#define WIFI_CONFIG_CONNECTED               BIT3
#define WIFI_CONFIG_DISCONNECTED            BIT4
#define WIFI_CONFIG_CONNECTION_FAILED       BIT5
#define WIFI_CONFIG_WAITING_FOR_CREDENTIALS BIT0
#define WIFI_CONFIG_CREDENTIALS_STORED_BIT  BIT6

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
    cpx_wifi&          _wifiCpx;
    wifi_config_t&     _wifiConfig;
    TaskHandle_t       _xHandleProgram;
    TaskHandle_t       _xHandleWifiEventHandler;
    TaskHandle_t       _xHandleWifiConfigEventHandler;
    ProgramState       _programState;
    EventGroupHandle_t _wifiConfigEventGroup;
    QueueHandle_t      _wifiConfigScanResults;

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
    EventGroupHandle_t& getWifiConfigEventGroup();

    /**
     * @brief Get the Wifi Config Scan Results object
     *
     * @return QueueHandle_t
     */
    QueueHandle_t getWifiConfigScanResults();
};

#endif /* Proc_wifiConfigurationManager_HPP */
