/**
 * @file Proc_wifiConfigurationManager.cpp
 * @brief Source file for Proc_wifiConfigurationManager
 *
 * This file contains definitions for the Proc_wifiConfigurationManager class and related data types and functions.
 */

#include "Proc_wifiConfigurationManager.hpp"
#include "HAL/Platform/ESP32/Library/logImpl.h"

namespace
{
constexpr uint16_t taskDelay       = 1000; // milliseconds
constexpr uint16_t taskStackSize   = 4096; // bytes
constexpr uint8_t  taskPriority    = 5;
constexpr char     taskName[]      = "wifiConfigMngrTask";
constexpr uint8_t  tryConnectCount = 3;
constexpr uint8_t  tryConnectDelay = 100; // milliseconds
} // namespace

static void programTask(void* pvParameters);

Proc_wifiConfigurationManager::Proc_wifiConfigurationManager(cpx_wifi& wifiCpx, wifi_config_t& wifiConfig) : _wifiCpx(wifiCpx), _wifiConfig(wifiConfig), _xHandle(nullptr), _programState(ProgramState::UNINITIALIZED), _wifiEventGroup(xEventGroupCreate())
{
    wifiCpx.set(&_wifiConfig);
    setState(IProcess::State::INITIALIZED);
}

Proc_wifiConfigurationManager::~Proc_wifiConfigurationManager()
{
    // destructor implementation
}

sys_error_t Proc_wifiConfigurationManager::start()
{
    xTaskCreate(programTask, taskName, taskStackSize, this, taskPriority, &_xHandle);
    setState(IProcess::State::RUNNING);
    return ERROR_SUCCESS;
}

