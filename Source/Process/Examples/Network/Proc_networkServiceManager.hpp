/**
 * @file Proc_networkServiceManager.hpp
 * @brief Header file for Proc_networkServiceManager
 *
 * This file contains declarations for the Proc_networkServiceManager class and related data types and functions.
 */

#ifndef Proc_networkServiceManager_HPP
#define Proc_networkServiceManager_HPP

#include "Process/Process.hpp"
#include "System/system.h"
#include "Process/Examples/Network/Proc_wifiConfigurationManager.hpp"
#include <vector>

enum class NetworkServiceType
{
    AP,
    STA
};

class Proc_networkServiceManager : public Process
{
private:
    std::vector<IProcess*> _networkServicesAP;  // Access Point Dependent Network Services
    std::vector<IProcess*> _networkServicesSTA; // Station Dependent Network Services

    TaskHandle_t        _xHandle;
    EventGroupHandle_t& _wifiConfigEventGroup;

public:
    Proc_networkServiceManager(EventGroupHandle_t& wifiEventGroup);
    ~Proc_networkServiceManager();

    sys_error_t start() override;

    sys_error_t stop() override;

    sys_error_t pause() override;

    sys_error_t resume() override;

public:
    /**
     * @brief Register a network service that is dependent on the Access Point
     *
     * @param networkService
     * @return system_error_t
     */
    sys_error_t registerNetworkService(IProcess& networkService, NetworkServiceType type);

    /**
     * @brief Register a network service that is dependent on the Station
     *
     * @param networkService
     * @return system_error_t
     */
    sys_error_t unregisterNetworkService(IProcess& networkService, NetworkServiceType type);

    /**
     * @brief Get the Wifi Event Group object
     *
     * @return EventGroupHandle_t&
     */
    EventGroupHandle_t& getWifiConfigEventGroup();

    /**
     * @brief Execute network services
     *
     * @param networkServices Vector of network services
     * @param startServices Start or stop services
     */
    void executeNetworkServices(NetworkServiceType type, bool startServices);
};

#endif /* Proc_networkServiceManager_HPP */
