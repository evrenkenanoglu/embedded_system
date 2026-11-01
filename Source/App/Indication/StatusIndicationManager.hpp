/** @file       StatusIndicationManager.hpp
 *  @brief      Application facade for system visual indication states.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       31/10/2026
 */

#pragma once

#include "System/system.h"

#include <cstdint>

class Proc_StatusIndicator;

/**
 * @brief Application-visible states rendered by the status indicator process.
 */
enum class system_visual_state_t : std::uint8_t
{
    CRITICAL_ERROR,
    OTA_UPDATING,
    MATTER_PAIRING,
    WIFI_CONNECTING,
    AP_PROVISIONING,
    NORMAL_IDLE
};

/**
 * @class StatusIndicationManager
 * @brief Maps application visual states to the authoritative status indicator process.
 *
 * @note Thread-Safety: This facade stores no state; non-blocking state updates and
 *       concurrency are owned by the injected process.
 */
class StatusIndicationManager
{
public:
    /**
     * @brief Construct a facade for the status indicator process.
     * @param[in] statusIndicator Process that owns active states and priority arbitration.
     */
    explicit StatusIndicationManager(Proc_StatusIndicator& statusIndicator);

    /**
     * @brief Activate or clear an application visual state.
     * @param[in] state  Visual state to update.
     * @param[in] active True to activate the state, false to clear it.
     * @return sys_error_t ERROR_SUCCESS on success, otherwise the process error or ERROR_INVALID_ARG.
     * @note This method must not be called from an ISR context.
     */
    sys_error_t setState(system_visual_state_t state, bool active);

private:
    Proc_StatusIndicator& _statusIndicator;
};
