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
constexpr uint16_t programRoutineTaskDelay     = 1000; // milliseconds
constexpr uint16_t programRoutineTaskStackSize = 4096; // bytes
constexpr uint8_t  programRoutineTaskPriority  = 5;
constexpr char     programRoutineTaskName[]    = "programRoutineTask";

constexpr uint16_t wifiEventHandlerStackSize = 4096; // bytes
constexpr uint8_t  wifiEventHandlerPriority  = 5;
constexpr char     wifiEventHandlerName[]    = "wifiEventHandler";

constexpr uint16_t wifiConfigEventHandlerStackSize = 4096; // bytes
constexpr uint8_t  wifiConfigEventHandlerPriority  = 5;
constexpr char     wifiConfigEventHandlerName[]    = "wifiConfigEventHandler";

constexpr uint8_t tryConnectCount = 3;
constexpr uint8_t tryConnectDelay = 100; // milliseconds
} // namespace

/**
 * @brief Program task for the WiFi Configuration Manager
 *
 * @param pvParameters Proc_wifiConfigurationManager object
 */
static void programRoutineTask(void* pvParameters);

/**
 * @brief Wifi Event Handler
 *
 * This task handles the events related to the Wifi Cpx object
 *
 * @param pvParameters Proc_wifiConfigurationManager object
 */
static void wifiEventHandler(void* pvParameters);

/**
 * @brief Event manager task
 *
 * This task handles the events related to the Wifi Configuration Manager
 *
 * @param pvParameters Proc_wifiConfigurationManager object
 */
static void wifiConfigEventHandler(void* pvParameters);

Proc_wifiConfigurationManager::Proc_wifiConfigurationManager(cpx_wifi& wifiCpx, wifi_config_t& wifiConfig)
    : _wifiCpx(wifiCpx), _wifiConfig(wifiConfig), _xHandleProgram(nullptr), _xHandleWifiEventHandler(nullptr), _xHandleWifiConfigEventHandler(nullptr), _programState(ProgramState::UNINITIALIZED),
      _wifiConfigEventGroup(xEventGroupCreate()), _wifiConfigScanResults(xQueueCreate(WIFI_SCAN_MAX_RECORDS, sizeof(wifiApRecord_t)))
{
    wifiCpx.set(&_wifiConfig);
    setState(Process::State::INITIALIZED);
}

Proc_wifiConfigurationManager::~Proc_wifiConfigurationManager()
{
    if (_wifiConfigEventGroup != nullptr)
    {
        vEventGroupDelete(_wifiConfigEventGroup);
        _wifiConfigEventGroup = nullptr;
    }

    if (_wifiConfigScanResults != nullptr)
    {
        vQueueDelete(_wifiConfigScanResults);
        _wifiConfigScanResults = nullptr;
    }
}

sys_error_t Proc_wifiConfigurationManager::start()
{
    BaseType_t result = xTaskCreate(programRoutineTask,          // Task function
                                    programRoutineTaskName,      // Task name
                                    programRoutineTaskStackSize, // Stack size
                                    this,                 // Task parameters
                                    programRoutineTaskPriority,  // Task priority
                                    &_xHandleProgram);    // Task handle
    if (result != pdPASS)
    {
        stop();
        return ERROR_FAIL;
    }

    result = xTaskCreate(wifiEventHandler,           // Task function
                         wifiEventHandlerName,       // Task name
                         wifiEventHandlerStackSize,  // Stack size
                         this,                       // Task parameters
                         wifiEventHandlerPriority,   // Task priority
                         &_xHandleWifiEventHandler); // Task handle

    if (result != pdPASS)
    {
        stop();
        return ERROR_FAIL;
    }

    result = xTaskCreate(wifiConfigEventHandler,           // Task function
                         wifiConfigEventHandlerName,       // Task name
                         wifiConfigEventHandlerStackSize,  // Stack size
                         this,                             // Task parameters
                         wifiConfigEventHandlerPriority,   // Task priority
                         &_xHandleWifiConfigEventHandler); // Task handle

    if (result != pdPASS)
    {
        stop();
        return ERROR_FAIL;
    }

    setState(Process::State::RUNNING);
    return ERROR_SUCCESS;
}

