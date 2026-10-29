/** @file       ThermalProtectionManager.cpp
 *  @brief      Thermal safety state machine with emergency relay shutoff.
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
#include "ThermalProtectionManager.hpp"

#include "System/LogHandler.h"
#include "System/errorTranslateHandler.h"
#include "System/error_macros.h"

/** CONSTANTS *****************************************************************/

/** TYPEDEFS ******************************************************************/

/** MACROS ********************************************************************/

/** VARIABLES *****************************************************************/

/** LOCAL FUNCTIONS ***********************************************************/

/** FUNCTIONS *****************************************************************/
ThermalProtectionManager::ThermalProtectionManager(
    Proc_ThermalMonitor& monitor, RelayTripCallback relayTrip, float warningTemperatureC, float shutdownTemperatureC,
    float recoveryHysteresisC)
    : _monitor(monitor)                           // Sample source
    , _relayTrip(std::move(relayTrip))            // Emergency relay action
    , _warningTemperatureC(warningTemperatureC)   // Warning threshold
    , _shutdownTemperatureC(shutdownTemperatureC) // Shutdown threshold
    , _recoveryHysteresisC(recoveryHysteresisC)   // Recovery hysteresis
    , _state(thermal_protection_state_t::NOMINAL) // Start in the safe nominal state
    , _lastTemperatureC(0.0F)                     // No sample yet
    , _relayTripped(false)                        // Relay starts closed
{
}

ThermalProtectionManager::~ThermalProtectionManager() = default;

sys_error_t ThermalProtectionManager::start()
{
    /// Validate the relay trip callback before arming the protection.
    RETURN_IF_ERROR(_relayTrip == nullptr, ERROR_NULL_POINTER, SYS_LOG_E("Thermal relay trip callback is null"));

    /// Validate the threshold ordering for a meaningful state machine.
    RETURN_IF_ERROR(_warningTemperatureC >= _shutdownTemperatureC, ERROR_INVALID_CONFIG, SYS_LOG_E("Thermal warning threshold must be below shutdown threshold"));
    RETURN_IF_ERROR(_recoveryHysteresisC <= 0.0F, ERROR_INVALID_CONFIG, SYS_LOG_E("Thermal recovery hysteresis must be positive"));

    /// Subscribe to the acquisition process samples.
    RETURN_ON_ERROR(_monitor.addListener([this](const thermal_sample_t& sample) { onSample(sample); }), SYS_LOG_E("Failed to subscribe thermal protection listener"));

    SYS_LOG_I(
        "ThermalProtectionManager armed (warn=%.1fC, shutdown=%.1fC, hyst=%.1fC)", (double)_warningTemperatureC, (double)_shutdownTemperatureC, (double)_recoveryHysteresisC);
    return ERROR_SUCCESS;
}

thermal_protection_state_t ThermalProtectionManager::getState() const
{
    return _state;
}

float ThermalProtectionManager::getLastTemperatureC() const
{
    return _lastTemperatureC;
}

void ThermalProtectionManager::onSample(const thermal_sample_t& sample)
{
    /// A sensor fault immediately forces the fail-safe state.
    if (sample.sensorState != hal_temp_sensor_state_t::OK)
    {
        SYS_LOG_E("Thermal sensor fault (state: %d); forcing emergency shutoff", (int)sample.sensorState);
        _lastTemperatureC = sample.temperatureC;
        tripRelay();
        _state = thermal_protection_state_t::OVERHEAT_SHUTDOWN;
        return;
    }

    _lastTemperatureC = sample.temperatureC;

    /// Evaluate the safety state machine with hysteresis-gated recovery.
    switch (_state)
    {
        case thermal_protection_state_t::NOMINAL:
        {
            if (sample.temperatureC >= _shutdownTemperatureC)
            {
                SYS_LOG_E("Over-temperature shutdown: %.1fC >= %.1fC", (double)sample.temperatureC, (double)_shutdownTemperatureC);
                tripRelay();
                _state = thermal_protection_state_t::OVERHEAT_SHUTDOWN;
            }
            else if (sample.temperatureC >= _warningTemperatureC)
            {
                SYS_LOG_W("Thermal warning: %.1fC >= %.1fC", (double)sample.temperatureC, (double)_warningTemperatureC);
                _state = thermal_protection_state_t::WARNING;
            }
        }
        break;

        case thermal_protection_state_t::WARNING:
        {
            if (sample.temperatureC >= _shutdownTemperatureC)
            {
                SYS_LOG_E("Over-temperature shutdown: %.1fC >= %.1fC", (double)sample.temperatureC, (double)_shutdownTemperatureC);
                tripRelay();
                _state = thermal_protection_state_t::OVERHEAT_SHUTDOWN;
            }
            else if (sample.temperatureC < _warningTemperatureC)
            {
                SYS_LOG_I("Thermal condition cleared: %.1fC", (double)sample.temperatureC);
                _state = thermal_protection_state_t::NOMINAL;
            }
        }
        break;

        case thermal_protection_state_t::OVERHEAT_SHUTDOWN:
        {
            /// The relay may only re-close below the recovery threshold.
            const float recoveryTemperatureC = _shutdownTemperatureC - _recoveryHysteresisC;
            if (sample.temperatureC < recoveryTemperatureC)
            {
                SYS_LOG_I("Thermal recovery: %.1fC < %.1fC; releasing relay", (double)sample.temperatureC, (double)recoveryTemperatureC);
                releaseRelay();
                _state = thermal_protection_state_t::NOMINAL;
            }
        }
        break;

        default:
            _state = thermal_protection_state_t::OVERHEAT_SHUTDOWN;
            tripRelay();
            break;
    }
}

void ThermalProtectionManager::tripRelay()
{
    /// Avoid issuing a redundant trip command.
    if (_relayTripped)
    {
        return;
    }

    RETURN_IF_ERROR(_relayTrip == nullptr, , SYS_LOG_E("Thermal relay trip callback is null"));

    _relayTrip(true);
    _relayTripped = true;
    SYS_LOG_E("EMERGENCY: relay tripped due to thermal condition");
}

void ThermalProtectionManager::releaseRelay()
{
    /// Avoid issuing a redundant release command.
    if (!_relayTripped)
    {
        return;
    }

    RETURN_IF_ERROR(_relayTrip == nullptr, , SYS_LOG_E("Thermal relay release callback is null"));

    _relayTrip(false);
    _relayTripped = false;
    SYS_LOG_I("Thermal relay released after recovery");
}
