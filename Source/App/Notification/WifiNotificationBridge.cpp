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
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_PROVISIONING_MODE),
                notification_category_t::CONNECTIVITY,
                notification_severity_t::INFO,
                true);
            break;

        case WifiEvent::AP_SETUP_FINISH:
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_PROVISIONING_MODE),
                notification_category_t::CONNECTIVITY,
                notification_severity_t::INFO,
                false);
            break;

        case WifiEvent::STA_SETUP_READY:
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING), notification_category_t::CONNECTIVITY, notification_severity_t::INFO, true);
            break;

        case WifiEvent::STA_SETUP_FINISH:
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING), notification_category_t::CONNECTIVITY, notification_severity_t::INFO, false);
            break;

        case WifiEvent::DISCONNECTED:
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTED_STA), notification_category_t::CONNECTIVITY, notification_severity_t::INFO, false);
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_LINK_DISCONNECTED),
                notification_category_t::CONNECTIVITY,
                notification_severity_t::WARNING,
                true);
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING), notification_category_t::CONNECTIVITY, notification_severity_t::WARNING, true);
            break;

        case WifiEvent::CONNECTED:
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTING), notification_category_t::CONNECTIVITY, notification_severity_t::INFO, false);
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_LINK_DISCONNECTED),
                notification_category_t::CONNECTIVITY,
                notification_severity_t::INFO,
                false);
            _notificationManager.publishState(
                static_cast<notification_id_t>(framework_notification_id_t::WIFI_CONNECTED_STA), notification_category_t::CONNECTIVITY, notification_severity_t::INFO, true);
            break;

        default:
            SYS_LOG_E("WiFi notification bridge: unknown event (%d)", static_cast<int>(event));
            break;
    }
}
