#!/usr/bin/env python3
"""
@file       ota_manifest_packager.py
@brief      Assembles release metadata into server manifest.json catalog entries
            and stores version-tagged immutable binaries.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import hashlib
import json
import shutil
import sys
from pathlib import Path
from typing import Dict, Any


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Package OTA release metadata into manifest catalog."
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        required=True,
        help="Path to existing or target manifest.json",
    )
    parser.add_argument(
        "--version", type=str, required=True, help="Semantic version (e.g. 1.0.1)"
    )
    parser.add_argument(
        "--binary", type=Path, required=True, help="Path to target application binary"
    )
    parser.add_argument(
        "--signature", type=str, required=True, help="HEX target signature string"
    )
    parser.add_argument(
        "--signing-cert",
        type=Path,
        required=True,
        help="Path to developer signing certificate PEM",
    )
    parser.add_argument(
        "--channel", type=str, default="stable", help="Deployment cohort channel"
    )
    parser.add_argument(
        "--hsvn", type=int, default=1, help="Hardware Security Version Number"
    )
    parser.add_argument(
        "--hardware",
        type=str,
        default="ESP32-C6-WROOM-1",
        help="Hardware device identifier",
    )
    parser.add_argument(
        "--copy-to-storage",
        action="store_true",
        default=True,
        help="Copy binary as version-tagged file into manifest's directory",
    )
    args = parser.parse_args()

    if not args.binary.exists():
        print(f"[ERROR] Binary file not found: {args.binary}", file=sys.stderr)
        return 1

    if not args.signing_cert.exists():
        print(f"[ERROR] Certificate file not found: {args.signing_cert}", file=sys.stderr)
        return 1

    file_size = args.binary.stat().st_size
    with open(args.binary, "rb") as f:
        file_hash = hashlib.sha256(f.read()).hexdigest()

    with open(args.signing_cert, "r", encoding="utf-8") as f:
        cert_pem = f.read()

    # Immutable version-tagged filename (e.g. Embedded_IoT_BT_WIFI_Base_Project_1.0.1.bin)
    stem = args.binary.stem
    versioned_filename = f"{stem}_{args.version}.bin" if not stem.endswith(args.version) else args.binary.name

    storage_dir = args.manifest.parent
    storage_dir.mkdir(parents=True, exist_ok=True)

    # Copy binary into storage if requested
    if args.copy_to_storage:
        dest_binary_path = storage_dir / versioned_filename
        shutil.copy2(args.binary, dest_binary_path)
        print(f"[OK] Stored immutable binary: {dest_binary_path.name} ({file_size / (1024 * 1024):.2f} MB)")

    manifest_data: Dict[str, Any] = {}
    if args.manifest.exists():
        try:
            with open(args.manifest, "r", encoding="utf-8") as f:
                manifest_data = json.load(f)
        except Exception:
            manifest_data = {}

    manifest_data.setdefault("schema_version", "1.0.0")
    manifest_data.setdefault("channels", {})
    manifest_data["channels"][args.channel] = {
        "latest_version": args.version,
        "hardware_device": args.hardware,
    }

    manifest_data.setdefault("releases", {})
    manifest_data["releases"][args.version] = {
        "file_name": versioned_filename,
        "target_size": file_size,
        "target_hash": file_hash,
        "target_signature": args.signature,
        "signing_cert": cert_pem,
        "target_hsvn": args.hsvn,
        "status": "active",
        "canary_percentage": 100,
        "patches": manifest_data.get("releases", {}).get(args.version, {}).get("patches", {}),
    }

    temp_path = args.manifest.with_suffix(".tmp")
    with open(temp_path, "w", encoding="utf-8") as f:
        json.dump(manifest_data, f, indent=4)
    shutil.move(temp_path, args.manifest)

    print(f"[OK] Release v{args.version} cataloged into: {args.manifest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
