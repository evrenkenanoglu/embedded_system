import json
from pathlib import Path
from fastapi import APIRouter, HTTPException, Query, Request
from fastapi.responses import FileResponse
from src.core.config import settings

router = APIRouter()


def is_newer_version(current: str, latest: str) -> bool:
    """Compares two semantic version strings (e.g., '1.0.0' and '1.0.1').

    Returns True if latest is newer than current.
    """
    try:
        curr_parts = [int(x) for x in current.split(".")]
        late_parts = [int(x) for x in latest.split(".")]
        return late_parts > curr_parts
    except (ValueError, AttributeError):
        return latest != current


@router.get("/check")
async def ota_check(
    request: Request,
    ver: str = Query(None, description="Backup query parameter for version"),
    hw: str = Query(None, description="Backup query parameter for hardware")
):
    """Firmware availability checker configured by dynamic headers in config.py."""
    # Retrieve platform-agnostic identification configurations
    client_version = request.headers.get(settings.HEADER_VERSION_KEY) or ver
    client_hardware = request.headers.get(settings.HEADER_HARDWARE_KEY) or hw

    if not client_version or not client_hardware:
        raise HTTPException(
            status_code=400,
            detail=f"Missing identification parameters. Provide '{settings.HEADER_VERSION_KEY}' and '{settings.HEADER_HARDWARE_KEY}' headers or query parameters."
        )

    if not settings.MANIFEST_FILE.exists():
        raise HTTPException(status_code=404, detail="No firmware updates available on this server.")

    with open(settings.MANIFEST_FILE, "r") as f:
        try:
            manifest = json.load(f)
        except json.JSONDecodeError:
            raise HTTPException(status_code=500, detail="Corrupted system update database.")

    manifest_hardware = manifest.get("hardware_device", "")
    if client_hardware != manifest_hardware:
        raise HTTPException(
            status_code=400,
            detail=f"Incompatible hardware signature. Received: {client_hardware}, Required: {manifest_hardware}"
        )

    latest_version = manifest.get("latest_version", "")
    
    if is_newer_version(client_version, latest_version):
        update_info = manifest.get("updates", {}).get(latest_version)
        if not update_info:
            raise HTTPException(status_code=404, detail="Target binary metadata mismatch.")

        binary_path = update_info.get("binary_path", "")
        binary_name = Path(binary_path).name

        base_url = str(request.base_url).rstrip("/")
        download_url = f"{base_url}/api/v1/ota/download/{binary_name}"

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


@router.get("/download/{filename}")
async def ota_download(filename: str):
    """Serves binary firmware files via dynamic chunked streaming."""
    file_path = (settings.FIRMWARE_DIR / filename).resolve()

    if not file_path.is_relative_to(settings.FIRMWARE_DIR.resolve()):
        raise HTTPException(status_code=403, detail="Access denied.")

    if not file_path.exists() or not file_path.is_file():
        raise HTTPException(status_code=404, detail="Firmware binary not found.")

    return FileResponse(
        path=file_path,
        media_type="application/octet-stream",
        filename=filename
    )