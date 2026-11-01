/** @file       WifiNotificationBridge.hpp
 *  @brief      Forwards Wi-Fi manager events to NotificationManager as framework notifications.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

#pragma once

#include "App/Network/WifiConfigurationManager.hpp"

class NotificationManager;

/**
 * @class WifiNotificationBridge
 * @brief Translates WifiEvent callbacks into WIFI_* STATE notifications.
 *
 * Register onWifiEvent() with WifiConfigurationManager::subscribeEvent(), passing this object as user data.
 *
 * @note Thread-Safety: onWifiEvent() runs in the Wi-Fi manager worker task. It only calls NotificationManager::publish().
 */
class WifiNotificationBridge
{
public:
    /**
     * @brief Construct the bridge.
     * @param[in] notificationManager Target hub. Must outlive the Wi-Fi subscription.
     */
    explicit WifiNotificationBridge(NotificationManager& notificationManager);

    /**
     * @brief WifiEventCb trampoline. userData must be a WifiNotificationBridge pointer.
     */
    static void onWifiEvent(WifiEvent event, const WifiMgrStatus& status, void* userData);

private:
    void _forward(WifiEvent event);

    NotificationManager& _notificationManager;
};
