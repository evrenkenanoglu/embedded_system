/** @file       SerialNotificationSubscriber.hpp
 *  @brief      Writes notifications to the serial log.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

#pragma once

#include "App/Notification/INotificationSubscriber.hpp"

/**
 * @class SerialNotificationSubscriber
 * @brief Logs notification id, category, state, and numeric payload. Never logs raw payload bytes as text.
 *
 * @note Thread-Safety: onNotification() runs in the NotificationManager dispatcher task.
 */
class SerialNotificationSubscriber : public INotificationSubscriber
{
public:
    SerialNotificationSubscriber() = default;

    void onNotification(const user_notification_t& notification) override;
};
