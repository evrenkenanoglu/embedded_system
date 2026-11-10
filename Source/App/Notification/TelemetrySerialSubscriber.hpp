/** @file       TelemetrySerialSubscriber.hpp
 *  @brief      Writes notifications as CRC-protected serial telemetry records.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       10/11/2026
 */

#pragma once

#include "App/Notification/INotificationSubscriber.hpp"

#include <cstdint>

/**
 * @class TelemetrySerialSubscriber
 * @brief Emits notification records on stdout using the structured telemetry wire format.
 *
 * @note Thread-Safety: onNotification() runs in the NotificationManager dispatcher task.
 */
class TelemetrySerialSubscriber : public INotificationSubscriber
{
public:
    TelemetrySerialSubscriber() = default;

    void onNotification(const user_notification_t& notification) override;

private:
    void _recordDrop();

    uint32_t _droppedCount = 0U;
};
