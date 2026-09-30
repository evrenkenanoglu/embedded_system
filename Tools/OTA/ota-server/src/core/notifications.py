"""
@file       notifications.py
@brief      Asynchronous incident webhook dispatcher for OTA server events.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import datetime
import json
import logging
from typing import Dict, Any, Optional
import requests
from src.core.config import settings

logger = logging.getLogger("uvicorn.error")


def dispatch_rollback_alert(
    target_version: str,
    channel: str,
    failure_rate: float,
    failed_count: int,
    total_count: int,
    reverted_to: Optional[str] = None,
) -> bool:
    """
    Dispatches a structured JSON incident alert to the configured webhook endpoint.

    :param target_version: Firmware version that breached failure thresholds.
    :param channel: Deployment cohort channel (e.g. stable, beta).
    :param failure_rate: Calculated failure percentage in the sliding window.
    :param failed_count: Number of failed client reports.
    :param total_count: Total client reports in the active window.
    :param reverted_to: Previous stable version restored as active target.
    :return: True if webhook was successfully delivered or omitted, False on failure.
    """
    webhook_url = settings.WEBHOOK_URL.strip()
    if not webhook_url:
        logger.info("[ALERT] Webhook URL not configured. Notification bypassed.")
        return True

    timestamp_utc = datetime.datetime.now(datetime.timezone.utc).isoformat()

    payload: Dict[str, Any] = {
        "event": "AUTOMATIC_EMERGENCY_ROLLBACK",
        "severity": "CRITICAL",
        "project_name": settings.PROJECT_NAME,
        "timestamp_utc": timestamp_utc,
        "details": {
            "revoked_version": target_version,
            "channel": channel,
            "reverted_to_version": reverted_to or "NONE",
            "failure_rate_percent": round(failure_rate, 2),
            "threshold_percent": settings.MAX_FAILURE_RATE_PERCENT,
            "failed_reports": failed_count,
            "total_reports_evaluated": total_count,
            "window_seconds": settings.SLIDING_WINDOW_SECONDS,
        },
        "text": (
            f":rotating_light: *CRITICAL: Automatic OTA Rollback Triggered!*\n"
            f"*Project*: {settings.PROJECT_NAME}\n"
            f"*Revoked Version*: `{target_version}` (Channel: `{channel}`)\n"
            f"*Failure Rate*: *{failure_rate:.1f}%* ({failed_count}/{total_count} reports in trailing {settings.SLIDING_WINDOW_SECONDS // 60}m)\n"
            f"*Action Taken*: Version marked `soft-rolled-back`. Reverted to `{reverted_to or 'NONE'}`."
        ),
    }

    try:
        response = requests.post(
            webhook_url,
            json=payload,
            headers={"Content-Type": "application/json"},
            timeout=5.0,
        )
        if response.status_code in [200, 204]:
            logger.info(f"[ALERT] Incident webhook delivered to {webhook_url}")
            return True
        else:
            logger.error(
                f"[ALERT] Webhook delivery failed: HTTP {response.status_code} - {response.text}"
            )
            return False
    except Exception as exc:
        logger.error(f"[ALERT] Exception while dispatching webhook notification: {exc}")
        return False
