"""
@file       ota.py
@brief      Device handshake, 1-hop delta routing, download streaming, and telemetry.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import hashlib
import hmac
import json
import logging
import shutil
import time
from pathlib import Path
from pydantic import BaseModel, Field
from fastapi import APIRouter, HTTPException, Query, Request, status
from fastapi.responses import FileResponse

from src.core.config import settings
from src.core.manifest_schema import OtaManifest
from src.core.security import verify_certificate_status
from src.core.notifications import dispatch_rollback_alert

logger = logging.getLogger("uvicorn.error")

router = APIRouter()
direct_router = APIRouter()

DOWNLOAD_ROUTE_PREFIX = "/download"


class TelemetryReport(BaseModel):
    device_id: str = Field(..., description="Unique hardware identifier MAC or UUID")
    previous_version: str = Field(..., description="Currently installed SemVer")
    target_version: str = Field(..., description="Destination OTA SemVer attempt")
    status: str = Field(..., description="'success' or 'failure'")
    error_code: int = Field(
        0, description="Platform crash or code-sign failure reason code"
    )


def semver_key(v: str):
    """Sorts semver strings numerically rather than alphabetically."""
    try:
        return [int(x) for x in v.split(".")]
    except ValueError:
        return [0]


def is_newer_version(current: str, latest: str) -> bool:
    try:
        return semver_key(latest) > semver_key(current)
    except Exception:
        return latest != current


def is_in_canary_group(
    device_id: str, target_version: str, target_percentage: int
) -> bool:
    if target_percentage >= 100:
        return True
    if target_percentage <= 0:
        return False
    hash_payload = f"{device_id}:{target_version}".encode("utf-8")
    hash_digest = hashlib.md5(hash_payload).hexdigest()
    return (int(hash_digest, 16) % 100) < target_percentage


def generate_download_token(
    filename: str, expires_in_sec: int = settings.TOKEN_EXPIRATION_SECONDS
) -> str:
    expire_time = int(time.time()) + expires_in_sec
    message = f"{filename}:{expire_time}".encode("utf-8")
    signature = hmac.new(
        settings.API_KEY.encode("utf-8"), message, hashlib.sha256
    ).hexdigest()
    return f"{expire_time}.{signature}"


def verify_download_token(filename: str, token: str) -> bool:
    try:
        expire_str, signature = token.split(".", 1)
        expire_time = int(expire_str)
        if time.time() > expire_time:
            logger.warning(f"Presigned token for '{filename}' has expired.")
            return False
        message = f"{filename}:{expire_time}".encode("utf-8")
        expected_sig = hmac.new(
            settings.API_KEY.encode("utf-8"), message, hashlib.sha256
        ).hexdigest()
        return hmac.compare_digest(expected_sig, signature)
    except Exception:
        return False


def authenticate_request(request: Request, filename: str, token: str = None):
    api_key = request.headers.get(settings.API_KEY_HEADER)
    if api_key and api_key == settings.API_KEY:
        return
    if token and verify_download_token(filename, token):
        return
    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Invalid, missing, or expired download credentials.",
    )


def resolve_file_path(filename: str) -> Path:
    """Resolves binary or patch file path from the decoupled release-catalog."""
    clean_name = filename.strip().lstrip("/")

    # 1. Check in patches/
    if clean_name.startswith("patches/"):
        target_path = (
            settings.PATCHES_DIR / clean_name.replace("patches/", "")
        ).resolve()
        if target_path.exists():
            return target_path

    # 2. Check in binaries/
    target_path = (settings.BINARIES_DIR / clean_name).resolve()
    if target_path.exists():
        return target_path

    # 3. Fallback to catalog root
    fallback_path = (settings.CATALOG_DIR / clean_name).resolve()
    if fallback_path.exists():
        return fallback_path

    return target_path


async def get_validated_file_response(filename: str) -> FileResponse:
    file_path = resolve_file_path(filename)
    if not file_path.is_relative_to(settings.CATALOG_DIR.resolve()):
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN,
            detail="Access denied. Path traversal detected.",
        )
    if not file_path.exists() or not file_path.is_file():
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail=f"File not found: {filename}"
        )
    return FileResponse(
        path=file_path, media_type="application/octet-stream", filename=file_path.name
    )


def load_manifest() -> OtaManifest:
    if settings.MANIFEST_FILE.exists():
        try:
            with open(settings.MANIFEST_FILE, "r", encoding="utf-8") as f:
                return OtaManifest.model_validate(json.load(f))
        except Exception as e:
            logger.error(f"Failed to parse manifest: {e}")
    return OtaManifest()


def save_manifest(manifest: OtaManifest) -> None:
    temp_path = settings.MANIFEST_FILE.with_suffix(".tmp")
    with open(temp_path, "w", encoding="utf-8") as f:
        json.dump(manifest.model_dump(), f, indent=4)
    shutil.move(temp_path, settings.MANIFEST_FILE)


def evaluate_auto_rollback(target_version: str):
    if not settings.TELEMETRY_LOG_DIR.exists():
        return
    current_time = int(time.time())
    window_cutoff = current_time - settings.SLIDING_WINDOW_SECONDS
    successes, failures = 0, 0

    for file_path in settings.TELEMETRY_LOG_DIR.glob("device_*.json"):
        try:
            with open(file_path, "r", encoding="utf-8") as f:
                history = json.load(f)
                for entry in history:
                    if (
                        entry.get("target_version") == target_version
                        and entry.get("timestamp", 0) >= window_cutoff
                    ):
                        status_val = entry.get("status", "")
                        if status_val == "success":
                            successes += 1
                        elif status_val in ["failure", "rollback"]:
                            failures += 1
        except Exception:
            continue

    total = successes + failures
    if total < settings.MIN_STATUS_REPORTS_FOR_ROLLBACK:
        return

    rate = (failures / total) * 100.0
    if rate >= settings.MAX_FAILURE_RATE_PERCENT:
        logger.critical(
            f"[ROLLBACK TRIGGER] Version '{target_version}' failure rate: {rate:.1f}% ({failures}/{total})"
        )
        manifest = load_manifest()
        if target_version in manifest.releases:
            rel = manifest.releases[target_version]
            rel.status = "soft-rolled-back"
            ch = rel.channel

            remaining = [
                v
                for v, r in manifest.releases.items()
                if r.channel == ch and r.status == "active"
            ]
            reverted = sorted(remaining, key=semver_key)[-1] if remaining else ""
            if ch in manifest.channels:
                manifest.channels[ch].latest_version = reverted

            save_manifest(manifest)
            dispatch_rollback_alert(target_version, ch, rate, failures, total, reverted)


@router.post("/status")
async def ota_status_report(request: Request, report: TelemetryReport):
    authenticate_request(
        request, "telemetry", request.headers.get(settings.API_KEY_HEADER)
    )
    settings.TELEMETRY_LOG_DIR.mkdir(parents=True, exist_ok=True)
    safe_id = report.device_id.replace(":", "-")
    log_file = settings.TELEMETRY_LOG_DIR / f"device_{safe_id}.json"

    history = []
    if log_file.exists():
        try:
            history = json.loads(log_file.read_text(encoding="utf-8"))
        except Exception:
            pass
    r_dict = report.model_dump()
    r_dict["timestamp"] = int(time.time())
    history.append(r_dict)
    log_file.write_text(json.dumps(history, indent=2), encoding="utf-8")

    evaluate_auto_rollback(report.target_version)
    return {"status": "recorded"}


@router.get("/check")
async def ota_check(
    request: Request,
    ver: str = Query(None),
    hw: str = Query(None),
    device_id: str = Query(None),
    channel: str = Query(None),
    hsvn: int = Query(None),
):
    authenticate_request(
        request, "manifest.json", request.headers.get(settings.API_KEY_HEADER)
    )

    client_version = request.headers.get(settings.HEADER_VERSION_KEY) or ver
    client_hardware = request.headers.get(settings.HEADER_HARDWARE_KEY) or hw
    client_device_id = request.headers.get(settings.HEADER_DEVICE_ID_KEY) or device_id
    client_channel = (
        request.headers.get(settings.HEADER_CHANNEL_KEY)
        or channel
        or settings.DEFAULT_CHANNEL
    )
    client_hsvn = int(
        request.headers.get(settings.HEADER_HSVN_KEY) or hsvn or settings.DEFAULT_HSVN
    )

    if not client_version or not client_hardware or not client_device_id:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Missing identification headers.",
        )

    manifest = load_manifest()
    channel_info = manifest.channels.get(client_channel)
    if not channel_info or not channel_info.latest_version:
        return {"update_available": False, "message": "No active updates in channel."}

    latest_version = channel_info.latest_version
    target_release = manifest.releases.get(latest_version)

    if not target_release or target_release.status != "active":
        return {
            "update_available": False,
            "message": "Target release inactive or missing.",
        }

    # Verify hardware compatibility
    target_hw = target_release.hardware or channel_info.hardware
    if client_hardware != target_hw:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Incompatible hardware profile. Target: {client_hardware}, Required: {target_hw}",
        )

    if is_newer_version(client_version, latest_version):
        if client_hsvn > target_release.hsvn:
            return {
                "update_available": False,
                "message": "Anti-downgrade boundary active.",
            }

        if not is_in_canary_group(
            client_device_id, latest_version, target_release.canary_percentage
        ):
            return {
                "update_available": False,
                "message": "Device not targeted in canary cohort.",
            }

        signing_cert_pem = target_release.signing_cert or ""
        if signing_cert_pem:
            is_valid, reason = verify_certificate_status(signing_cert_pem)
            if not is_valid:
                logger.critical(f"[SECURITY REVOCATION] Refusing update: {reason}")
                return {
                    "update_available": False,
                    "message": f"Signing certificate revoked ({reason}).",
                }

        base_url = str(request.base_url).rstrip("/")

        # --- 1-HOP RULE EVALUATION ---
        if client_version in target_release.patches:
            patch = target_release.patches[client_version]
            token = generate_download_token(patch.file_name)
            download_url = (
                f"{base_url}{DOWNLOAD_ROUTE_PREFIX}/{patch.file_name}?token={token}"
            )
            logger.info(
                f"⚡ Delivering 1-Hop Delta: {client_version} -> {latest_version}"
            )

            return {
                "update_available": True,
                "update_type": "delta",
                "version": latest_version,
                "url": download_url,
                "size": patch.size_bytes,
                "sha256": patch.sha256,
                "signature": patch.signature,
                "target_sha256": target_release.binary.sha256,
                "target_signature": target_release.binary.signature,
                "target_size": target_release.binary.size_bytes,
                "signing_cert": signing_cert_pem,
                "notes": target_release.release_notes,
            }

        # --- FALLBACK: FULL BINARY DELIVERY ---
        bin_name = target_release.binary.file_name
        token = generate_download_token(bin_name)
        download_url = f"{base_url}{DOWNLOAD_ROUTE_PREFIX}/{bin_name}?token={token}"
        logger.info(f"📦 Delivering Full Binary Fallback: v{latest_version}")

        return {
            "update_available": True,
            "update_type": "full",
            "version": latest_version,
            "url": download_url,
            "size": target_release.binary.size_bytes,
            "sha256": target_release.binary.sha256,
            "signature": target_release.binary.signature,
            "target_sha256": target_release.binary.sha256,
            "target_signature": target_release.binary.signature,
            "target_size": target_release.binary.size_bytes,
            "signing_cert": signing_cert_pem,
            "notes": target_release.release_notes,
        }

    return {"update_available": False, "message": "Firmware is up to date."}


# Single consolidated download endpoint mounted at /download/{filename:path}
@direct_router.get(f"{DOWNLOAD_ROUTE_PREFIX}/{{filename:path}}")
async def download_file(filename: str, request: Request, token: str = Query(None)):
    authenticate_request(request, filename, token)
    return await get_validated_file_response(filename)
