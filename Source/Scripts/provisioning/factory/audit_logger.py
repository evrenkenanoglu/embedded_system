#!/usr/bin/env python3
"""
@file       audit_logger.py
@brief      Records manufacturing station audit logs for provisioned hardware units.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import datetime
import json
from pathlib import Path
from typing import Dict, Any


class AuditLogger:
    def __init__(self, output_dir: Path):
        self.output_dir = output_dir
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def record_provisioning_event(
        self,
        mac_address: str,
        device_id: str,
        hsvn: int,
        flash_key_file: str,
        sb_key_file: str,
        nvs_key_file: str,
        status: str,
        extra_metadata: Dict[str, Any] = None
    ) -> Path:
        record: Dict[str, Any] = {
            "mac_address": mac_address,
            "device_id": device_id,
            "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
            "hsvn": hsvn,
            "flash_key_file": flash_key_file,
            "sb_key_file": sb_key_file,
            "nvs_key_file": nvs_key_file,
            "status": status,
            "extra_metadata": extra_metadata or {}
        }

        audit_file = self.output_dir / f"audit_{mac_address.replace(':', '-')}.json"
        with open(audit_file, "w", encoding="utf-8") as f:
            json.dump(record, f, indent=4)

        return audit_file