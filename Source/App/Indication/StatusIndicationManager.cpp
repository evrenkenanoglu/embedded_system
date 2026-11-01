/** @file       StatusIndicationManager.cpp
 *  @brief      Application facade implementation for status indication states.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       31/10/2026
 */

/** INCLUDES ******************************************************************/
#include "StatusIndicationManager.hpp"

#include "Process/Peripheral/Proc_StatusIndicator.hpp"
#include "System/error_macros.h"

/** LOCAL FUNCTIONS ***********************************************************/
namespace
{
    bool toProcessState(system_visual_state_t state, indication_state_t& processState)
    {
        switch (state)
        {
            case system_visual_state_t::CRITICAL_ERROR:
                processState = indication_state_t::CRITICAL_ERROR;
                return true;

            case system_visual_state_t::OTA_UPDATING:
                processState = indication_state_t::OTA_UPDATING;
                return true;

            case system_visual_state_t::MATTER_PAIRING:
                processState = indication_state_t::MATTER_PAIRING;
                return true;

            case system_visual_state_t::WIFI_CONNECTING:
                processState = indication_state_t::WIFI_CONNECTING;
                return true;

            case system_visual_state_t::AP_PROVISIONING:
                processState = indication_state_t::AP_PROVISIONING;
                return true;

            case system_visual_state_t::NORMAL_IDLE:
                processState = indication_state_t::NORMAL_IDLE;
                return true;

            default:
                return false;
        }
    }
} // namespace

/** FUNCTIONS *****************************************************************/
StatusIndicationManager::StatusIndicationManager(Proc_StatusIndicator& statusIndicator)
    : _statusIndicator(statusIndicator)
{
}

sys_error_t StatusIndicationManager::setState(system_visual_state_t state, bool active)
{
    indication_state_t processState;
    RETURN_IF_ERROR((!toProcessState(state, processState)), ERROR_INVALID_ARG);
    RETURN_ON_ERROR((_statusIndicator.setState(processState, active)));

    return ERROR_SUCCESS;
}
