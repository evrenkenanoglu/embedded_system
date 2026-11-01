/** @file       StatusLedNotificationSubscriber.cpp
 *  @brief      Maps notifications to status indicator states.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

/** INCLUDES ******************************************************************/
#include "StatusLedNotificationSubscriber.hpp"

#include "Process/Peripheral/Proc_StatusIndicator.hpp"
#include "System/LogHandler.h"

/** LOCAL FUNCTIONS ***********************************************************/
namespace
{
    bool toIndicationState(notification_id_t id, indication_state_t& state)
    {
        switch (id)
        {
            case static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING):
            case static_cast<notification_id_t>(framework_notification_id_t::WIFI_LINK_DISCONNECTED):
                state = indication_state_t::WIFI_CONNECTING;
                return true;

            case static_cast<notification_id_t>(framework_notification_id_t::WIFI_PROVISIONING_MODE):
                state = indication_state_t::AP_PROVISIONING;
                return true;

            case static_cast<notification_id_t>(framework_notification_id_t::MATTER_COMMISSIONING_WINDOW_OPEN):
            case static_cast<notification_id_t>(framework_notification_id_t::MATTER_COMMISSIONING_IN_PROGRESS):
                state = indication_state_t::MATTER_PAIRING;
                return true;

            case static_cast<notification_id_t>(framework_notification_id_t::OTA_UPDATING):
                state = indication_state_t::OTA_UPDATING;
                return true;

            case static_cast<notification_id_t>(framework_notification_id_t::SECURITY_TAMPER_DETECTED):
            case static_cast<notification_id_t>(framework_notification_id_t::DIAG_THERMAL_TRIP_SHUTDOWN):
            case static_cast<notification_id_t>(framework_notification_id_t::DIAG_HARDWARE_FAULT):
                state = indication_state_t::CRITICAL_ERROR;
                return true;

            default:
                return false;
        }
    }
} // namespace

/** FUNCTIONS *****************************************************************/
StatusLedNotificationSubscriber::StatusLedNotificationSubscriber(Proc_StatusIndicator& statusIndicator)
    : _statusIndicator(statusIndicator)
{
}

void StatusLedNotificationSubscriber::onNotification(const user_notification_t& notification)
{
    /// The LED reflects persistent conditions only, so one-shot events are ignored.
    if (notification.kind == notification_kind_t::EVENT)
    {
        return;
    }

    /// Unmapped identifiers do not affect the indicator.
    indication_state_t state;
    if (!toIndicationState(notification.id, state))
    {
        return;
    }

    sys_error_t error = _statusIndicator.setState(state, notification.active);
    if (error != ERROR_SUCCESS)
    {
        SYS_LOG_E("Failed to update status indication (%d)", static_cast<int>(error));
    }
}
