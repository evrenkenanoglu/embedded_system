/** @file       ThermalProtectionManager.hpp
 *  @brief      Thermal safety state machine with emergency relay shutoff.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       28/10/2026
 */

#pragma once

#include "Process/Peripheral/Proc_ThermalMonitor.hpp"

#include <functional>

/**
 * @brief Safety states of the thermal protection state machine.
 */
enum class thermal_protection_state_t : uint8_t
{
    NOMINAL           = 0, ///< Temperature below the warning threshold
    WARNING           = 1, ///< Temperature between warning and shutdown thresholds
    OVERHEAT_SHUTDOWN = 2  ///< Over-temperature or sensor fault; relay tripped and latched
};

/**
 * @brief Callback signature for the emergency relay trip action.
 *
 * @param[in] trip true to open the relay (disconnect the load), false to allow
 *                 re-closing after recovery.
 */
using RelayTripCallback = std::function<void(bool trip)>;

/**
 * @class ThermalProtectionManager
 * @brief Evaluates thermal thresholds and enforces fail-safe load shutoff.
 *
 * Consumes samples from Proc_ThermalMonitor and drives a three-state safety
 * machine. The over-temperature transition trips the relay immediately and
 * latches until the temperature cools below the recovery hysteresis threshold
 * (T_shutdown - T_hysteresis). Sensor faults also trip the relay: a missing
 * reading must never leave a high-power load unmonitored. The manager operates
 * entirely from the acquisition task context and is independent of network
 * connectivity.
 *
 * @note Thread-Safety: Samples update the state from the acquisition task;
 *       callers must externally synchronize concurrent getter access.
 * @note Lifetime: The manager must remain alive until its thermal monitor has
 *       stopped delivering callbacks.
 */
class ThermalProtectionManager
{
public:
    /**
     * @brief Construct a new ThermalProtectionManager object.
     *
     * @param monitor              Acquisition process providing samples.
     * @param relayTrip            Emergency relay trip callback.
     * @param warningTemperatureC  Warning threshold in degrees Celsius.
     * @param shutdownTemperatureC Shutdown threshold in degrees Celsius.
     * @param recoveryHysteresisC  Cooling hysteresis below the shutdown threshold.
     */
    ThermalProtectionManager(Proc_ThermalMonitor& monitor, RelayTripCallback relayTrip, float warningTemperatureC, float shutdownTemperatureC, float recoveryHysteresisC);
    ~ThermalProtectionManager();

    ThermalProtectionManager(const ThermalProtectionManager&)            = delete;
    ThermalProtectionManager& operator=(const ThermalProtectionManager&) = delete;
    ThermalProtectionManager(ThermalProtectionManager&&)                 = delete;
    ThermalProtectionManager& operator=(ThermalProtectionManager&&)      = delete;

    /**
     * @brief Subscribes the manager to the acquisition process.
     *
     * @return sys_error_t ERROR_SUCCESS on success, ERROR_FAIL when
     *         registration with the monitor fails.
     */
    sys_error_t start();

    /**
     * @brief Returns the current safety state.
     *
     * @return thermal_protection_state_t Current state machine state.
     */
    thermal_protection_state_t getState() const;

    /**
     * @brief Returns the last evaluated temperature.
     *
     * @return float Last filtered temperature in degrees Celsius.
     */
    float getLastTemperatureC() const;

private:
    void onSample(const thermal_sample_t& sample);
    void tripRelay();
    void releaseRelay();

    Proc_ThermalMonitor&       _monitor;
    RelayTripCallback          _relayTrip;
    float                      _warningTemperatureC;
    float                      _shutdownTemperatureC;
    float                      _recoveryHysteresisC;
    thermal_protection_state_t _state;
    float                      _lastTemperatureC;
    bool                       _relayTripped;
};
