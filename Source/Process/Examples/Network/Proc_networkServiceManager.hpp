/**
 * @file       Proc_networkServiceManager.hpp
 * @brief      Network service lifecycle coordinator.
 *
 * Subscribes to WifiConfigurationManager events and applies the start/stop
 * policy for AP-dependent and STA-dependent services. Services themselves
 * stay domain-pure: they know nothing about WiFi.
 * @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *             Permission to use, reproduce, copy, prepare derivative works,
 *             modify, distribute, perform, display or sell this software and/or
 *             its documentation for any purpose is prohibited without the express
 *             written consent of Evren Kenanoglu.
 * @date       26/10/2026
 */

#ifndef Proc_networkServiceManager_HPP
#define Proc_networkServiceManager_HPP

#include "App/Network/WifiConfigurationManager.hpp"
#include "Process/Process.hpp"
#include "System/system.h"
#include <vector>

enum class NetworkServiceType
{
    AP,
    STA
};

enum class NetworkServiceState
{
    INITIALIZED,
    RUNNING,
    STOPPED
};

class Proc_networkServiceManager : public Process
{
private:
    std::vector<IProcess*> _networkServicesAP;  // Access Point Dependent Network Services
    std::vector<IProcess*> _networkServicesSTA; // Station Dependent Network Services

    NetworkServiceState _stateAP;
    NetworkServiceState _stateSTA;

    TaskHandle_t              _xHandle;
    WifiConfigurationManager& _wifiMgr;          // Event source (single source of truth)
    WifiEventSubscription     _wifiSubscription; // Handle held while subscribed

public:
    /**
     * @brief Construct the coordinator.
     *
     * @param wifiMgr WiFi configuration manager whose events drive service start/stop (must outlive this object)
     */
    explicit Proc_networkServiceManager(WifiConfigurationManager& wifiMgr);
    ~Proc_networkServiceManager();

    // Block copy semantics (owns a subscription and a worker task)
    Proc_networkServiceManager(const Proc_networkServiceManager&)            = delete;
    Proc_networkServiceManager& operator=(const Proc_networkServiceManager&) = delete;
    Proc_networkServiceManager(Proc_networkServiceManager&&)                 = delete;
    Proc_networkServiceManager& operator=(Proc_networkServiceManager&&)      = delete;

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
     * @brief Apply the current policy immediately.
     *
     * Starts the services matching the manager's current connectivity state
     * (used after late registration so a service does not wait for the next
     * state transition).
     */
    sys_error_t applyCurrentState();

    /**
     * @brief Get the Network Service State object
     *
     * @param type Network Service Type
     * @return NetworkServiceState
     */
    NetworkServiceState getNetworkServiceState(NetworkServiceType type) const;

private:
    /**
     * @brief Event callback invoked from the WiFi manager worker task.
     */
    static void _wifiEventCb(WifiEvent event, const WifiMgrStatus& status, void* user_data);

    /**
     * @brief FreeRTOS trampoline and idle worker keeping the lifecycle alive.
     */
    static void _workerTrampoline(void* param);
    void        _worker();

    /**
     * @brief Apply the policy for a single event (called on the worker task).
     */
    void _applyPolicy(WifiEvent event);

    /**
     * @brief Execute network services
     *
     * @param type Service group to start or stop
     * @param startServices Start or stop services
     */
    void _executeNetworkServices(NetworkServiceType type, bool startServices);
};

#endif /* Proc_networkServiceManager_HPP */
