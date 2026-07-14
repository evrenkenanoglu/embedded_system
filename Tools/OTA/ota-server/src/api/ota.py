import hmac
import hashlib
import json
import time
import logging
from pathlib import Path
from pydantic import BaseModel, Field
from fastapi import APIRouter, HTTPException, Query, Request, status
from fastapi.responses import FileResponse
from src.core.config import settings
from src.core.security import get_signing_certificate_pem

logger = logging.getLogger("uvicorn.error")

router = APIRouter()
direct_router = APIRouter()

DOWNLOAD_ROUTE_PREFIX = f"/{settings.FIRMWARE_DIR.name}"


class TelemetryReport(BaseModel):
    device_id: str = Field(..., description="Unique hardware identifier MAC or UUID")
    previous_version: str = Field(..., description="Currently installed SemVer")
    target_version: str = Field(..., description="Destination OTA SemVer attempt")
    status: str = Field(..., description="'success' or 'failure'")
    error_code: int = Field(0, description="Platform crash or code-sign failure reason code")


def is_newer_version(current: str, latest: str) -> bool:
    try:
        curr_parts = [int(x) for x in current.split(".")]
        late_parts = [int(x) for x in latest.split(".")]
        return late_parts > curr_parts
    except (ValueError, AttributeError):
        return latest != current


def is_in_canary_group(device_id: str, target_version: str, target_percentage: int) -> bool:
    """Determines deterministically if a device falls within the canary target rollout percentage."""
    if target_percentage >= 100:
        return True
    if target_percentage <= 0:
        return False
    hash_payload = f"{device_id}:{target_version}".encode("utf-8")
    hash_digest = hashlib.md5(hash_payload).hexdigest()
    # Modulo yields deterministic value [0..99]
    hash_value = int(hash_digest, 16) % 100
    return hash_value < target_percentage


def generate_download_token(filename: str, expires_in_sec: int = settings.TOKEN_EXPIRATION_SECONDS) -> str:
    expire_time = int(time.time()) + expires_in_sec
    message = f"{filename}:{expire_time}".encode("utf-8")
    signature = hmac.new(
        settings.API_KEY.encode("utf-8"), 
        message, 
        hashlib.sha256
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
            settings.API_KEY.encode("utf-8"), 
            message, 
            hashlib.sha256
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
    logger.warning(f"Unauthorized access attempt to download '{filename}'")
    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Invalid, missing, or expired download credentials."
    )


async def get_validated_file_response(filename: str) -> FileResponse:
    file_path = (settings.FIRMWARE_DIR / filename).resolve()
    if not file_path.is_relative_to(settings.FIRMWARE_DIR.resolve()):
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, 
            detail="Access denied. Directory traversal detected."
        )
    if not file_path.exists() or not file_path.is_file():
        logger.error(f"Binary file not found on disk: {file_path}")
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, 
            detail="Requested binary file not found."
        )
    return FileResponse(
        path=file_path,
        media_type="application/octet-stream",
        filename=filename
    )


def log_telemetry(report: TelemetryReport):
    """Saves structured telemetry payload to local storage directory."""
    settings.TELEMETRY_LOG_DIR.mkdir(parents=True, exist_ok=True)
    log_file = settings.TELEMETRY_LOG_DIR / f"device_{report.device_id}.json"
    
    device_history = []
    if log_file.exists():
        try:
            with open(log_file, "r") as f:
                device_history = json.load(f)
        except json.JSONDecodeError:
            pass
            
    report_dict = report.model_dump()
    report_dict["timestamp"] = int(time.time())
    device_history.append(report_dict)
    
    with open(log_file, "w") as f:
        json.dump(device_history, f, indent=2)


def evaluate_auto_rollback(target_version: str):
    """Inspects all telemetry events for a firmware version to trigger auto-rollback on failure."""
    if not settings.TELEMETRY_LOG_DIR.exists():
        return

    successes = 0
    failures = 0

    for file_path in settings.TELEMETRY_LOG_DIR.glob("device_*.json"):
        try:
            with open(file_path, "r") as f:
                history = json.load(f)
                # Parse entries for the specific target version
                for entry in history:
                    if entry.get("target_version") == target_version:
                        if entry.get("status") == "success":
                            successes += 1
                        elif entry.get("status") == "failure":
                            failures += 1
        except Exception:
            continue

    total_reports = successes + failures
    if total_reports < settings.MIN_STATUS_REPORTS_FOR_ROLLBACK:
        return

    failure_rate = (failures / total_reports) * 100
    if failure_rate >= settings.MAX_FAILURE_RATE_PERCENT:
        logger.critical(
            f"Auto-Rollback Triggered! Version {target_version} has failed {failures}/{total_reports} "
            f"attempts ({failure_rate:.1f}% failure rate, maximum allowed is {settings.MAX_FAILURE_RATE_PERCENT}%)."
        )
        _soft_rollback_version_in_manifest(target_version)


