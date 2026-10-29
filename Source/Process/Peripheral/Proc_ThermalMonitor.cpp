/** @file       Proc_ThermalMonitor.cpp
 *  @brief      Periodic thermal acquisition process with EMA filtering.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#define ENABLE_SYS_LOG_D

/** INCLUDES ******************************************************************/
#include "Proc_ThermalMonitor.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr uint32_t kListenerLockTimeoutMs = 100U; ///< Listener list lock timeout in milliseconds
} // namespace

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/
Proc_ThermalMonitor::Proc_ThermalMonitor(
    IHal_Sensor_Temperature& sensor, IThermalMonitor& monitor, uint32_t sampleIntervalMs, float emaAlpha, IHal_Sys_Wdt* watchdog, uint32_t stackSize,
    int taskPriority)
    : _sensor(sensor)                     // HAL raw voltage source
    , _monitor(monitor)                   // PAL temperature engine
    , _watchdog(watchdog)                 // Optional watchdog driver
    , _sampleIntervalMs(sampleIntervalMs) // Acquisition period
    , _emaAlpha(emaAlpha)                 // EMA smoothing factor
    , _stackSize(stackSize)               // Task stack size
    , _taskPriority(taskPriority)         // Task priority
    , _taskHandle(nullptr)                // No task yet
    , _listenersMutex(nullptr)            // No listener lock yet
    , _filteredTemperatureC(0.0F)         // No sample yet
    , _hasValidSample(false)              // No sample yet
{
    // Validate the acquisition period and filter coefficient up front.
    RETURN_IF_ERROR(sampleIntervalMs == 0U, , SYS_LOG_E("Proc_ThermalMonitor: sample interval must be non-zero"));
    RETURN_IF_ERROR(emaAlpha <= 0.0F || emaAlpha > 1.0F, , SYS_LOG_E("Proc_ThermalMonitor: EMA alpha must be in (0, 1]"));

    _listenersMutex = xSemaphoreCreateMutex();
    RETURN_IF_ERROR(_listenersMutex == nullptr, , SYS_LOG_E("Proc_ThermalMonitor: listener mutex creation failed"));

    setState(State::INITIALIZED);
}

Proc_ThermalMonitor::~Proc_ThermalMonitor()
{
    (void)stop();
}

sys_error_t Proc_ThermalMonitor::start()
{
    // Ensure process is in INITIALIZED or STOPPED state
    RETURN_IF_ERROR_WITH_LOG(
        getState() != State::INITIALIZED && getState() != State::STOPPED, // Expression
        ERROR_INVALID_STATE,                                              // Error Code
        "Process not in INITIALIZED or STOPPED state");                   // Error Message

    // Create the thermal acquisition task
    BaseType_t result = xTaskCreate(
        taskEntry,              // Task function
        "thermal_monitor_task", // Task name
        _stackSize,             // Stack size
        this,                   // Task parameter
        _taskPriority,          // Task priority
        &_taskHandle);          // Task handle

    RETURN_IF_ERROR_WITH_LOG(result != pdPASS, ERROR_FAIL, "Failed to create thermal monitor task");

    setState(State::RUNNING);
    return ERROR_SUCCESS;
}

