/** @file       Proc_ThermalMonitor.hpp
 *  @brief      Periodic thermal acquisition process with EMA filtering.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "HAL/IHAL/IHal_Sensor_Temperature.h"
#include "HAL/IHAL/IHal_Sys_Wdt.h"
#include "PAL/Protocols/Thermal/IThermalMonitor.hpp"
#include "Process/Process.hpp"

#include <freertos/semphr.h>

#include <functional>
#include <vector>

/**
 * @brief Listener payload delivered on every completed acquisition cycle.
 */
typedef struct
{
    float                   temperatureC; ///< EMA-filtered temperature in degrees Celsius
    hal_temp_sensor_state_t sensorState;  ///< Electrical health reported by the HAL driver
} thermal_sample_t;

/**
 * @brief Callback signature invoked for each registered listener after every cycle.
 */
using ThermalSampleCallback = std::function<void(const thermal_sample_t& sample)>;

/**
 * @class Proc_ThermalMonitor
 * @brief FreeRTOS acquisition loop bridging the HAL sensor and the PAL engine.
 *
 * Runs at a fixed sample interval, reads the calibrated voltage from the HAL,
 * converts it to a temperature via the PAL, applies an exponential moving
 * average filter and notifies all registered listeners. Faults are forwarded
 * immediately without filtering. The optional watchdog driver is fed every
 * cycle to keep the task subscribed to task watchdog monitoring.
 */
class Proc_ThermalMonitor : public Process
{
public:
    /**
     * @brief Construct a new Proc_ThermalMonitor object.
     *
     * @param sensor       HAL temperature sensor driver (raw voltage source).
     * @param monitor      PAL temperature computation engine.
     * @param sampleIntervalMs Acquisition period in milliseconds.
     * @param emaAlpha     EMA smoothing factor in (0, 1]; 1.0 disables filtering.
     * @param watchdog     Optional watchdog driver fed every cycle.
     * @param stackSize    FreeRTOS task stack size in bytes.
     * @param taskPriority FreeRTOS task priority.
     */
    Proc_ThermalMonitor(
        IHal_Sensor_Temperature& sensor, IThermalMonitor& monitor, uint32_t sampleIntervalMs, float emaAlpha, IHal_Sys_Wdt* watchdog = nullptr, uint32_t stackSize = 4096,
        int taskPriority = tskIDLE_PRIORITY + 1);
    ~Proc_ThermalMonitor() override;

    Proc_ThermalMonitor(const Proc_ThermalMonitor&)            = delete;
    Proc_ThermalMonitor& operator=(const Proc_ThermalMonitor&) = delete;
    Proc_ThermalMonitor(Proc_ThermalMonitor&&)                 = delete;
    Proc_ThermalMonitor& operator=(Proc_ThermalMonitor&&)      = delete;

    sys_error_t start() override;
    sys_error_t stop() override;
    sys_error_t pause() override;
    sys_error_t resume() override;

    /**
     * @brief Registers a listener invoked with every filtered sample or fault.
     *
     * @param callback Callable invoked from the acquisition task context.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_NULL_POINTER when
     *         the callback is empty.
     */
    sys_error_t addListener(ThermalSampleCallback callback);

    /**
     * @brief Returns the most recent EMA-filtered temperature.
     *
     * @return float Latest filtered temperature in degrees Celsius.
     */
    float getLastTemperatureC() const;

private:
    static void taskEntry(void* arg);
    void        run();
    void        notifyFault();
    void        notifySample(float temperatureC);

    IHal_Sensor_Temperature&           _sensor;
    IThermalMonitor&                   _monitor;
    IHal_Sys_Wdt*                      _watchdog;
    uint32_t                           _sampleIntervalMs;
    float                              _emaAlpha;
    uint32_t                           _stackSize;
    int                                _taskPriority;
    TaskHandle_t                       _taskHandle;
    SemaphoreHandle_t                  _listenersMutex; ///< Serializes listener list access between API and task
    std::vector<ThermalSampleCallback> _listeners;
    float                              _filteredTemperatureC;
    bool                               _hasValidSample;
};