def _soft_rollback_version_in_manifest(target_version: str):
    """Soft rolls back the firmware version from active status in the database."""
    if not settings.MANIFEST_FILE.exists():
        return

    with open(settings.MANIFEST_FILE, "r") as f:
        try:
            data = json.load(f)
        except json.JSONDecodeError:
            return

    target_update = data.get("updates", {}).get(target_version)
    if target_update and target_update.get("status") == "active":
        target_update["status"] = "soft-rolled-back"
        target_channel = target_update.get("channel", settings.DEFAULT_CHANNEL)

        # Re-evaluate latest channel version map excluding this failed target
        remaining_versions_in_channel = [
            v for v, info in data.get("updates", {}).items() 
            if info.get("channel") == target_channel and info.get("status") == "active"
        ]

        if remaining_versions_in_channel:
            def semver_key(v):
                try:
                    return [int(x) for x in v.split(".")]
                except ValueError:
                    return [0]
            sorted_versions = sorted(remaining_versions_in_channel, key=semver_key)
            data["channels"][target_channel]["latest_version"] = sorted_versions[-1]
        else:
            # Revert latest version indicator to previous stable if empty
            data["channels"][target_channel]["latest_version"] = ""

        temp_path = settings.MANIFEST_FILE.with_suffix(".tmp")
        with open(temp_path, "w") as f:
            json.dump(data, f, indent=2)
        os.replace(temp_path, settings.MANIFEST_FILE)
        logger.warning(f"Version {target_version} successfully deactivated in channel {target_channel}.")


@router.post("/status")
async def ota_status_report(request: Request, report: TelemetryReport):
    """Receives structured execution outcomes and enforces automatic rolling halts."""
    authenticate_request(request, "telemetry", request.headers.get(settings.API_KEY_HEADER))
    log_telemetry(report)
    evaluate_auto_rollback(report.target_version)
    return {"status": "recorded"}


