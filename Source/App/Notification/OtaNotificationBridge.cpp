/** @file       OtaNotificationBridge.cpp
 *  @brief      Forwards OTA state transitions to NotificationManager as framework notifications.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 */

/** INCLUDES ******************************************************************/
#include "App/Notification/OtaNotificationBridge.hpp"

#include "App/Notification/NotificationManager.hpp"

/** FUNCTIONS *****************************************************************/
OtaNotificationBridge::OtaNotificationBridge(NotificationManager& notificationManager)
    : _notificationManager(notificationManager)
{
}

void OtaNotificationBridge::onOtaState(OtaState state)
{
    const auto otaUpdatingId = static_cast<notification_id_t>(framework_notification_id_t::OTA_UPDATING);

    switch (state)
    {
        case OtaState::Downloading:
        case OtaState::Verifying:
        case OtaState::Applying:
            _notificationManager.publishState(otaUpdatingId, notification_category_t::OTA, notification_severity_t::INFO, true);
            break;

        case OtaState::Success:
        case OtaState::Idle:
            _notificationManager.publishState(otaUpdatingId, notification_category_t::OTA, notification_severity_t::INFO, false);
            break;

        case OtaState::Failed:
            _notificationManager.publishState(otaUpdatingId, notification_category_t::OTA, notification_severity_t::ERROR, false);
            break;
    }
}
