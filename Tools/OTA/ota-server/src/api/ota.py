import hmac
import hashlib
import json
import time
import logging
from pathlib import Path
from fastapi import APIRouter, HTTPException, Query, Request, status
from fastapi.responses import FileResponse
from src.core.config import settings

logger = logging.getLogger("uvicorn.error")

# Router for standard API-prefixed endpoints
router = APIRouter()

# Router for root-level direct download endpoints
direct_router = APIRouter()

# Resolve the URL route segment dynamically based on the configured directory name
DOWNLOAD_ROUTE_PREFIX = f"/{settings.FIRMWARE_DIR.name}"


def is_newer_version(current: str, latest: str) -> bool:
    """Compares two semantic version strings.

    Returns True if latest is newer than current.
    """
    try:
        curr_parts = [int(x) for x in current.split(".")]
        late_parts = [int(x) for x in latest.split(".")]
        return late_parts > curr_parts
    except (ValueError, AttributeError):
        return latest != current


def generate_download_token(filename: str, expires_in_sec: int = 300) -> str:
    """Generates a secure, cryptographically signed, time-limited download token.
    
    Defaults to a 5-minute (300-second) expiration window.
    """
    expire_time = int(time.time()) + expires_in_sec
    message = f"{filename}:{expire_time}".encode("utf-8")
    
    # Generate signature using SHA-256 HMAC keyed with the API security key
    signature = hmac.new(
        settings.API_KEY.encode("utf-8"), 
        message, 
        hashlib.sha256
    ).hexdigest()
    
    return f"{expire_time}.{signature}"


def verify_download_token(filename: str, token: str) -> bool:
    """Verifies the validity and expiration window of a cryptographically signed token."""
    try:
        expire_str, signature = token.split(".", 1)
        expire_time = int(expire_str)
        
        # 1. Assert token has not expired
        if time.time() > expire_time:
            logger.warning(f"Presigned token for '{filename}' has expired.")
            return False
            
        # 2. Re-evaluate signature matches using constant-time comparison
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
    """Authorizes the request via either standard HTTP headers or a signed query token."""
    api_key = request.headers.get(settings.API_KEY_HEADER)
    
    # 1. Authorize via valid administrative key in headers
    if api_key and api_key == settings.API_KEY:
        return

    # 2. Authorize via presigned query token
    if token and verify_download_token(filename, token):
        return

    # If both authentication vectors fail
    logger.warning(f"Unauthorized access attempt to download '{filename}'")
    raise HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Invalid, missing, or expired download credentials."
    )


async def get_validated_file_response(filename: str) -> FileResponse:
    """Resolves, checks for directory traversal, and returns a binary file stream."""
    file_path = (settings.FIRMWARE_DIR / filename).resolve()

    # Enforce directory traversal guard
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


