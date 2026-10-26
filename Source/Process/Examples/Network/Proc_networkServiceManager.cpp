/**
 * @file       Proc_networkServiceManager.cpp
 * @brief      Network service lifecycle coordinator implementation.
 *
 * Subscribes to WifiConfigurationManager events and applies the start/stop
 * policy for AP-dependent and STA-dependent services on the WiFi worker task.
 * @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *             Permission to use, reproduce, copy, prepare derivative works,
 *             modify, distribute, perform, display or sell this software and/or
 *             its documentation for any purpose is prohibited without the express
 *             written consent of Evren Kenanoglu.
 * @date       26/10/2026
 */

#include "Proc_networkServiceManager.hpp"
#include "System/LogHandler.h"
#include <algorithm>

namespace
{
    constexpr uint32_t    kServiceStackSize = 4096;
    constexpr UBaseType_t kServicePriority  = 3;
} // namespace

Proc_networkServiceManager::Proc_networkServiceManager(WifiConfigurationManager& wifiMgr)
    : Process()
    , _networkServicesAP()
    , _networkServicesSTA()
    , _stateAP(NetworkServiceState::INITIALIZED)
    , _stateSTA(NetworkServiceState::INITIALIZED)
    , _xHandle(nullptr)
    , _wifiMgr(wifiMgr)
    , _wifiSubscription(0)
{
}

Proc_networkServiceManager::~Proc_networkServiceManager()
{
    (void)stop();
}

sys_error_t Proc_networkServiceManager::start()
{
    if (getState() == State::RUNNING)
    {
        return ERROR_INVALID_STATE;
    }

    if (_xHandle != nullptr)
    {
        return ERROR_SUCCESS;
    }

    if (xTaskCreate(Proc_networkServiceManager::_workerTrampoline, "netSvcMgr", kServiceStackSize, this, kServicePriority, &_xHandle) != pdPASS)
    {
        _xHandle = nullptr;
        return ERROR_OUT_OF_MEMORY;
    }

    if (_wifiMgr.subscribeEvent(&Proc_networkServiceManager::_wifiEventCb, this, _wifiSubscription) != ERROR_SUCCESS)
    {
        vTaskDelete(_xHandle);
        _xHandle = nullptr;
        return ERROR_OUT_OF_MEMORY;
    }

    setState(State::RUNNING);
    // Make sure services match the connectivity state that already exists.
    applyCurrentState();
    return ERROR_SUCCESS;
}

