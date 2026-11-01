/** @file       SerialNotificationSubscriber.cpp
 *  @brief      Writes notifications to the serial log.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

/** INCLUDES ******************************************************************/
#include "App/Notification/SerialNotificationSubscriber.hpp"

#include "System/LogHandler.h"

/** FUNCTIONS *****************************************************************/
void SerialNotificationSubscriber::onNotification(const user_notification_t& notification)
{
    /// Convert the numeric fields to the argument types expected by the log format.
    const unsigned int id       = static_cast<unsigned int>(notification.id);
    const unsigned int category = static_cast<unsigned int>(notification.category);
    const char*        kind     = (notification.kind == notification_kind_t::STATE) ? "STATE" : "EVENT";
    const int          active   = static_cast<int>(notification.active);
    const unsigned int sequence = static_cast<unsigned int>(notification.sequence);
    const unsigned int payload  = static_cast<unsigned int>(notification.payload.u32);

    /// Route the notification to the log level matching its severity.
    switch (notification.severity)
    {
        case notification_severity_t::INFO:
            SYS_LOG_I("Notification id=0x%02X category=0x%02X kind=%s active=%d seq=%u payload=%u", id, category, kind, active, sequence, payload);
            break;

        case notification_severity_t::WARNING:
            SYS_LOG_W("Notification id=0x%02X category=0x%02X kind=%s active=%d seq=%u payload=%u", id, category, kind, active, sequence, payload);
            break;

        case notification_severity_t::ERROR:
        case notification_severity_t::CRITICAL:
        default:
            SYS_LOG_E("Notification id=0x%02X category=0x%02X kind=%s active=%d seq=%u payload=%u", id, category, kind, active, sequence, payload);
            break;
    }
}
