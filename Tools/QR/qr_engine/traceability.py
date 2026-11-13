"""
@file       traceability.py
@brief      Append-only JSON Lines traceability entries. Payload plaintext is never stored.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path

from qr_engine import TOOL_NAME


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def build_entry(
    *,
    device_id: str,
    tool_version: str,
    payload: str,
    ecc: str,
    scale: int,
    border: int,
    outputs: dict,
) -> dict:
    """Builds one log record. `outputs` maps format name to the written file path."""
    payload_bytes = payload.encode("utf-8")
    return {
        "device_id": device_id,
        "timestamp_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "tool": TOOL_NAME,
        "tool_version": tool_version,
        "payload_sha256": sha256_bytes(payload_bytes),
        "payload_bytes": len(payload_bytes),
        "error_correction": ecc,
        "scale": scale,
        "border": border,
        "outputs": [
            {"format": fmt, "file": str(path), "sha256": sha256_file(path)}
            for fmt, path in outputs.items()
        ],
    }


def append_entry(log_path: Path, entry: dict) -> None:
    line = json.dumps(entry, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    with open(log_path, "a", encoding="utf-8") as fh:
        fh.write(line + "\n")
        fh.flush()
        os.fsync(fh.fileno())
