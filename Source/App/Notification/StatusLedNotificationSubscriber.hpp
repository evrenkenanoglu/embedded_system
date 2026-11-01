/** @file       StatusLedNotificationSubscriber.hpp
 *  @brief      Maps notifications to status indicator states.
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

class Proc_StatusIndicator;

/**
 * @class StatusLedNotificationSubscriber
 * @brief Drives the status indicator from notifications. Does not touch GPIO directly.
 *
 * @note Thread-Safety: onNotification() runs in the NotificationManager dispatcher task.
 */
class StatusLedNotificationSubscriber : public INotificationSubscriber
{
public:
    /**
     * @brief Construct the subscriber.
     * @param[in] statusIndicator Process that owns the status GPIO. Must outlive this object.
     */
    explicit StatusLedNotificationSubscriber(Proc_StatusIndicator& statusIndicator);

    void onNotification(const user_notification_t& notification) override;

private:
    Proc_StatusIndicator& _statusIndicator;
};
