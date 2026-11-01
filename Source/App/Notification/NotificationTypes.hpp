/** @file       NotificationTypes.hpp
 *  @brief      Generic IoT notification types, categories, and framework IDs.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

#pragma once

#include "System/system.h"

#include <cstdint>

/**
 * @brief Functional category domains for coarse-grained subscription filtering.
 */
enum class notification_category_t : uint8_t
{
    SYSTEM       = (1U << 0), ///< Boot, reset reason, watchdog, factory reset
    CONNECTIVITY = (1U << 1), ///< Wi-Fi, Ethernet, BLE commissioning, Matter fabric
    SECURITY     = (1U << 2), ///< Authentication, tamper, certificates
    OTA          = (1U << 3), ///< Firmware check, download progress, verification, rollback
    DIAGNOSTICS  = (1U << 4), ///< Thermal sensors, hardware faults
    APPLICATION  = (1U << 5), ///< Project-specific domain events
    ALL          = 0x3F       ///< All defined category bits
};

/** Bitwise combination of category flags for notification_filter_t::categoryMask. */
constexpr uint8_t operator|(notification_category_t lhs, notification_category_t rhs)
{
    return static_cast<uint8_t>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

/**
 * @brief Notification kind classification.
 */
enum class notification_kind_t : uint8_t
{
    STATE = 0, ///< Persistent condition, kept in the snapshot and replayed to new subscribers
    EVENT = 1  ///< One-shot occurrence, dispatched once and not retained
};

/**
 * @brief Severity classification for presentation mapping (LED priority, log level).
 */
enum class notification_severity_t : uint8_t
{
    INFO = 0,
    WARNING,
    ERROR,
    CRITICAL
};

/**
 * @brief 4-byte scalar payload passed by value (no heap allocation).
 */
typedef union
{
    uint32_t    u32;         ///< Progress percentage, counters, raw bitflags
    int32_t     i32;         ///< Signed metrics such as temperature in millidegrees
    sys_error_t err;         ///< Operation error code
    uint8_t     rawBytes[4]; ///< Custom application bytes; never log as text
} notification_payload_t;

/** Boundary between framework IDs (0x00..0x7F) and application IDs (0x80..0xFF). */
constexpr uint8_t  kNotificationFrameworkMax = 0x7F;
constexpr uint8_t  kNotificationAppBase      = 0x80;
constexpr uint16_t kNotificationTotalIds     = 256;

/**
 * @brief Framework notification identifiers (0x00 .. 0x7F).
 */
enum class framework_notification_id_t : uint8_t
{
    NONE = 0x00,

    // SYSTEM (0x01 .. 0x1F)
    SYS_BOOTING = 0x01,
    SYS_READY,
    SYS_SHUTTING_DOWN,
    SYS_FACTORY_RESET_TRIGGERED,
    SYS_WATCHDOG_WARNING,
    SYS_ABNORMAL_RESET_DETECTED,

    // CONNECTIVITY (0x20 .. 0x3F)
    WIFI_LINK_DISCONNECTED = 0x20, ///< Renamed: WIFI_DISCONNECTED collides with a cpx_wifi.h macro
    WIFI_CONNECTING,
    WIFI_CONNECTED_AP,
    WIFI_CONNECTED_STA,
    WIFI_PROVISIONING_MODE,
    MATTER_COMMISSIONING_WINDOW_OPEN,
    MATTER_COMMISSIONING_IN_PROGRESS,
    MATTER_COMMISSIONED,
    MATTER_COMMISSIONING_FAILED,

    // SECURITY (0x40 .. 0x4F)
    SECURITY_AUTH_FAILED = 0x40,
    SECURITY_TAMPER_DETECTED,
    SECURITY_CERTIFICATE_EXPIRED,

    // OTA (0x50 .. 0x67)
    OTA_CHECKING = 0x50,
    OTA_UPDATING,
    OTA_COMPLETED,
    OTA_FAILED,
    OTA_ROLLBACK_OCCURRED,

    // DIAGNOSTICS (0x68 .. 0x7F)
    DIAG_THERMAL_WARNING = 0x68,
    DIAG_THERMAL_TRIP_SHUTDOWN,
    DIAG_HARDWARE_FAULT
};

/**
 * @brief Notification identifier covering both framework and application IDs.
 */
typedef uint8_t notification_id_t;

/**
 * @brief Notification delivered to subscribers.
 *
 * Publishers fill id, category, kind, severity, active, and payload.
 * NotificationManager assigns sequence when the notification is accepted.
 */
typedef struct
{
    notification_id_t       id;       ///< Framework or application identifier
    notification_category_t category; ///< Category domain
    notification_kind_t     kind;     ///< STATE or EVENT
    notification_severity_t severity; ///< Severity classification
    bool                    active;   ///< True = asserted, false = cleared
    uint16_t                sequence; ///< Monotonic ordering counter, assigned by the manager
    notification_payload_t  payload;  ///< Contextual scalar payload
} user_notification_t;

/**
 * @brief Subscription filter applied by the manager before delivery.
 */
typedef struct
{
    uint8_t                 categoryMask; ///< Bitmask of notification_category_t values
    notification_severity_t minSeverity;  ///< Minimum severity required for delivery
} notification_filter_t;