sys_error_t Proc_wifiConfigurationManager::stop()
{
    vTaskDelete(_xHandle);
    setState(IProcess::State::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_wifiConfigurationManager::pause()
{
    vTaskSuspend(_xHandle);
    setState(IProcess::State::PAUSED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_wifiConfigurationManager::resume()
{
    vTaskResume(_xHandle);
    setState(IProcess::State::RUNNING);
    return ERROR_SUCCESS;
}

ProgramState Proc_wifiConfigurationManager::getProgramState() const
{
    return _programState;
}

void Proc_wifiConfigurationManager::setProgramState(ProgramState programState)
{
    _programState = programState;
}

cpx_wifi& Proc_wifiConfigurationManager::getWifiCpx() const
{
    return _wifiCpx;
}

EventGroupHandle_t& Proc_wifiConfigurationManager::getWifiEventGroup()
{
    return _wifiEventGroup;
}

void programTask(void* pvParameters)
{
    Proc_wifiConfigurationManager* proc = (Proc_wifiConfigurationManager*)pvParameters;

    for (;;)
    {
        switch (proc->getProgramState())
        {

            case ProgramState::UNINITIALIZED:
            {
                proc->setProgramState(ProgramState::INITIALIZED);
            }
            break;

            case ProgramState::INITIALIZED:
            {
                logger().log(ILog::LogLevel::INFO, "WiFi Configuration Manager Initialized");
                // Check if ssid and password are stored in NVS
                // if(proc->getWifiCpx().getSsid().empty() && proc->getWifiCpx().getPassword().empty())
                if(true)
                {
                    // If not, change the state to CREDENTIALS_NOT_FOUND_OR_INVALID
                    proc->setProgramState(ProgramState::CREDENTIALS_NOT_FOUND_OR_INVALID);
                }
                else
                {
                    // If yes, start wifi in STA mode
                    proc->getWifiCpx().stop();
                    proc->setProgramState(ProgramState::STA_MODE);
                }
            }
            break;

            case ProgramState::TRY_CONNECT:
            {
                logger().log(ILog::LogLevel::INFO, "Trying to connect to the network...");
                for (uint8_t i = 0; i < tryConnectCount; i++)
                { // Try to connect to the network
                    if (proc->getWifiCpx().connect())
                    {
                        // If connection fails, change the state to DISCONNECTED

                        if (i == tryConnectCount - 1) // If the connection fails after the last try, change the state to DISCONNECTED
                            proc->setProgramState(ProgramState::CREDENTIALS_NOT_FOUND_OR_INVALID);
                        else
                            continue;
                    }
                    else
                    {
                        if (proc->getWifiCpx().getWifiMode() == WIFI_MODE_STA)
                        {
                            // If the WiFi mode is STA, change the state to CONNECTED
                            proc->setProgramState(ProgramState::CONNECTED);
                            break;
                        }
                        else if (proc->getWifiCpx().getWifiMode() == WIFI_MODE_APSTA)
                        {
                            // If the WiFi mode is AP-STA, change the state to restart the WiFi in STA mode
                            proc->setProgramState(ProgramState::RESTART);
                            break;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(tryConnectDelay));
                }
            }
            break;

            case ProgramState::CREDENTIALS_NOT_FOUND_OR_INVALID:
            {
                // Notify the user that the credentials are not found or invalid
                logger().log(ILog::LogLevel::WARNING, "Credentials not found or invalid");
                // Stop WiFi
                std::cout << "Stopping WiFi..."<< std::endl;
                proc->getWifiCpx().stop();

                std::cout << "Changing state..."<< std::endl;
                // Start AP-STA mode
                proc->setProgramState(ProgramState::AP_STA_MODE);
            }
            break;

            case ProgramState::AP_STA_MODE:
            {
                logger().log(ILog::LogLevel::INFO, "Starting AP-STA mode...");
                
                // Set WiFi mode as AP-STA mode
                proc->getWifiCpx().setWifiMode(WIFI_MODE_APSTA);
                proc->getWifiCpx().start();

                std::cout << "Changing state to AWAITING_CREDENTIALS..." << std::endl;
                proc->setProgramState(ProgramState::AWAITING_CREDENTIALS);
            }
            break;

            case ProgramState::STA_MODE:
            {
                logger().log(ILog::LogLevel::INFO, "Starting STA mode...");
                // Set WiFi mode as STA mode
                proc->getWifiCpx().setWifiMode(WIFI_MODE_STA);
                proc->getWifiCpx().start();
                proc->setProgramState(ProgramState::TRY_CONNECT);
            }
            break;

            case ProgramState::AWAITING_CREDENTIALS:
            {
                logger().log(ILog::LogLevel::INFO, "Awaiting credentials...");
                // Wait until user enters the SSID and password
                xEventGroupWaitBits(proc->getWifiEventGroup(), WIFI_CONFIG_CREDENTIALS_STORED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
                // If user enters, try to connect to the network
                proc->setProgramState(ProgramState::TRY_CONNECT);
            }
            break;

            case ProgramState::CONNECTED:
            {
                logger().log(ILog::LogLevel::INFO, "Connected to the network");
                // Notify the user that the connection is successful
                // Change the state to STA_MODE
                proc->setProgramState(ProgramState::STA_MODE);
            }
            break;

            case ProgramState::RESTART:
            {
                logger().log(ILog::LogLevel::INFO, "Restarting WiFi...");
                // Notify the user that the WiFi is restarting

                // Restart WiFi
                proc->getWifiCpx().stop();
                proc->setProgramState(ProgramState::INITIALIZED);
            }
            break;

            case ProgramState::DISCONNECTED:
            {
                logger().log(ILog::LogLevel::INFO, "Disconnected from the network");
                // Try to connect to the network
                proc->setProgramState(ProgramState::TRY_CONNECT);
            }
            break;

            case ProgramState::CONNECTION_FAILED:
            {
                logger().log(ILog::LogLevel::ERROR, "Connection failed");
                // Notify the user that the connection failed

                // Restart WiFi
                proc->getWifiCpx().stop();
                proc->setProgramState(ProgramState::AP_STA_MODE);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(taskDelay));
    }
}

// todo wifi event manager task

// void wifiEventManagerTask()
// {
//     for (;;)
//     {
//         // Wait for the event bits to be set
//         EventBits_t uxBits = xEventGroupWaitBits(wifiEventGroup, WIFI_CONFIG_WAITING_FOR_CREDENTIALS | WIFI_CONFIG_TRY_CONNECT, pdTRUE, pdFALSE, portMAX_DELAY);

//         // Check if the event bits are set
//         if (uxBits & WIFI_CONFIG_WAITING_FOR_CREDENTIALS)
//         {
//             // Set the bit to indicate that credentials are stored
//             xEventGroupSetBits(wifiEventGroup, WIFI_CONFIG_CREDENTIALS_STORED_BIT);
//         }

//         if (uxBits & WIFI_CONFIG_TRY_CONNECT)
//         {
//             // Set the bit to indicate that the device is trying to connect to the network
//             xEventGroupSetBits(wifiEventGroup, WIFI_CONFIG_TRY_CONNECT);
//         }
//     }
// }