/** @file       OtaNotificationBridge.hpp
 *  @brief      Forwards OTA state transitions to NotificationManager as framework notifications.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 */

#pragma once

#include "PAL/Protocols/OTA/IOtaService.hpp"

class NotificationManager;

/**
 * @class OtaNotificationBridge
 * @brief Translates OtaState transitions into the OTA_UPDATING STATE notification.
 *
 * Register a lambda that forwards to onOtaState() with OtaManager::setStateObserver().
 * The bridge must outlive the observer registration.
 *
 * @note Thread-Safety: onOtaState() runs in the OTA worker task. It only calls NotificationManager::publishState().
 */
class OtaNotificationBridge
{
public:
    /**
     * @brief Construct the bridge.
     * @param[in] notificationManager Target hub. Must outlive the OTA observer registration.
     */
    explicit OtaNotificationBridge(NotificationManager& notificationManager);

    /**
     * @brief Publish the OTA_UPDATING state that corresponds to an OTA state transition.
     * @param[in] state Current OTA state reported by OtaManager.
     */
    void onOtaState(OtaState state);

private:
    NotificationManager& _notificationManager;
};
