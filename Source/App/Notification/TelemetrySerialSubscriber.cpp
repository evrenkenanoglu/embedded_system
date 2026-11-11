/** @file       TelemetrySerialSubscriber.cpp
 *  @brief      Writes notifications as CRC-protected serial telemetry records.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       10/11/2026
 */

/** INCLUDES ******************************************************************/
#include "App/Notification/TelemetrySerialSubscriber.hpp"

#include "Library/Common/Crc.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

/** CONSTANTS *****************************************************************/
namespace
{
    constexpr std::size_t kMaxRecordLength = 120U;
} // namespace

/** LOCAL FUNCTIONS ***********************************************************/
namespace
{
    bool writeRecord(char* record, std::size_t recordSize, int prefixLength)
    {
        if (prefixLength < 0 || static_cast<std::size_t>(prefixLength) >= recordSize)
        {
            return false;
        }

        Crc16             crc(Crc16::Profile::CcittFalse());
        const std::size_t prefixSize = static_cast<std::size_t>(prefixLength);
        for (std::size_t index = 0U; index < prefixSize; ++index)
        {
            crc.update(static_cast<uint8_t>(record[index]));
        }

        const int crcLength = std::snprintf(record + prefixSize, recordSize - prefixSize, " crc=0x%04X", static_cast<unsigned int>(crc.get()));
        if (crcLength < 0 || static_cast<std::size_t>(crcLength) >= (recordSize - prefixSize))
        {
            return false;
        }

        const int recordLength = prefixLength + crcLength;
        return std::printf("%s\n", record) == (recordLength + 1);
    }

    bool writeNotificationRecord(const user_notification_t& notification, uint32_t timestampMs)
    {
        const char* kind = nullptr;
        switch (notification.kind)
        {
            case notification_kind_t::STATE:
                kind = "STATE";
                break;
            case notification_kind_t::EVENT:
                kind = "EVENT";
                break;
            default:
                return false;
        }

        const char* severity = nullptr;
        switch (notification.severity)
        {
            case notification_severity_t::INFO:
                severity = "I";
                break;
            case notification_severity_t::WARNING:
                severity = "W";
                break;
            case notification_severity_t::ERROR:
                severity = "E";
                break;
            case notification_severity_t::CRITICAL:
                severity = "C";
                break;
            default:
                return false;
        }

        char      record[kMaxRecordLength];
        const int prefixLength = std::snprintf(
            record,
            sizeof(record),
            "@TLM v=1 seq=%u ms=%lu kind=%s id=0x%02X cat=0x%02X sev=%s act=%u pl=0x%08X",
            static_cast<unsigned int>(notification.sequence),
            static_cast<unsigned long>(timestampMs),
            kind,
            static_cast<unsigned int>(notification.id),
            static_cast<unsigned int>(notification.category),
            severity,
            notification.active ? 1U : 0U,
            static_cast<unsigned int>(notification.payload.u32));
        return writeRecord(record, sizeof(record), prefixLength);
    }

    bool writeDropRecord(uint32_t droppedCount)
    {
        char      record[kMaxRecordLength];
        const int prefixLength = std::snprintf(record, sizeof(record), "@TLM v=1 kind=DROP n=%lu", static_cast<unsigned long>(droppedCount));
        return writeRecord(record, sizeof(record), prefixLength);
    }
} // namespace

/** FUNCTIONS *****************************************************************/
void TelemetrySerialSubscriber::onNotification(const user_notification_t& notification)
{
    if (_droppedCount > 0U)
    {
        if (!writeDropRecord(_droppedCount))
        {
            _recordDrop();
            return;
        }
        _droppedCount = 0U;
    }

    const uint32_t timestampMs = static_cast<uint32_t>(_rtc.getMonotonicTimeUs() / 1000U);
    if (!writeNotificationRecord(notification, timestampMs))
    {
        _recordDrop();
    }
}

void TelemetrySerialSubscriber::_recordDrop()
{
    if (_droppedCount < 0xFFFFFFFFU)
    {
        ++_droppedCount;
    }
}