sys_error_t Proc_wifiConfigurationManager::stop()
{
    vTaskDelete(_xHandleProgram);
    vTaskDelete(_xHandleWifiEventHandler);
    vTaskDelete(_xHandleWifiConfigEventHandler);
    setState(Process::State::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_wifiConfigurationManager::pause()
{
    vTaskSuspend(_xHandleProgram);
    vTaskSuspend(_xHandleWifiEventHandler);
    vTaskSuspend(_xHandleWifiConfigEventHandler);
    setState(Process::State::PAUSED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_wifiConfigurationManager::resume()
{
    vTaskResume(_xHandleProgram);
    vTaskResume(_xHandleWifiEventHandler);
    vTaskResume(_xHandleWifiConfigEventHandler);
    setState(Process::State::RUNNING);
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

EventGroupHandle_t& Proc_wifiConfigurationManager::getWifiConfigEventGroup()
{
    return _wifiConfigEventGroup;
}

QueueHandle_t Proc_wifiConfigurationManager::getWifiConfigScanResults()
{
    return _wifiConfigScanResults;
}

void programRoutineTask(void* pvParameters)
{
    Proc_wifiConfigurationManager* proc = static_cast<Proc_wifiConfigurationManager*>(pvParameters);

    if (proc == nullptr)
    {
        logger().log(ILog::LogLevel::ERROR, "Event manager task: Invalid parameters");
        return;
    }

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
                if (true)
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
                std::cout << "WIFI CONFIG PROGRAM: Stopping WiFi..." << std::endl;
                proc->getWifiCpx().stop();

                std::cout << "WIFI CONFIG PROGRAM: Changing state..." << std::endl;
                // Start AP-STA mode
                proc->setProgramState(ProgramState::AP_STA_MODE);
            }
            break;

            case ProgramState::AP_STA_MODE:
            {
                logger().log(ILog::LogLevel::INFO, "Starting AP-STA mode...");

                // Set WiFi mode as AP-STA mode
                proc->getWifiCpx().setWifiMode(WIFI_MODE_APSTA);
                ON_ERROR_WITH_OUTPUT(proc->getWifiCpx().start(), logger());

                // Notify other tasks that the AP is ready
                xEventGroupSetBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_AP_SETUP_READY);

                std::cout << "WIFI CONFIG PROGRAM: Changing state to AWAITING_CREDENTIALS..." << std::endl;
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
                xEventGroupWaitBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_CREDENTIALS_STORED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

                logger().log(ILog::LogLevel::INFO, "Credentials stored");
                // If user enters, try to connect to the network

                std::cout << "WIFI CONFIG PROGRAM: Changing state to TRY_CONNECT..." << std::endl;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(programRoutineTaskDelay));
    }
}

void wifiEventHandler(void* pvParameters)
{
    Proc_wifiConfigurationManager* proc = static_cast<Proc_wifiConfigurationManager*>(pvParameters);
    if (proc == nullptr)
    {
        logger().log(ILog::LogLevel::ERROR, "Event manager task: Invalid parameters");
        return;
    }

    for (;;)
    {
        std::cout << "WIFI EVENT HANDLER: Event manager waiting for events..." << std::endl;
        // Wait for the event bits to be set
        EventBits_t eventBits = xEventGroupWaitBits(proc->getWifiCpx().getWifiEventGroup(), // Event Group Handle
                                                                                            // Bits to wait for
                                                    WIFI_SCAN_DONE,                         //
                                                    pdTRUE,                                 // Clear bits on exit
                                                    pdFALSE,                                // Wait for all bit
                                                    portMAX_DELAY);                         // Wait indefinitely

        std::cout << "WIFI EVENT HANDLER: Event manager got event bits: " << (int)eventBits << std::endl;
        // Check if the event bits are set

        if (eventBits & WIFI_SCAN_DONE)
        {
            std::cout << "WIFI EVENT HANDLER: Getting wifi scan results..." << std::endl;
            // Set the bit to indicate that the scan is done
            proc->getWifiCpx().getScanResults(proc->getWifiConfigScanResults());

            // Send the scan results to the queue
            xEventGroupSetBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_SCAN_DONE);
        }
    }
}

void wifiConfigEventHandler(void* pvParameters)
{
    Proc_wifiConfigurationManager* proc = static_cast<Proc_wifiConfigurationManager*>(pvParameters);
    if (proc == nullptr)
    {
        logger().log(ILog::LogLevel::ERROR, "Event manager task: Invalid parameters");
        return;
    }

    for (;;)
    {
        std::cout << "WIFI CONFIG EVENT HANDLER: Event manager waiting for events..." << std::endl;
        // Wait for the event bits to be set
        EventBits_t eventBits = xEventGroupWaitBits(proc->getWifiConfigEventGroup(),     // Event Group Handle
                                                                                         // Bits to wait for
                                                    WIFI_CONFIG_CREDENTIALS_STORED_BIT | //
                                                        WIFI_CONFIG_SCAN_REQUESTED,      //
                                                    pdTRUE,                              // Clear bits on exit
                                                    pdFALSE,                             // Wait for all bit
                                                    portMAX_DELAY);                      // Wait indefinitely

        std::cout << "WIFI CONFIG EVENT HANDLER: Event manager got event bits: " << (int)eventBits << std::endl;
        // Check if the event bits are set
        if (eventBits & WIFI_CONFIG_SCAN_REQUESTED)
        {
            if (proc->getProgramState() == ProgramState::AWAITING_CREDENTIALS)
            {
                // Set the bit to indicate that credentials are stored
                sys_error_t error = proc->getWifiCpx().scan(nullptr);
                if (error != ERROR_SUCCESS)
                {
                    logger().log(ILog::LogLevel::ERROR, "Failed to scan for WiFi networks");
                }
            }
        }
    }
}