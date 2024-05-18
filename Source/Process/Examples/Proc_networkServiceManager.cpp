/**
 * @file Proc_networkServiceManager.cpp
 * @brief Source file for Proc_networkServiceManager
 *
 * This file contains definitions for the Proc_networkServiceManager class and related data types and functions.
 */

#include "Proc_networkServiceManager.hpp"
#include "HAL/Platform/ESP32/Library/logImpl.h"
#include <algorithm>

namespace
{
constexpr uint16_t taskStackSize = 4096; // bytes
constexpr uint8_t  taskPriority  = 5;
constexpr char     taskName[]    = "networkServiceMngrTask";
} // namespace

SemaphoreHandle_t mutex;

static void programTask(void* pvParameters);

Proc_networkServiceManager::Proc_networkServiceManager(EventGroupHandle_t& wifiEventGroup) : _xHandle(nullptr), _wifiEventGroup(wifiEventGroup)
{
    mutex = xSemaphoreCreateMutex();
}

Proc_networkServiceManager::~Proc_networkServiceManager()
{
    // destructor implementation
}

sys_error_t Proc_networkServiceManager::start()
{
    setState(IProcess::State::RUNNING);
    BaseType_t result = xTaskCreate(programTask, taskName, taskStackSize, this, taskPriority, &_xHandle);
    if (result != pdPASS)
    {
        return ERROR_FAIL;
    }

    logger().log(ILog::LogLevel::INFO, "Starting network service manager");
    return ERROR_SUCCESS;
}

sys_error_t Proc_networkServiceManager::stop()
{
    setState(IProcess::State::STOPPED);
    vTaskDelete(_xHandle);
    return ERROR_SUCCESS;
}

sys_error_t Proc_networkServiceManager::pause()
{
    logger().log(ILog::LogLevel::ERROR, "Pause not implemented");
    return ERROR_NOT_IMPLEMENTED;
}

sys_error_t Proc_networkServiceManager::resume()
{
    logger().log(ILog::LogLevel::ERROR, "Pause not implemented");
    return ERROR_NOT_IMPLEMENTED;
}

std::vector<IProcess*>& Proc_networkServiceManager::getNetworkServicesAP()
{
    return _networkServicesAP;
}

std::vector<IProcess*>& Proc_networkServiceManager::getNetworkServicesSTA()
{
    return _networkServicesSTA;
}

sys_error_t Proc_networkServiceManager::registerNetworkService(IProcess& networkService, NetworkServiceType type)
{
    // mutex lock
    xSemaphoreTake(mutex, portMAX_DELAY);

    std::vector<IProcess*>& networkServices = (type == NetworkServiceType::AP) ? _networkServicesAP : _networkServicesSTA;

    // Check if the network service is already registered
    auto it = std::find(networkServices.begin(), networkServices.end(), &networkService);
    if (it != networkServices.end())
    {
        // mutex unlock
        xSemaphoreGive(mutex);
        return ERROR_SUCCESS;
    }
    networkServices.emplace_back(&networkService);

    // mutex unlock
    xSemaphoreGive(mutex);
    return ERROR_SUCCESS;
}

sys_error_t Proc_networkServiceManager::unregisterNetworkService(IProcess& networkService, NetworkServiceType type)
{
    // mutex lock
    xSemaphoreTake(mutex, portMAX_DELAY);

    std::vector<IProcess*>& networkServices = (type == NetworkServiceType::AP) ? _networkServicesAP : _networkServicesSTA;

    auto it = std::find(networkServices.begin(), networkServices.end(), &networkService);
    if (it != networkServices.end())
    {
        networkServices.erase(it);
    }

    // mutex unlock
    xSemaphoreGive(mutex);

    return ERROR_SUCCESS;
}

// Helper function to start or stop network services
void Proc_networkServiceManager::executeNetworkServices(NetworkServiceType type, bool startServices)
{
    // mutex lock
    xSemaphoreTake(mutex, portMAX_DELAY);

    std::vector<IProcess*>& networkServices = (type == NetworkServiceType::AP) ? _networkServicesAP : _networkServicesSTA;

    for (auto networkService : networkServices)
    {
        if (networkService == nullptr)
        {
            continue;
        }

        if (startServices)
        {
            networkService->start();
        }
        else
        {
            networkService->stop();
        }
    }

    // mutex unlock
    xSemaphoreGive(mutex);
}

static void programTask(void* pvParameters)
{
    // Cast the provided parameter to Proc_networkServiceManager
    Proc_networkServiceManager* proc = static_cast<Proc_networkServiceManager*>(pvParameters);

    for (;;)
    {
        // Wait for any of the specified event bits to be set
        EventBits_t eventBits = xEventGroupWaitBits(proc->getWifiEventGroup(),        // Event Group Handle
                                                                                      // Bits to wait for
                                                    WIFI_CONFIG_AP_SETUP_READY |      //
                                                        WIFI_CONFIG_AP_SETUP_FINISH | //
                                                        WIFI_CONFIG_STA_SETUP_READY | //
                                                        WIFI_CONFIG_STA_SETUP_FINISH, //
                                                    pdTRUE,                           // Clear bits on exit
                                                    pdFALSE,                          // Wait for any bit
                                                    portMAX_DELAY);                   // Wait indefinitely

        // If the AP setup ready bit is set, start the AP network services
        if (eventBits & WIFI_CONFIG_AP_SETUP_READY)
        {
            proc->executeNetworkServices(NetworkServiceType::AP, true);
        }
        // If the AP setup finish bit is set, stop the STA network services
        else if (eventBits & WIFI_CONFIG_AP_SETUP_FINISH)
        {
            proc->executeNetworkServices(NetworkServiceType::AP, false);
        }

        // If the STA setup ready bit is set, start the STA network services
        if (eventBits & WIFI_CONFIG_STA_SETUP_READY)
        {
            proc->executeNetworkServices(NetworkServiceType::STA, true);
        }
        // If the STA setup finish bit is set, stop the STA network services
        else if (eventBits & WIFI_CONFIG_STA_SETUP_FINISH)
        {
            proc->executeNetworkServices(NetworkServiceType::STA, false);
        }
    }
}