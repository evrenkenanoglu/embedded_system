"""Decoding for notification ids, category masks, and severity letters (NotificationTypes.hpp)."""

APP_ID_BASE = 0x80

FRAMEWORK_ID_NAMES: dict[int, str] = {
    0x00: "NONE",
    0x01: "SYS_BOOTING",
    0x02: "SYS_READY",
    0x03: "SYS_SHUTTING_DOWN",
    0x04: "SYS_FACTORY_RESET_TRIGGERED",
    0x05: "SYS_WATCHDOG_WARNING",
    0x06: "SYS_ABNORMAL_RESET_DETECTED",
    0x20: "WIFI_LINK_DISCONNECTED",
    0x21: "WIFI_CONNECTING",
    0x22: "WIFI_CONNECTED_AP",
    0x23: "WIFI_CONNECTED_STA",
    0x24: "WIFI_PROVISIONING_MODE",
    0x25: "MATTER_COMMISSIONING_WINDOW_OPEN",
    0x26: "MATTER_COMMISSIONING_IN_PROGRESS",
    0x27: "MATTER_COMMISSIONED",
    0x28: "MATTER_COMMISSIONING_FAILED",
    0x40: "SECURITY_AUTH_FAILED",
    0x41: "SECURITY_TAMPER_DETECTED",
    0x42: "SECURITY_CERTIFICATE_EXPIRED",
    0x50: "OTA_CHECKING",
    0x51: "OTA_UPDATING",
    0x52: "OTA_COMPLETED",
    0x53: "OTA_FAILED",
    0x54: "OTA_ROLLBACK_OCCURRED",
    0x68: "DIAG_THERMAL_WARNING",
    0x69: "DIAG_THERMAL_TRIP_SHUTDOWN",
    0x6A: "DIAG_HARDWARE_FAULT",
}

CATEGORY_NAMES: dict[int, str] = {
    0x01: "SYSTEM",
    0x02: "CONNECTIVITY",
    0x04: "SECURITY",
    0x08: "OTA",
    0x10: "DIAGNOSTICS",
    0x20: "APPLICATION",
}

SEVERITY_NAMES: dict[str, str] = {
    "I": "INFO",
    "W": "WARNING",
    "E": "ERROR",
    "C": "CRITICAL",
}


def is_known_notification_id(notification_id: int) -> bool:
    return notification_id >= APP_ID_BASE or notification_id in FRAMEWORK_ID_NAMES


def notification_name(notification_id: int) -> str:
    if notification_id in FRAMEWORK_ID_NAMES:
        return FRAMEWORK_ID_NAMES[notification_id]
    if notification_id >= APP_ID_BASE:
        return f"APP_0x{notification_id:02X}"
    return f"UNKNOWN_0x{notification_id:02X}"


def decode_categories(mask: int) -> tuple[str, ...]:
    names = [name for bit, name in CATEGORY_NAMES.items() if mask & bit]
    unknown_bits = mask & ~sum(CATEGORY_NAMES) & 0xFF
    if unknown_bits:
        names.append(f"UNKNOWN_0x{unknown_bits:02X}")
    return tuple(names)


def decode_severity(letter: str) -> str:
    return SEVERITY_NAMES[letter]
