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

constexpr uint16_t staReadyTimeout = 10000; // milliseconds
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

/**
 * @brief Get the WiFi credentials from the memory
 *
 * @param memDevice Memory device object
 * @param ssid SSID
 * @param password Password
 * @return sys_error_t
 */
static sys_error_t getWifiCredentialsfromMem(IHAL_MEM& memDevice, uint8_t* ssid, uint8_t* password);

Proc_wifiConfigurationManager::Proc_wifiConfigurationManager(cpx_wifi& wifiCpx, wifi_config_t& wifiConfig, IHAL_MEM& memDevice)
    : _wifiCpx(wifiCpx)
    , _wifiConfig(wifiConfig)
    , _xHandleProgram(nullptr)
    , _xHandleWifiEventHandler(nullptr)
    , _xHandleWifiConfigEventHandler(nullptr)
    , _programState(ProgramState::UNINITIALIZED)
    , _wifiConfigEventGroup(xEventGroupCreate())
    , _wifiConfigScanResults(xQueueCreate(WIFI_SCAN_MAX_RECORDS, sizeof(wifiApRecord_t)))
    , _memDevice(memDevice)
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
                                    this,                        // Task parameters
                                    programRoutineTaskPriority,  // Task priority
                                    &_xHandleProgram);           // Task handle
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

IHAL_MEM& Proc_wifiConfigurationManager::getMemDevice() const
{
    return _memDevice;
}

