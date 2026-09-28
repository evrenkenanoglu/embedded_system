#!/usr/bin/env python3
"""
@file       audit_logger.py
@brief      Records manufacturing station audit logs for provisioned hardware units.
            Outputs individual JSON logs and appends to a consolidated CSV ledger.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import csv
import datetime
import json
from pathlib import Path
from typing import Dict, Any, Optional


class AuditLogger:
    def __init__(self, output_dir: Path):
        self.output_dir = output_dir
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.summary_csv = self.output_dir / "manufacturing_summary.csv"
        self._ensure_csv_header()

    def _ensure_csv_header(self) -> None:
        """Initializes the CSV header if the summary ledger does not exist."""
        if not self.summary_csv.exists():
            with open(self.summary_csv, "w", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)
                writer.writerow([
                    "Timestamp_UTC",
                    "MAC_Address",
                    "Device_ID",
                    "HSVN",
                    "Flash_Key_File",
                    "SB_Key_File",
                    "NVS_Key_File",
                    "Status"
                ])

    def record_provisioning_event(
        self,
        mac_address: str,
        device_id: str,
        hsvn: int,
        flash_key_file: str,
        sb_key_file: str,
        nvs_key_file: str,
        status: str,
        extra_metadata: Optional[Dict[str, Any]] = None
    ) -> Path:
        timestamp_utc = datetime.datetime.now(datetime.timezone.utc).isoformat()

        record: Dict[str, Any] = {
            "mac_address": mac_address,
            "device_id": device_id,
            "timestamp_utc": timestamp_utc,
            "hsvn": hsvn,
            "flash_key_file": flash_key_file,
            "sb_key_file": sb_key_file,
            "nvs_key_file": nvs_key_file,
            "status": status,
            "extra_metadata": extra_metadata or {}
        }

        # 1. Write individual unit JSON audit trail
        audit_file = self.output_dir / f"audit_{mac_address.replace(':', '-')}.json"
        with open(audit_file, "w", encoding="utf-8") as f:
            json.dump(record, f, indent=4)

        # 2. Append to consolidated factory CSV summary ledger
        with open(self.summary_csv, "a", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow([
                timestamp_utc,
                mac_address,
                device_id,
                hsvn,
                Path(flash_key_file).name,
                Path(sb_key_file).name,
                Path(nvs_key_file).name,
                status
            ])

        return audit_file