sys_error_t Proc_networkServiceManager::stop()
{
    if (getState() != State::RUNNING)
    {
        return ERROR_INVALID_STATE;
    }

    if (_wifiSubscription != 0)
    {
        (void)_wifiMgr.unsubscribeEvent(_wifiSubscription);
        _wifiSubscription = 0;
    }

    // Shut every managed service down before tearing ourselves apart.
    _executeNetworkServices(NetworkServiceType::AP, false);
    _executeNetworkServices(NetworkServiceType::STA, false);
    _stateAP  = NetworkServiceState::STOPPED;
    _stateSTA = NetworkServiceState::STOPPED;

    if (_xHandle != nullptr)
    {
        vTaskDelete(_xHandle);
        _xHandle = nullptr;
    }

    setState(State::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_networkServiceManager::pause()
{
    if (getState() != State::RUNNING)
    {
        return ERROR_INVALID_STATE;
    }

    setState(State::PAUSED);
    _executeNetworkServices(NetworkServiceType::AP, false);
    _executeNetworkServices(NetworkServiceType::STA, false);
    _stateAP  = NetworkServiceState::STOPPED;
    _stateSTA = NetworkServiceState::STOPPED;
    return ERROR_SUCCESS;
}

sys_error_t Proc_networkServiceManager::resume()
{
    if (getState() != State::PAUSED)
    {
        return ERROR_INVALID_STATE;
    }

    setState(State::RUNNING);
    applyCurrentState();
    return ERROR_SUCCESS;
}

NetworkServiceState Proc_networkServiceManager::getNetworkServiceState(NetworkServiceType type) const
{
    switch (type)
    {
        case NetworkServiceType::AP:
            return _stateAP;
        case NetworkServiceType::STA:
            return _stateSTA;
        default:
            return NetworkServiceState::STOPPED;
    }
}

sys_error_t Proc_networkServiceManager::registerNetworkService(IProcess& networkService, NetworkServiceType type)
{
    std::vector<IProcess*>* target = nullptr;

    switch (type)
    {
        case NetworkServiceType::AP:
            target = &_networkServicesAP;
            break;
        case NetworkServiceType::STA:
            target = &_networkServicesSTA;
            break;
        default:
            return ERROR_INVALID_ARG;
    }

    if (std::find(target->begin(), target->end(), &networkService) != target->end())
    {
        return ERROR_INVALID_STATE;
    }

    target->push_back(&networkService);
    return ERROR_SUCCESS;
}

sys_error_t Proc_networkServiceManager::unregisterNetworkService(IProcess& networkService, NetworkServiceType type)
{
    std::vector<IProcess*>* target = nullptr;

    switch (type)
    {
        case NetworkServiceType::AP:
            target = &_networkServicesAP;
            break;
        case NetworkServiceType::STA:
            target = &_networkServicesSTA;
            break;
        default:
            return ERROR_INVALID_ARG;
    }

    auto it = std::find(target->begin(), target->end(), &networkService);
    if (it == target->end())
    {
        return ERROR_INVALID_ARG;
    }

    target->erase(it);
    return ERROR_SUCCESS;
}

sys_error_t Proc_networkServiceManager::applyCurrentState()
{
    if (getState() != State::RUNNING)
    {
        return ERROR_INVALID_STATE;
    }

    WifiMgrStatus status = {};
    _wifiMgr.getStatus(status);
    switch (status.state)
    {
        case WifiMgrState::CONNECTED:
            _executeNetworkServices(NetworkServiceType::AP, false);
            _executeNetworkServices(NetworkServiceType::STA, true);
            break;

        case WifiMgrState::PROVISIONING:
            _executeNetworkServices(NetworkServiceType::STA, false);
            _executeNetworkServices(NetworkServiceType::AP, true);
            break;

        default:
            // BOOT/CONNECTING/RECONNECT_WAIT: nothing must be up yet.
            _executeNetworkServices(NetworkServiceType::AP, false);
            _executeNetworkServices(NetworkServiceType::STA, false);
            break;
    }
    return ERROR_SUCCESS;
}

void Proc_networkServiceManager::_wifiEventCb(WifiEvent event, const WifiMgrStatus& status, void* user_data)
{
    (void)status;
    Proc_networkServiceManager* self = static_cast<Proc_networkServiceManager*>(user_data);
    self->_applyPolicy(event);
}

void Proc_networkServiceManager::_applyPolicy(WifiEvent event)
{
    switch (event)
    {
        case WifiEvent::CONNECTED:
            _executeNetworkServices(NetworkServiceType::AP, false);
            _executeNetworkServices(NetworkServiceType::STA, true);
            break;

        case WifiEvent::DISCONNECTED:
            _executeNetworkServices(NetworkServiceType::STA, false);
            break;

        case WifiEvent::AP_SETUP_READY:
            _executeNetworkServices(NetworkServiceType::STA, false);
            _executeNetworkServices(NetworkServiceType::AP, true);
            break;

        case WifiEvent::AP_SETUP_FINISH:
            _executeNetworkServices(NetworkServiceType::AP, false);
            break;

        case WifiEvent::STA_SETUP_READY:
        case WifiEvent::STA_SETUP_FINISH:
        default:
            break; // transitional events; no service action required
    }
}

void Proc_networkServiceManager::_executeNetworkServices(NetworkServiceType type, bool startServices)
{
    std::vector<IProcess*>* target = (type == NetworkServiceType::AP) ? &_networkServicesAP : &_networkServicesSTA;
    NetworkServiceState*    state  = (type == NetworkServiceType::AP) ? &_stateAP : &_stateSTA;

    for (IProcess* service : *target)
    {
        if (service == nullptr)
        {
            continue;
        }

        if (startServices)
        {
            if (service->start() == ERROR_SUCCESS)
            {
                SYS_LOG_I("netSvcMgr: started %s service", (type == NetworkServiceType::AP) ? "AP" : "STA");
            }
        }
        else
        {
            if (service->stop() == ERROR_SUCCESS)
            {
                SYS_LOG_I("netSvcMgr: stopped %s service", (type == NetworkServiceType::AP) ? "AP" : "STA");
            }
        }
    }

    *state = startServices ? NetworkServiceState::RUNNING : NetworkServiceState::STOPPED;
}

void Proc_networkServiceManager::_workerTrampoline(void* param)
{
    static_cast<Proc_networkServiceManager*>(param)->_worker();
}

void Proc_networkServiceManager::_worker()
{
    // The coordinator is fully event-driven; its task only keeps the
    // Process lifecycle alive and could host periodic maintenance later.
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