wifi_config_t& Proc_wifiConfigurationManager::getWifiConfig() const
{
    return _wifiConfig;
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

                proc->setProgramState(ProgramState::TRY_ACCESS_CREDENTIALS);
            }
            break;

            case ProgramState::TRY_ACCESS_CREDENTIALS:
            {
                logger().log(ILog::LogLevel::INFO, "Trying to access credentials...");

                // Try to access the credentials
                uint8_t ssid[WIFI_SSID_LENGTH];
                uint8_t password[WIFI_PASSWORD_LENGTH];

                // Read the SSID and password from NVS
                sys_error_t error = getWifiCredentialsfromMem(proc->getMemDevice(), ssid, password);

                if (error != ERROR_SUCCESS)
                {
                    // If the credentials are not found, change the state to CREDENTIALS_NOT_FOUND_OR_INVALID
                    proc->setProgramState(ProgramState::CREDENTIALS_NOT_FOUND_OR_INVALID);
                }
                else
                {

                    // // Stop WiFi if it is already started
                    // proc->getWifiCpx().stop();

                    // wifi_config_t wifi_config = {};
                    // strncpy((char*)wifi_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID, sizeof(wifi_config.sta.ssid));
                    // strncpy((char*)wifi_config.sta.password, EXAMPLE_ESP_WIFI_PASS, sizeof(wifi_config.sta.password));
                    // wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

                    // Set the SSID and password to the wifi configuration
                    // wifi_config_t* config = static_cast<wifi_config_t*>(proc->getWifiCpx().get());
                    wifi_config_t config = {};
                    memcpy(config.sta.ssid, ssid, sizeof(config.sta.ssid));
                    memcpy(config.sta.password, password, sizeof(config.sta.password));
                    config.sta.threshold.authmode = WIFI_AUTH_OPEN;

                    std::stringstream ss("");
                    ss << "SSID: " << config.sta.ssid << std::endl;
                    ss << "Password: " << config.sta.password << std::endl;
                    logger().log(ILog::LogLevel::INFO, ss.str());

                    proc->getWifiCpx().set(&config);

                    // If the credentials are found, change the state to TRY_CONNECT
                    proc->setProgramState(ProgramState::STA_MODE);
                }
            }

            break;

            case ProgramState::TRY_CONNECT:
            {
                logger().log(ILog::LogLevel::INFO, "Trying to connect to the network...");
                for (uint8_t i = 0; i < tryConnectCount; i++)
                {
                    // Try to connect to the network
                    if (proc->getWifiCpx().connect())
                    {
                        // If connection fails, change the state to DISCONNECTED

                        if (i == tryConnectCount - 1) // If the connection fails after the last try, change the state to DISCONNECTED
                                                      // proc->setProgramState(ProgramState::CREDENTIALS_NOT_FOUND_OR_INVALID);
                        {
                            logger().log(ILog::LogLevel::ERROR, "Connection failed");
                        }
                        else
                            continue;
                    }

                    // wait until WIFI_CONFIG_CONNECTED_TO_AP or WIFI_CONFIG_DISCONNECTED_FROM_AP event is set
                    EventBits_t bits = xEventGroupWaitBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_CONNECTED_TO_AP | WIFI_CONFIG_DISCONNECTED_FROM_AP, pdTRUE, pdFALSE, portMAX_DELAY);

                    if (bits & WIFI_CONFIG_CONNECTED_TO_AP)
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

                        break;
                    }
                    else if (bits & WIFI_CONFIG_DISCONNECTED_FROM_AP)
                    {
                        if (i == tryConnectCount - 1) // If the connection fails after the last try, change the state to DISCONNECTED
                                                      // proc->setProgramState(ProgramState::CREDENTIALS_NOT_FOUND_OR_INVALID);
                        {
                            logger().log(ILog::LogLevel::ERROR, "Connection failed");
                            proc->setProgramState(ProgramState::CREDENTIALS_NOT_FOUND_OR_INVALID);
                        }
                        else
                            continue;
                    }

                    // wait for a while before trying to connect again
                    std::this_thread::sleep_for(std::chrono::milliseconds(tryConnectDelay));
                }
            }
            break;

            case ProgramState::CREDENTIALS_NOT_FOUND_OR_INVALID:
            {
                // Notify the user that the credentials are not found or invalid
                logger().log(ILog::LogLevel::WARNING, "Credentials not found or invalid");

                if (proc->getWifiCpx().getWifiMode() == WIFI_MODE_STA)
                {
                    // If the WiFi mode is STA
                    // Stop WiFi
                    std::cout << "WIFI CONFIG PROGRAM: Stopping WiFi..." << std::endl;
                    proc->getWifiCpx().stop();
                }

                std::cout << "WIFI CONFIG PROGRAM: Changing state..." << std::endl;
                // Start AP-STA mode
                proc->setProgramState(ProgramState::AP_STA_MODE);
            }
            break;

            case ProgramState::AP_STA_MODE:
            {
                logger().log(ILog::LogLevel::INFO, "Starting AP-STA mode...");

                // Stop WiFi if it is already started
                proc->getWifiCpx().stop();

                // Set the SSID and password to the wifi configuration for AP mode
                proc->getWifiCpx().set(&proc->getWifiConfig());

                // Set WiFi mode as AP-STA mode
                proc->getWifiCpx().setWifiMode(WIFI_MODE_APSTA);

                // Start WiFi
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

                if (proc->getWifiCpx().start() != ERROR_SUCCESS)
                {
                    // If the WiFi fails to start, change the state to CONNECTION_FAILED
                    proc->setProgramState(ProgramState::CONNECTION_FAILED);
                }

                // Wait until the station is started with staReadyTimeout
                EventBits_t bits = xEventGroupWaitBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_STA_SETUP_READY, pdTRUE, pdFALSE, staReadyTimeout / portTICK_PERIOD_MS);

                if (bits & WIFI_CONFIG_STA_SETUP_READY)
                {
                    // If the station is started, change the state to CONNECTED
                    proc->setProgramState(ProgramState::CONNECTED);
                }
                else
                {
                    // If the station is not started and timeout occured, change the state to CONNECTION_FAILED
                    proc->setProgramState(ProgramState::CONNECTION_FAILED);
                }
                // Change the state to TRY_CONNECT
                proc->setProgramState(ProgramState::TRY_CONNECT);
            }
            break;

            case ProgramState::AWAITING_CREDENTIALS:
            {
                logger().log(ILog::LogLevel::INFO, "Awaiting credentials...");
                // Wait until user enters the SSID and password
                xEventGroupWaitBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_CREDENTIALS_STORED, pdTRUE, pdFALSE, portMAX_DELAY);

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
                // proc->setProgramState(ProgramState::STA_MODE);

                EventBits_t bits = xEventGroupWaitBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_DISCONNECTED_FROM_AP, pdTRUE, pdFALSE, portMAX_DELAY);

                if (bits & WIFI_CONFIG_DISCONNECTED_FROM_AP)
                {
                    // If the station is disconnected, change the state to DISCONNECTED
                    proc->setProgramState(ProgramState::DISCONNECTED);
                }
            }
            break;

            case ProgramState::RESTART:
            {
                logger().log(ILog::LogLevel::INFO, "Restarting WiFi...");
                // Notify the user that the WiFi is restarting

                // Restart WiFi
                std::cout << "DEBUG: STOPPING WIFI" << std::endl;
                proc->getWifiCpx().stop();
                std::cout << "DEBUG: SETTING WIFI MODE TO STA" << std::endl;
                proc->getWifiCpx().setWifiMode(WIFI_MODE_STA);
                std::cout << "DEBUG: STARTING WIFI" << std::endl;
                proc->getWifiCpx().start();

                logger().log(ILog::LogLevel::INFO, "WIFI CONFIG PROGRAM: Changing state to INITIALIZED...");
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
                                                    WIFI_SCAN_DONE |                        //
                                                        WIFI_STA_STARTED |                  //
                                                        WIFI_CONNECTED |                    //
                                                        WIFI_DISCONNECTED,                  //
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
        else if (eventBits & WIFI_STA_STARTED)
        {
            // Set the bit to indicate that the station is started
            xEventGroupSetBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_STA_SETUP_READY);
        }
        else if (eventBits & WIFI_CONNECTED)
        {
            // Set the bit to indicate that the station is connected
            xEventGroupSetBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_CONNECTED_TO_AP);
        }
        else if (eventBits & WIFI_DISCONNECTED)
        {
            // Set the bit to indicate that the station is disconnected
            xEventGroupSetBits(proc->getWifiConfigEventGroup(), WIFI_CONFIG_DISCONNECTED_FROM_AP);
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
        EventBits_t eventBits = xEventGroupWaitBits(proc->getWifiConfigEventGroup(), // Event Group Handle
                                                                                     // Bits to wait for
                                                    WIFI_CONFIG_CREDENTIALS_STORED | //
                                                        WIFI_CONFIG_SCAN_REQUESTED,  //
                                                    pdTRUE,                          // Clear bits on exit
                                                    pdFALSE,                         // Wait for all bit
                                                    portMAX_DELAY);                  // Wait indefinitely

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
        else if (eventBits & WIFI_CONFIG_CREDENTIALS_STORED)
        {
            if (proc->getProgramState() == ProgramState::AWAITING_CREDENTIALS)
            {
                // When the credentials are stored, set program state to TRY_ACCESS_CREDENTIALS
                proc->setProgramState(ProgramState::TRY_ACCESS_CREDENTIALS);
            }
        }
    }
}

sys_error_t getWifiCredentialsfromMem(IHAL_MEM& memDevice, uint8_t* ssid, uint8_t* password)
{
    std::cout << "Checking for SSID and password in memory..." << std::endl;
    RETURN_ON_ERROR(memDevice.readData((void*)WIFI_SSID, ssid, WIFI_PASSWORD_LENGTH));
    RETURN_ON_ERROR(memDevice.readData((void*)WIFI_PASSWORD, password, WIFI_PASSWORD_LENGTH));

    if (strlen((char*)ssid) == 0 || strlen((char*)password) == 0)
    {
        printf("SSID or password not found\n");
        return ERROR_FAIL;
    }

    std::stringstream ss;
    ss << "SSID: " << ssid << std::endl;
    ss << "Password: " << password << std::endl;

    // Log SSID and Password
    logger().log(ILog::LogLevel::INFO, ss.str());
    return ERROR_SUCCESS;
}