sys_error_t Proc_ThermalMonitor::stop()
{
    RETURN_IF_ERROR_WITH_LOG(getState() != State::RUNNING && getState() != State::PAUSED, ERROR_INVALID_STATE, "Process not in RUNNING or PAUSED state");

    // Delete the acquisition task
    if (_taskHandle != nullptr)
    {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }

    // Release the listener lock; the task can no longer touch the list.
    if (_listenersMutex != nullptr)
    {
        vSemaphoreDelete(_listenersMutex);
        _listenersMutex = nullptr;
    }

    setState(State::STOPPED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_ThermalMonitor::pause()
{
    RETURN_IF_ERROR_WITH_LOG(getState() != State::RUNNING, ERROR_INVALID_STATE, "Process not in RUNNING state");

    vTaskSuspend(_taskHandle);
    setState(State::PAUSED);
    return ERROR_SUCCESS;
}

sys_error_t Proc_ThermalMonitor::resume()
{
    RETURN_IF_ERROR_WITH_LOG(getState() != State::PAUSED, ERROR_INVALID_STATE, "Process not in PAUSED state");

    vTaskResume(_taskHandle);
    setState(State::RUNNING);
    return ERROR_SUCCESS;
}

sys_error_t Proc_ThermalMonitor::addListener(ThermalSampleCallback callback)
{
    RETURN_IF_ERROR(callback == nullptr, ERROR_NULL_POINTER, SYS_LOG_E("Thermal listener callback is null"));
    RETURN_IF_ERROR(_listenersMutex == nullptr, ERROR_INVALID_STATE, SYS_LOG_E("Thermal monitor stopped; listener registration rejected"));

    // Serialize against the task loop iterating the listener list.
    if (xSemaphoreTake(_listenersMutex, pdMS_TO_TICKS(kListenerLockTimeoutMs)) != pdTRUE)
    {
        SYS_LOG_E("Thermal listener registration timed out");
        return ERROR_DEVICE_BUSY;
    }
    _listeners.push_back(std::move(callback));
    xSemaphoreGive(_listenersMutex);
    return ERROR_SUCCESS;
}

float Proc_ThermalMonitor::getLastTemperatureC() const
{
    return _filteredTemperatureC;
}

void Proc_ThermalMonitor::taskEntry(void* arg)
{
    // Validate argument
    RETURN_IF_ERROR(arg == nullptr, , SYS_LOG_E("Proc_ThermalMonitor argument is null"));

    auto* const proc = static_cast<Proc_ThermalMonitor*>(arg);
    proc->run();
}

void Proc_ThermalMonitor::notifyFault()
{
    // Forward sensor faults to listeners immediately without filtering.
    thermal_sample_t faultSample = {};
    faultSample.temperatureC     = _filteredTemperatureC;
    faultSample.sensorState      = _sensor.getSensorState();

    // Snapshot the listener list under the lock so registration during
    // acquisition cannot invalidate the iteration.
    std::vector<ThermalSampleCallback> snapshot;
    if (xSemaphoreTake(_listenersMutex, pdMS_TO_TICKS(kListenerLockTimeoutMs)) == pdTRUE)
    {
        snapshot = _listeners;
        xSemaphoreGive(_listenersMutex);
    }

    for (const auto& listener : snapshot)
    {
        listener(faultSample);
    }
}

void Proc_ThermalMonitor::notifySample(float temperatureC)
{
    // Notify all registered listeners with the filtered sample.
    thermal_sample_t sample = {};
    sample.temperatureC     = temperatureC;
    sample.sensorState      = _sensor.getSensorState();

    // Snapshot the listener list under the lock so registration during
    // acquisition cannot invalidate the iteration.
    std::vector<ThermalSampleCallback> snapshot;
    if (xSemaphoreTake(_listenersMutex, pdMS_TO_TICKS(kListenerLockTimeoutMs)) == pdTRUE)
    {
        snapshot = _listeners;
        xSemaphoreGive(_listenersMutex);
    }

    for (const auto& listener : snapshot)
    {
        listener(sample);
    }
}

void Proc_ThermalMonitor::run()
{
    // Subscribe this task to watchdog monitoring when a driver is available.
    if (_watchdog != nullptr)
    {
        RETURN_IF_ERROR(_watchdog->registerCurrentTask(), , SYS_LOG_E("Failed to register thermal task with watchdog"));
    }

    // Initialize the periodic delay anchor so the first cycle runs immediately.
    TickType_t       lastWakeTime = xTaskGetTickCount();
    const TickType_t periodTicks  = pdMS_TO_TICKS(_sampleIntervalMs);

    for (;;)
    {
        // Block deterministically until the next acquisition instant.
        vTaskDelayUntil(&lastWakeTime, periodTicks);

        // Read the calibrated voltage from the HAL sensor.
        uint32_t    voltageMv = 0U;
        sys_error_t err       = _sensor.readRawVoltageMv(voltageMv);

        if (err != ERROR_SUCCESS)
        {
            notifyFault();

            // Feed the watchdog even during fault cycles to avoid spurious resets.
            if (_watchdog != nullptr)
            {
                (void)_watchdog->feed();
            }
            continue;
        }

        // Convert the calibrated voltage into a physical temperature.
        float temperatureC = 0.0F;
        err                = _monitor.voltageToTemperature(voltageMv, temperatureC);
        if (err != ERROR_SUCCESS)
        {
            SYS_LOG_E("Thermal conversion failed for %u mV (err: %d)", (unsigned)voltageMv, (int)err);
            if (_watchdog != nullptr)
            {
                (void)_watchdog->feed();
            }
            continue;
        }

        // Apply the exponential moving average filter:
        // T_filtered = alpha * T_current + (1 - alpha) * T_previous
        if (_hasValidSample)
        {
            _filteredTemperatureC = (_emaAlpha * temperatureC) + ((1.0F - _emaAlpha) * _filteredTemperatureC);
        }
        else
        {
            _filteredTemperatureC = temperatureC;
            _hasValidSample       = true;
        }

        notifySample(_filteredTemperatureC);

        // Kick the watchdog to prove the loop is alive.
        if (_watchdog != nullptr)
        {
            (void)_watchdog->feed();
        }
    }
}
