#!/usr/bin/env python3
"""
@file       diff_worker.py
@brief      Standalone, framework-independent release catalog manager and 1-hop delta patch engine.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import hashlib
import json
import logging
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, Any, Optional

CATALOG_DIR = Path(__file__).resolve().parent


def calculate_sha256(file_path: Path) -> str:
    """Calculates SHA-256 digest in 64 KB chunks."""
    sha = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            sha.update(chunk)
    return sha.hexdigest()


def generate_delta_patch(base_path: Path, new_path: Path, patch_path: Path) -> bool:
    """
    Invokes cross-platform detools CLI to create a Heatshrink-compressed binary delta.
    Fails gracefully if detools is not installed, preserving full-binary delivery.
    """
    detools_executable = shutil.which("detools") or shutil.which("detools.exe")
    if not detools_executable:
        print(
            "[WARN] 'detools' executable not found in PATH. Skipping delta patch creation.",
            file=sys.stderr,
        )
        return False

    patch_path.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        detools_executable,
        "create_patch",
        "-c",
        "heatshrink",
        str(base_path.resolve()),
        str(new_path.resolve()),
        str(patch_path.resolve()),
    ]
    try:
        subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True
        )
        return True
    except subprocess.CalledProcessError as exc:
        print(
            f"[ERROR] detools failed (code {exc.returncode}): {exc.stderr.strip()}",
            file=sys.stderr,
        )
        return False
    except Exception as exc:
        print(f"[ERROR] Failed to execute detools process: {exc}", file=sys.stderr)
        return False


def load_manifest(manifest_path: Path) -> Dict[str, Any]:
    """Loads manifest.json or creates a clean canonical v1.0.0 structure."""
    if manifest_path.exists():
        try:
            with open(manifest_path, "r", encoding="utf-8") as f:
                data = json.load(f)
                if isinstance(data, dict):
                    return data
        except Exception as exc:
            print(
                f"[WARN] Failed to parse {manifest_path}: {exc}. Rebuilding structure.",
                file=sys.stderr,
            )

    return {
        "schema_version": "1.0.0",
        "project_name": "Embedded_IoT_BT_WIFI_Base_Project",
        "updated_at": int(time.time()),
        "channels": {},
        "releases": {},
    }


def save_manifest(manifest_path: Path, data: Dict[str, Any]) -> None:
    """Atomically replaces manifest.json to prevent corruption during concurrent reads."""
    data["updated_at"] = int(time.time())
    temp_path = manifest_path.with_suffix(".tmp")
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    with open(temp_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=4)
    shutil.move(str(temp_path), str(manifest_path))


def reconcile_1hop_patches(catalog_path: Path, manifest: Dict[str, Any]) -> bool:
    """
    Inspects all channels in manifest.json. If the latest release in a channel lacks
    a 1-hop patch against the immediate predecessor, generates it automatically.
    """
    binaries_dir = catalog_path / "binaries"
    patches_dir = catalog_path / "patches"
    updated = False

    channels = manifest.get("channels", {})
    releases = manifest.get("releases", {})

    for ch_name, ch_info in channels.items():
        latest_ver = ch_info.get("latest_version")
        if not latest_ver or latest_ver not in releases:
            continue

        # Find previous active release in this channel
        channel_releases = [
            v
            for v, r in releases.items()
            if r.get("channel") == ch_name
            and r.get("status") == "active"
            and v != latest_ver
        ]
        if not channel_releases:
            continue

        def semver_key(v: str):
            try:
                return [int(x) for x in v.split(".")]
            except ValueError:
                return [0]

        prev_ver = sorted(channel_releases, key=semver_key)[-1]
        latest_rel = releases[latest_ver]
        prev_rel = releases[prev_ver]

        latest_rel.setdefault("patches", {})
        if prev_ver in latest_rel["patches"]:
            continue

        base_bin_name = prev_rel.get("binary", {}).get("file_name") or prev_rel.get(
            "file_name"
        )
        new_bin_name = latest_rel.get("binary", {}).get("file_name") or latest_rel.get(
            "file_name"
        )

        if not base_bin_name or not new_bin_name:
            continue

        base_bin_path = binaries_dir / base_bin_name
        new_bin_path = binaries_dir / new_bin_name

        if base_bin_path.exists() and new_bin_path.exists():
            patch_name = f"patch_{prev_ver}_to_{latest_ver}.bin"
            patch_path = patches_dir / patch_name

            print(
                f"[*] [DIFF-WORKER] Generating 1-hop patch: {prev_ver} -> {latest_ver}..."
            )
            if generate_delta_patch(base_bin_path, new_bin_path, patch_path):
                p_size = patch_path.stat().st_size
                p_hash = calculate_sha256(patch_path)
                latest_rel["patches"][prev_ver] = {
                    "file_name": f"patches/{patch_name}",
                    "size_bytes": p_size,
                    "sha256": p_hash,
                    "signature": "",  # Unsigned on server; client verifies target_signature
                }
                updated = True
                print(
                    f"[OK] [DIFF-WORKER] Generated: {patch_name} ({p_size / 1024:.1f} KB)"
                )

    return updated


def register_release(
    catalog_path: Path,
    binary_path: Path,
    version: str,
    signature: str,
    signing_cert: str = "",
    hardware: str = "ESP32-C6-WROOM-1",
    channel: str = "stable",
    hsvn: int = 1,
    canary_percentage: int = 100,
    min_loader_version: str = "1.0.0",
    release_notes: str = "",
    project_name: str = "Embedded_IoT_BT_WIFI_Base_Project",
) -> None:
    """Stores the binary with an immutable version tag, computes hashes, and updates manifest."""
    binaries_dir = catalog_path / "binaries"
    binaries_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = catalog_path / "manifest.json"

    # 1. Store immutable version-tagged binary
    stem = binary_path.stem
    versioned_filename = (
        f"{stem}_{version}.bin" if not stem.endswith(version) else binary_path.name
    )
    target_binary = binaries_dir / versioned_filename
    shutil.copy2(binary_path, target_binary)

    file_size = target_binary.stat().st_size
    sha256_hash = calculate_sha256(target_binary)

    # 2. Update catalog manifest
    manifest = load_manifest(manifest_path)
    manifest["project_name"] = project_name

    manifest.setdefault("channels", {})
    manifest.setdefault("releases", {})

    manifest["releases"][version] = {
        "hardware": hardware,
        "channel": channel,
        "status": "active",
        "hsvn": hsvn,
        "canary_percentage": canary_percentage,
        "min_loader_version": min_loader_version,
        "binary": {
            "file_name": versioned_filename,
            "size_bytes": file_size,
            "sha256": sha256_hash,
            "signature": signature,
        },
        "signing_cert": signing_cert or None,
        "release_notes": release_notes,
        "patches": manifest.get("releases", {}).get(version, {}).get("patches", {}),
    }

    # 3. Compute 1-hop delta patch against previous release
    reconcile_1hop_patches(catalog_path, manifest)

    # 4. Point channel to latest version
    manifest["channels"][channel] = {
        "latest_version": version,
        "hardware": hardware,
    }

    save_manifest(manifest_path, manifest)
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print(f" ❯ 📦 RELEASE CATALOG UPDATED")
    print(f"   Binary Stored   : {target_binary}")
    print(f"   Target Version  : v{version} (Channel: {channel}, HSVN: {hsvn})")
    print(f"   File Size       : {file_size / (1024 * 1024):.2f} MB")
    print(f"   SHA-256 Digest  : {sha256_hash}")
    print(f"   Signature Len   : {len(signature)} chars")
    print(f"   Manifest Catalog: {manifest_path}")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Release Catalog Manager: Stores immutable binaries, manages catalog, and creates 1-hop diffs."
    )
    parser.add_argument(
        "--catalog-dir",
        type=Path,
        default=CATALOG_DIR,
        help="Path to release-catalog directory",
    )
    parser.add_argument(
        "--binary", type=Path, default=None, help="Path to compiled application binary"
    )
    parser.add_argument(
        "--version", type=str, default=None, help="Semver string (e.g. 1.0.1)"
    )
    parser.add_argument(
        "--signature",
        type=str,
        default="",
        help="Pre-computed developer target signature (HEX)",
    )
    parser.add_argument(
        "--signing-cert",
        type=Path,
        default=None,
        help="Path to developer signing.crt PEM",
    )
    parser.add_argument(
        "--hardware",
        type=str,
        default="ESP32-C6-WROOM-1",
        help="Target hardware profile",
    )
    parser.add_argument(
        "--channel", type=str, default="stable", help="Deployment cohort channel"
    )
    parser.add_argument(
        "--hsvn", type=int, default=1, help="Hardware Security Version Number"
    )
    parser.add_argument(
        "--canary", type=int, default=100, help="Canary percentage (0-100)"
    )
    parser.add_argument(
        "--notes",
        type=str,
        default="Production release build.",
        help="Changelog or release notes",
    )
    parser.add_argument(
        "--project-name",
        type=str,
        default="Embedded_IoT_BT_WIFI_Base_Project",
        help="Project name",
    )
    parser.add_argument(
        "--reconcile",
        action="store_true",
        help="Only reconcile missing 1-hop delta patches",
    )
    args = parser.parse_args()

    catalog_path = args.catalog_dir.resolve()
    manifest_path = catalog_path / "manifest.json"

    if args.reconcile:
        manifest = load_manifest(manifest_path)
        if reconcile_1hop_patches(catalog_path, manifest):
            save_manifest(manifest_path, manifest)
            print("[OK] Reconciliation completed. Manifest updated.")
        else:
            print("[OK] All 1-hop patches up to date.")
        return 0

    if not args.binary or not args.version:
        print(
            "[ERROR] Both --binary and --version are required unless running with --reconcile.",
            file=sys.stderr,
        )
        return 1

    if not args.binary.exists():
        print(f"[ERROR] Target binary not found: {args.binary}", file=sys.stderr)
        return 1

    cert_pem = ""
    if args.signing_cert and args.signing_cert.exists():
        cert_pem = args.signing_cert.read_text(encoding="utf-8")

    register_release(
        catalog_path=catalog_path,
        binary_path=args.binary.resolve(),
        version=args.version,
        signature=args.signature,
        signing_cert=cert_pem,
        hardware=args.hardware,
        channel=args.channel,
        hsvn=args.hsvn,
        canary_percentage=args.canary,
        release_notes=args.notes,
        project_name=args.project_name,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
