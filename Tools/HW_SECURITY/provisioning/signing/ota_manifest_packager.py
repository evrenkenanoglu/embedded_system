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
import time
from pathlib import Path
from typing import Dict, Any


def calculate_sha256(file_path: Path) -> str:
    sha = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            sha.update(chunk)
    return sha.hexdigest()


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

    binary_path = args.binary.resolve()
    cert_path = args.signing_cert.resolve()
    manifest_path = args.manifest.resolve()

    if not binary_path.exists():
        print(f"[ERROR] Binary file not found: {binary_path}", file=sys.stderr)
        return 1

    if not cert_path.exists():
        print(f"[ERROR] Certificate file not found: {cert_path}", file=sys.stderr)
        return 1

    file_size = binary_path.stat().st_size
    file_hash = calculate_sha256(binary_path)
    cert_pem = cert_path.read_text(encoding="utf-8")

    # Immutable version-tagged filename (e.g. Embedded_IoT_BT_WIFI_Base_Project_1.0.1.bin)
    stem = binary_path.stem
    versioned_filename = (
        f"{stem}_{args.version}.bin"
        if not stem.endswith(args.version)
        else binary_path.name
    )

    storage_dir = manifest_path.parent
    binaries_dir = storage_dir / "binaries"
    binaries_dir.mkdir(parents=True, exist_ok=True)
    dest_binary_path = binaries_dir / versioned_filename

    # Copy binary into binaries/ subfolder with source != destination guard
    if args.copy_to_storage and (binary_path != dest_binary_path):
        shutil.copy2(binary_path, dest_binary_path)
        print(
            f"[OK] Stored immutable binary in binaries/: {dest_binary_path.name} ({file_size / (1024 * 1024):.2f} MB)"
        )

    manifest_data: Dict[str, Any] = {}
    if manifest_path.exists():
        try:
            with open(manifest_path, "r", encoding="utf-8") as f:
                manifest_data = json.load(f)
        except Exception:
            manifest_data = {}

    manifest_data.setdefault("schema_version", "1.0.0")
    manifest_data.setdefault("project_name", "")
    manifest_data.setdefault("channels", {})
    manifest_data["channels"][args.channel] = {
        "latest_version": args.version,
        "hardware_device": args.hardware,
    }

    manifest_data.setdefault("releases", {})
    existing_patches = (
        manifest_data.get("releases", {}).get(args.version, {}).get("patches", {})
    )

    manifest_data["releases"][args.version] = {
        "hardware": args.hardware,
        "channel": args.channel,
        "status": "active",
        "hsvn": args.hsvn,
        "canary_percentage": 100,
        "min_loader_version": "1.0.0",
        "binary": {
            "file_name": versioned_filename,
            "size_bytes": file_size,
            "sha256": file_hash,
            "signature": args.signature,
        },
        "file_name": versioned_filename,
        "target_size": file_size,
        "target_hash": file_hash,
        "target_signature": args.signature,
        "signing_cert": cert_pem,
        "target_hsvn": args.hsvn,
        "patches": existing_patches,
    }

    temp_path = manifest_path.with_suffix(".tmp")
    with open(temp_path, "w", encoding="utf-8") as f:
        json.dump(manifest_data, f, indent=4)
    shutil.move(temp_path, manifest_path)

    print(f"[OK] Release v{args.version} cataloged into: {manifest_path}")
    return 0


# Alias for callers importing package_manifest_entry directly
def package_manifest_entry() -> int:
    return main()


if __name__ == "__main__":
    sys.exit(main())
