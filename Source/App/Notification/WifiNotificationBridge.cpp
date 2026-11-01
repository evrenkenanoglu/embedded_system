/** @file       WifiNotificationBridge.cpp
 *  @brief      Forwards Wi-Fi manager events to NotificationManager as framework notifications.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

/** INCLUDES ******************************************************************/
#include "App/Notification/WifiNotificationBridge.hpp"

#include "App/Notification/NotificationManager.hpp"
#include "System/LogHandler.h"

/** LOCAL FUNCTIONS ***********************************************************/
namespace
{
    void publishState(NotificationManager& notificationManager, notification_id_t id, bool active, notification_severity_t severity)
    {
        user_notification_t notification{};
        notification.id          = id;
        notification.category    = notification_category_t::CONNECTIVITY;
        notification.kind        = notification_kind_t::STATE;
        notification.severity    = severity;
        notification.active      = active;
        notification.sequence    = 0U;
        notification.payload.u32 = 0U;

        sys_error_t err = notificationManager.publish(notification);
        if (err != ERROR_SUCCESS)
        {
            SYS_LOG_E("WiFi notification bridge: publish failed (err: %d)", static_cast<int>(err));
        }
    }
} // namespace

/** FUNCTIONS *****************************************************************/
WifiNotificationBridge::WifiNotificationBridge(NotificationManager& notificationManager)
    : _notificationManager(notificationManager)
{
}

void WifiNotificationBridge::onWifiEvent(WifiEvent event, const WifiMgrStatus& status, void* userData)
{
    (void)status;

    WifiNotificationBridge* bridge = static_cast<WifiNotificationBridge*>(userData);
    if (bridge == nullptr)
    {
        return;
    }

    bridge->_forward(event);
}

void WifiNotificationBridge::_forward(WifiEvent event)
{
    /// Publish the framework STATE transitions for the event, in the order listed in the design.
    switch (event)
    {
        case WifiEvent::AP_SETUP_READY:
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_PROVISIONING_MODE), true, notification_severity_t::INFO);
            break;

        case WifiEvent::AP_SETUP_FINISH:
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_PROVISIONING_MODE), false, notification_severity_t::INFO);
            break;

        case WifiEvent::STA_SETUP_READY:
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING), true, notification_severity_t::INFO);
            break;

        case WifiEvent::STA_SETUP_FINISH:
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING), false, notification_severity_t::INFO);
            break;

        case WifiEvent::DISCONNECTED:
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTED_STA), false, notification_severity_t::INFO);
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_LINK_DISCONNECTED), true, notification_severity_t::WARNING);
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING), true, notification_severity_t::WARNING);
            break;

        case WifiEvent::CONNECTED:
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING), false, notification_severity_t::INFO);
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_LINK_DISCONNECTED), false, notification_severity_t::INFO);
            publishState(_notificationManager, static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTED_STA), true, notification_severity_t::INFO);
            break;

        default:
            SYS_LOG_E("WiFi notification bridge: unknown event (%d)", static_cast<int>(event));
            break;
    }
}
