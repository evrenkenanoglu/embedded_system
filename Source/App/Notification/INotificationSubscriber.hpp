/** @file       INotificationSubscriber.hpp
 *  @brief      Abstract subscriber interface for user notification dispatch.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

#pragma once

#include "App/Notification/NotificationTypes.hpp"

/**
 * @class INotificationSubscriber
 * @brief Abstract sink for user notifications.
 *
 * @note Thread-Safety: Callbacks run in the NotificationManager dispatcher task.
 *       Implementations must not block. Slow work (network, flash) must be
 *       handed to the subscriber's own task or queue.
 */
class INotificationSubscriber
{
public:
    virtual ~INotificationSubscriber() = default;

    /**
     * @brief Deliver one notification or state snapshot entry.
     * @param[in] notification Copy of the notification; valid only for the call.
     * @warning Must not call NotificationManager::subscribe, unsubscribe, or publish.
     */
    virtual void onNotification(const user_notification_t& notification) = 0;
};