@router.get("/check")
async def ota_check(
    request: Request,
    ver: str = Query(None, description="Backup query parameter for version"),
    hw: str = Query(None, description="Backup query parameter for hardware"),
    device_id: str = Query(None, description="Device UUID / MAC address"),
    channel: str = Query(None, description="Target deployment stream channel"),
    hsvn: int = Query(None, description="Hardware security version constraint")
):
    """Resolves and delivers updates considering canary profiles and security constraints."""
    api_key = request.headers.get(settings.API_KEY_HEADER)
    authenticate_request(request, "manifest.json", api_key)

    client_version = request.headers.get(settings.HEADER_VERSION_KEY) or ver
    client_hardware = request.headers.get(settings.HEADER_HARDWARE_KEY) or hw
    client_device_id = request.headers.get(settings.HEADER_DEVICE_ID_KEY) or device_id
    client_channel = request.headers.get(settings.HEADER_CHANNEL_KEY) or channel or settings.DEFAULT_CHANNEL
    
    raw_hsvn = request.headers.get(settings.HEADER_HSVN_KEY) or hsvn or settings.DEFAULT_HSVN
    try:
        client_hsvn = int(raw_hsvn)
    except (ValueError, TypeError):
        client_hsvn = settings.DEFAULT_HSVN

    if not client_version or not client_hardware or not client_device_id:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Missing identification keys. Required headers: "
                   f"'{settings.HEADER_VERSION_KEY}', '{settings.HEADER_HARDWARE_KEY}', '{settings.HEADER_DEVICE_ID_KEY}'."
        )

    if not settings.MANIFEST_FILE.exists():
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, 
            detail="No updates deployed on this server."
        )

    with open(settings.MANIFEST_FILE, "r") as f:
        try:
            manifest = json.load(f)
        except json.JSONDecodeError:
            raise HTTPException(
                status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, 
                detail="Corrupted update directory."
            )

    manifest_hardware = manifest.get("hardware_device", "")
    if client_hardware != manifest_hardware:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Incompatible hardware profile. Target: {client_hardware}, Required: {manifest_hardware}"
        )

    # Evaluate the active targeted release channel version
    latest_channel_version = manifest.get("channels", {}).get(client_channel, {}).get("latest_version")
    
    if latest_channel_version and is_newer_version(client_version, latest_channel_version):
        update_info = manifest.get("updates", {}).get(latest_channel_version)
        if not update_info or update_info.get("status") != "active":
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND, 
                detail="Version metadata unavailable or de-activated."
            )

        # Anti-Downgrade Check: Verify HSVN constraint
        target_hsvn = update_info.get("hsvn", settings.DEFAULT_HSVN)
        if client_hsvn > target_hsvn:
            logger.warning(
                f"Device '{client_device_id}' rejected downgrade target: Client HSVN is {client_hsvn}, "
                f"requested update target version HSVN is {target_hsvn}."
            )
            return {
                "update_available": False,
                "message": "Update aborted: Hardware anti-downgrade boundary protection active."
            }

        # Canary Target Calculation Check
        canary_percent = update_info.get("canary_percentage", settings.DEFAULT_CANARY_PERCENTAGE)
        if not is_in_canary_group(client_device_id, latest_channel_version, canary_percent):
            logger.info(f"Device '{client_device_id}' bypassed: Not targeted in {canary_percent}% canary rollout group.")
            return {
                "update_available": False,
                "message": "Device not targeted in staggered rollout cohort."
            }

        patches_map = update_info.get("patches", {})
        signing_cert_pem = get_signing_certificate_pem()
        
        # Scenario A: Deliver Delta patch
        if client_version in patches_map:
            logger.info(f"Target delta patch matched: Client version {client_version} -> {latest_channel_version}")
            patch_info = patches_map[client_version]
            patch_relative_path = patch_info.get("patch_path", "")
            patch_filename = f"patches/{Path(patch_relative_path).name}"
            
            token = generate_download_token(patch_filename)
            base_url = str(request.base_url).rstrip("/")
            download_url = f"{base_url}{DOWNLOAD_ROUTE_PREFIX}/{patch_filename}?token={token}"

            return {
                "update_available": True,
                "update_type": "delta",
                "version": latest_channel_version,
                "url": download_url,
                "size": patch_info.get("file_size_bytes"),
                "sha256": patch_info.get("sha256"),
                "signature": patch_info.get("signature"),
                "target_sha256": update_info.get("sha256"),
                "target_signature": update_info.get("signature"),
                "signing_cert": signing_cert_pem,
                "notes": update_info.get("release_notes")
            }

        # Scenario B: Deliver Complete binary fallback
        logger.info(f"Delivering full binary updates.")
        binary_path = update_info.get("binary_path", "")
        binary_name = Path(binary_path).name

        token = generate_download_token(binary_name)
        base_url = str(request.base_url).rstrip("/")
        download_url = f"{base_url}{DOWNLOAD_ROUTE_PREFIX}/{binary_name}?token={token}"

        return {
            "update_available": True,
            "update_type": "full",
            "version": latest_channel_version,
            "url": download_url,
            "size": update_info.get("file_size_bytes"),
            "sha256": update_info.get("sha256"),
            "signature": update_info.get("signature"),
            "target_sha256": update_info.get("sha256"),
            "target_signature": update_info.get("signature"),
            "signing_cert": signing_cert_pem,
            "notes": update_info.get("release_notes")
        }

    return {
        "update_available": False,
        "message": "Firmware is already up-to-date."
    }


@router.get("/download/{filename:path}")
async def ota_download(
    filename: str, 
    request: Request,
    token: str = Query(None, description="Temporary presigned query token")
):
    """Serves binary files dynamically using the presigned URL scheme."""
    authenticate_request(request, filename, token)
    return await get_validated_file_response(filename)


@direct_router.get(f"{DOWNLOAD_ROUTE_PREFIX}/{{filename:path}}")
async def direct_ota_download(
    filename: str, 
    request: Request,
    token: str = Query(None, description="Temporary presigned query token")
):
    """Serves binary files dynamically directly on the root storage path."""
    authenticate_request(request, filename, token)
    return await get_validated_file_response(filename)