import json
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


def verify_api_key(api_key_header_val: str):
    """Asserts that the provided API key matches the server credentials."""
    if not api_key_header_val or api_key_header_val != settings.API_KEY:
        logger.warning(f"Unauthorized access attempt with API Key: '{api_key_header_val}'")
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Invalid or missing API validation token."
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
    # Enforce API security verification dynamically
    api_key = request.headers.get(settings.API_KEY_HEADER)
    verify_api_key(api_key)

    # Retrieve platform-agnostic identification configurations
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

        binary_path = update_info.get("binary_path", "")
        binary_name = Path(binary_path).name

        # Construct download URLs dynamically using path definitions
        base_url = str(request.base_url).rstrip("/")
        download_url = f"{base_url}{DOWNLOAD_ROUTE_PREFIX}/{binary_name}"

        return {
            "update_available": True,
            "version": latest_version,
            "url": download_url,
            "size": update_info.get("file_size_bytes"),
            "sha256": update_info.get("sha256"),
            "notes": update_info.get("release_notes")
        }

    return {
        "update_available": False,
        "message": "Firmware is already up-to-date."
    }


# Route 1: Legacy check-route-derived download (/api/v1/ota/download/{filename})
@router.get("/download/{filename}")
async def ota_download(filename: str, request: Request):
    """Serves binary files dynamically under the standard API prefix."""
    api_key = request.headers.get(settings.API_KEY_HEADER)
    verify_api_key(api_key)

    client_version = request.headers.get(settings.HEADER_VERSION_KEY)
    client_hardware = request.headers.get(settings.HEADER_HARDWARE_KEY)

    logger.info(
        f"API download transaction: '{filename}' "
        f"(Device ID: {client_hardware or 'Generic'}, Version: {client_version or 'Unknown'})"
    )
    return await get_validated_file_response(filename)


# Route 2: Dynamic direct download root endpoint (interpolates e.g., /firmware_storage/{filename})
@direct_router.get(f"{DOWNLOAD_ROUTE_PREFIX}/{{filename}}")
async def direct_ota_download(filename: str, request: Request):
    """Serves binary files dynamically on the root directory path."""
    api_key = request.headers.get(settings.API_KEY_HEADER)
    verify_api_key(api_key)

    client_version = request.headers.get(settings.HEADER_VERSION_KEY)
    client_hardware = request.headers.get(settings.HEADER_HARDWARE_KEY)

    logger.info(
        f"Direct download transaction: '{filename}' "
        f"(Device ID: {client_hardware or 'Generic'}, Version: {client_version or 'Unknown'})"
    )
    return await get_validated_file_response(filename)