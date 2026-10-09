"""
@file       storage.py
@brief      Autonomous storage manager and 1-hop patch reconciler for ota-server.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import json
import logging
import shutil
import time
from pathlib import Path

from src.core.config import settings
from src.core.manifest_schema import OtaManifest
from src.core.patch import generate_delta_patch

logger = logging.getLogger("uvicorn.error")


def calculate_sha256(file_path: Path) -> str:
    import hashlib

    sha = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(settings.CHUNK_SIZE_BYTES):
            sha.update(chunk)
    return sha.hexdigest()


def reconcile_1hop_patches() -> None:
    """
    Autonomous server hook: Inspects firmware_storage/ on startup or file ingestion.
    If the latest release in a channel lacks a 1-hop patch against the previous version,
    generates it automatically on the server without needing private keys.
    """
    if not settings.MANIFEST_FILE.exists():
        return

    try:
        with open(settings.MANIFEST_FILE, "r", encoding="utf-8") as f:
            manifest = OtaManifest.model_validate(json.load(f))
    except Exception as exc:
        logger.error(f"[STORAGE] Failed to load manifest for reconciliation: {exc}")
        return

    updated = False
    patches_dir = settings.FIRMWARE_DIR / "patches"
    patches_dir.mkdir(parents=True, exist_ok=True)

    for channel_name, channel_info in manifest.channels.items():
        latest_ver = channel_info.latest_version
        if not latest_ver or latest_ver not in manifest.releases:
            continue

        # Find the previous active release in this channel
        all_channel_releases = [
            v
            for v, r in manifest.releases.items()
            if r.channel == channel_name and r.status == "active" and v != latest_ver
        ]
        if not all_channel_releases:
            continue

        def semver_key(v):
            try:
                return [int(x) for x in v.split(".")]
            except ValueError:
                return [0]

        prev_ver = sorted(all_channel_releases, key=semver_key)[-1]
        latest_release = manifest.releases[latest_ver]
        prev_release = manifest.releases[prev_ver]

        # Check if 1-hop patch already exists
        if prev_ver in latest_release.patches:
            continue

        base_bin = settings.FIRMWARE_DIR / prev_release.binary.file_name
        new_bin = settings.FIRMWARE_DIR / latest_release.binary.file_name

        if base_bin.exists() and new_bin.exists():
            patch_name = f"patch_{prev_ver}_to_{latest_ver}.bin"
            patch_path = patches_dir / patch_name

            logger.info(
                f"[STORAGE] Generating 1-hop delta patch: {prev_ver} -> {latest_ver}..."
            )
            if generate_delta_patch(str(base_bin), str(new_bin), str(patch_path)):
                p_size = patch_path.stat().st_size
                p_hash = calculate_sha256(patch_path)

                latest_release.patches[prev_ver] = {
                    "file_name": f"patches/{patch_name}",
                    "size_bytes": p_size,
                    "sha256": p_hash,
                    "signature": "",
                }
                updated = True
                logger.info(
                    f"⚡ [STORAGE] 1-Hop patch generated successfully: {patch_name} ({p_size / 1024:.1f} KB)"
                )

    if updated:
        manifest.updated_at = int(time.time())
        temp_path = settings.MANIFEST_FILE.with_suffix(".tmp")
        with open(temp_path, "w", encoding="utf-8") as f:
            json.dump(manifest.model_dump(), f, indent=4)
        shutil.move(temp_path, settings.MANIFEST_FILE)