@router.get("/check")
async def ota_check(
    request: Request,
    ver: str = Query(None, description="Backup query parameter for version"),
    hw: str = Query(None, description="Backup query parameter for hardware")
):
    """Firmware availability checker configured entirely by dynamic settings."""
    api_key = request.headers.get(settings.API_KEY_HEADER)

    # Diagnostic log
    logger.info(f"[DEBUG] Incoming Check Headers -> API-Key: {api_key}, Version: {request.headers.get(settings.HEADER_VERSION_KEY)}, Hardware: {request.headers.get(settings.HEADER_HARDWARE_KEY)}")
    
    authenticate_request(request, "manifest.json", api_key)

    client_version = request.headers.get(settings.HEADER_VERSION_KEY) or ver
    client_hardware = request.headers.get(settings.HEADER_HARDWARE_KEY) or hw

    if not client_version or not client_hardware:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Missing identification parameters. Provide '{settings.HEADER_VERSION_KEY}' and '{settings.HEADER_HARDWARE_KEY}' headers or query parameters."
        )

    if not settings.MANIFEST_FILE.exists():
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, 
            detail="No firmware updates available on this server."
        )

    with open(settings.MANIFEST_FILE, "r") as f:
        try:
            manifest = json.load(f)
        except json.JSONDecodeError:
            raise HTTPException(
                status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, 
                detail="Corrupted update database."
            )

    manifest_hardware = manifest.get("hardware_device", "")
    if client_hardware != manifest_hardware:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail=f"Incompatible hardware signature. Received: {client_hardware}, Required: {manifest_hardware}"
        )

    latest_version = manifest.get("latest_version", "")
    
    if is_newer_version(client_version, latest_version):
        update_info = manifest.get("updates", {}).get(latest_version)
        if not update_info:
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND, 
                detail="Target binary metadata mismatch."
            )

        # 1. Check if a pre-generated delta patch exists for the client's current version
        patches_map = update_info.get("patches", {})
        
        if client_version in patches_map:
            logger.info(f"Target delta patch found for client version {client_version} -> {latest_version}")
            patch_info = patches_map[client_version]
            patch_relative_path = patch_info.get("patch_path", "")
            patch_filename = f"patches/{Path(patch_relative_path).name}"
            
            token = generate_download_token(patch_filename)
            base_url = str(request.base_url).rstrip("/")
            download_url = f"{base_url}{DOWNLOAD_ROUTE_PREFIX}/{patch_filename}?token={token}"

            return {
                "update_available": True,
                "update_type": "delta",
                "version": latest_version,
                "url": download_url,
                "size": patch_info.get("file_size_bytes"),
                "sha256": patch_info.get("sha256"),          # SHA-256 of the patch binary itself
                "target_sha256": update_info.get("sha256"),   # SHA-256 of final reconstructed binary
                "notes": update_info.get("release_notes")
            }

        # 2. Fallback to Full update if no patch exists
        logger.info(f"No matching delta patch found for version {client_version}. Delivering full binary.")
        binary_path = update_info.get("binary_path", "")
        binary_name = Path(binary_path).name

        token = generate_download_token(binary_name)
        base_url = str(request.base_url).rstrip("/")
        download_url = f"{base_url}{DOWNLOAD_ROUTE_PREFIX}/{binary_name}?token={token}"

        return {
            "update_available": True,
            "update_type": "full",
            "version": latest_version,
            "url": download_url,
            "size": update_info.get("file_size_bytes"),
            "sha256": update_info.get("sha256"),
            "target_sha256": update_info.get("sha256"),
            "notes": update_info.get("release_notes")
        }

    return {
        "update_available": False,
        "message": "Firmware is already up-to-date."
    }


# Route 1: Legacy api-path download (/api/v1/ota/download/{filename:path})
@router.get("/download/{filename:path}")
async def ota_download(
    filename: str, 
    request: Request,
    token: str = Query(None, description="Temporary presigned query token")
):
    """Serves binary files dynamically using the presigned URL scheme (supports subdirectories)."""
    authenticate_request(request, filename, token)

    client_version = request.headers.get(settings.HEADER_VERSION_KEY)
    client_hardware = request.headers.get(settings.HEADER_HARDWARE_KEY)

    logger.info(
        f"API download transaction: '{filename}' "
        f"(Device ID: {client_hardware or 'Generic'}, Version: {client_version or 'Unknown'})"
    )
    return await get_validated_file_response(filename)


# Route 2: Dynamic direct download root endpoint (interpolates e.g., /firmware_storage/{filename:path})
@direct_router.get(f"{DOWNLOAD_ROUTE_PREFIX}/{{filename:path}}")
async def direct_ota_download(
    filename: str, 
    request: Request,
    token: str = Query(None, description="Temporary presigned query token")
):
    """Serves binary files dynamically directly on the root storage path (supports subdirectories)."""
    authenticate_request(request, filename, token)

    client_version = request.headers.get(settings.HEADER_VERSION_KEY)
    client_hardware = request.headers.get(settings.HEADER_HARDWARE_KEY)

    logger.info(
        f"Direct download transaction: '{filename}' "
        f"(Device ID: {client_hardware or 'Generic'}, Version: {client_version or 'Unknown'})"
    )
    return await get_validated_file_response(filename)