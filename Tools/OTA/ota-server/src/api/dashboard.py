import hashlib
import json
import os
import logging
from fastapi import APIRouter, Request, UploadFile, File, Form, HTTPException, status
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates
from src.core.config import settings
from src.core.patch import generate_delta_patch
from src.core.security import sign_file

logger = logging.getLogger("uvicorn.error")
router = APIRouter()
templates = Jinja2Templates(directory=str(settings.TEMPLATES_DIR))


def calculate_sha256(file_path: str) -> str:
    """Helper function to calculate the SHA-256 hash of a file on disk."""
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as f:
        for byte_block in iter(lambda: f.read(settings.CHUNK_SIZE_BYTES), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()


def update_manifest(
    hardware: str, 
    version: str, 
    filename: str, 
    file_size: int, 
    sha256_hash: str, 
    release_notes: str,
    channel: str,
    hsvn: int,
    canary_percentage: int
):
    """Updates manifest.json and triggers generation of delta updates."""
    manifest_path = settings.MANIFEST_FILE
    settings.FIRMWARE_DIR.mkdir(parents=True, exist_ok=True)
    patches_dir = settings.FIRMWARE_DIR / "patches"
    patches_dir.mkdir(parents=True, exist_ok=True)

    data = {}
    if manifest_path.exists():
        with open(manifest_path, "r") as f:
            try:
                data = json.load(f)
            except json.JSONDecodeError:
                pass

    previous_version = data.get("channels", {}).get(channel, {}).get("latest_version")
    previous_bin_relative_path = None

    if previous_version and "updates" in data and previous_version in data["updates"]:
        previous_bin_relative_path = data["updates"][previous_version].get("binary_path")

    data["hardware_device"] = hardware
    
    if "channels" not in data:
        data["channels"] = {}
    
    data["channels"][channel] = {
        "latest_version": version
    }
    
    if "updates" not in data:
        data["updates"] = {}

    # Calculate ECDSA signature for full binary
    new_file_path = settings.FIRMWARE_DIR / filename
    full_signature = sign_file(new_file_path)

    data["updates"][version] = {
        "binary_path": f"/{settings.FIRMWARE_DIR.name}/{filename}",
        "file_size_bytes": file_size,
        "sha256": sha256_hash,
        "signature": full_signature,
        "minimum_required_loader_version": settings.DEFAULT_MIN_LOADER_VERSION,
        "hsvn": hsvn,
        "canary_percentage": canary_percentage,
        "channel": channel,
        "status": "active",  # active, revoked, soft-rolled-back
        "release_notes": release_notes,
        "patches": {}
    }

    # Generate Delta Patch from previous version to this new version if matching channels
    if previous_version and previous_bin_relative_path:
        previous_bin_name = os.path.basename(previous_bin_relative_path)
        base_file_path = settings.FIRMWARE_DIR / previous_bin_name
        patch_filename = f"patch_{previous_version}_to_{version}.bin"
        patch_file_path = patches_dir / patch_filename

        if base_file_path.exists():
            patch_success = generate_delta_patch(
                str(base_file_path), 
                str(new_file_path), 
                str(patch_file_path)
            )
            
            if patch_success:
                patch_size = patch_file_path.stat().st_size
                patch_hash = calculate_sha256(str(patch_file_path))
                patch_signature = sign_file(patch_file_path)
                
                data["updates"][version]["patches"][previous_version] = {
                    "patch_path": f"/{settings.FIRMWARE_DIR.name}/patches/{patch_filename}",
                    "file_size_bytes": patch_size,
                    "sha256": patch_hash,
                    "signature": patch_signature
                }
        else:
            logger.warning(f"Base firmware version {previous_version} missing. Skipping patch.")

    temp_path = manifest_path.with_suffix(".tmp")
    with open(temp_path, "w") as f:
        json.dump(data, f, indent=2)
    os.replace(temp_path, manifest_path)


@router.get("/", response_class=HTMLResponse)
async def get_dashboard(request: Request):
    """Renders the HTML administrative dashboard."""
    manifest_data = {}
    
    if settings.MANIFEST_FILE.exists():
        with open(settings.MANIFEST_FILE, "r") as f:
            try:
                manifest_data = json.load(f)
            except json.JSONDecodeError:
                pass

    return templates.TemplateResponse(
        request=request,
        name="index.html",
        context={
            "manifest": manifest_data,
            "project_name": settings.PROJECT_NAME
        }
    )


@router.post("/upload")
async def upload_firmware(
    hardware: str = Form(...),
    version: str = Form(...),
    release_notes: str = Form(""),
    channel: str = Form(settings.DEFAULT_CHANNEL),
    hsvn: int = Form(settings.DEFAULT_HSVN),
    canary_percentage: int = Form(settings.DEFAULT_CANARY_PERCENTAGE),
    file: UploadFile = File(...)
):
    """Handles multipart upload of binaries and calculates cryptographic hashes."""
    if not file.filename.endswith(settings.FIRMWARE_EXTENSION):
        raise HTTPException(
            status_code=400, 
            detail=f"Invalid file type. Only {settings.FIRMWARE_EXTENSION} files are accepted."
        )

    settings.FIRMWARE_DIR.mkdir(parents=True, exist_ok=True)
    
    file_path = settings.FIRMWARE_DIR / file.filename
    file_size = 0

    with open(file_path, "wb") as buffer:
        while chunk := await file.read(settings.CHUNK_SIZE_BYTES):
            buffer.write(chunk)
            file_size += len(chunk)

    sha256_hash = calculate_sha256(str(file_path))
    update_manifest(
        hardware, 
        version, 
        file.filename, 
        file_size, 
        sha256_hash, 
        release_notes,
        channel,
        hsvn,
        canary_percentage
    )

    return RedirectResponse(url="/", status_code=303)


@router.post("/delete/{version}")
async def delete_version(version: str):
    """Deletes the specified version metadata and cleans up assets."""
    manifest_path = settings.MANIFEST_FILE
    if not manifest_path.exists():
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, 
            detail="Manifest file not found."
        )

    with open(manifest_path, "r") as f:
        try:
            data = json.load(f)
        except json.JSONDecodeError:
            raise HTTPException(
                status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, 
                detail="Corrupted manifest file."
            )

    if "updates" not in data or version not in data["updates"]:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, 
            detail=f"Version {version} not found in manifest."
        )

    files_to_delete = []
    target_update = data["updates"][version]
    target_channel = target_update.get("channel", settings.DEFAULT_CHANNEL)
    
    bin_path_rel = target_update.get("binary_path")
    if bin_path_rel:
        bin_name = os.path.basename(bin_path_rel)
        files_to_delete.append(settings.FIRMWARE_DIR / bin_name)

    patches = target_update.get("patches", {})
    for _, patch_meta in patches.items():
        patch_path_rel = patch_meta.get("patch_path")
        if patch_path_rel:
            patch_name = os.path.basename(patch_path_rel)
            files_to_delete.append(settings.FIRMWARE_DIR / "patches" / patch_name)

    # Remove targeted update record
    del data["updates"][version]

    # Re-evaluate latest channel mappings dynamically
    remaining_versions_in_channel = [
        v for v, info in data.get("updates", {}).items() 
        if info.get("channel") == target_channel
    ]

    if "channels" not in data:
        data["channels"] = {}

    if remaining_versions_in_channel:
        def semver_key(v):
            try:
                return [int(x) for x in v.split(".")]
            except ValueError:
                return [0]
        sorted_versions = sorted(remaining_versions_in_channel, key=semver_key)
        data["channels"][target_channel] = {
            "latest_version": sorted_versions[-1]
        }
    else:
        if target_channel in data["channels"]:
            del data["channels"][target_channel]

    for file_path in files_to_delete:
        try:
            if file_path.exists():
                file_path.unlink()
                logger.info(f"Deleted local asset: {file_path}")
        except Exception as e:
            logger.error(f"Failed to delete file {file_path}: {e}")

    temp_path = manifest_path.with_suffix(".tmp")
    with open(temp_path, "w") as f:
        json.dump(data, f, indent=2)
    os.replace(temp_path, manifest_path)

    return {"success": True, "message": f"Version {version} and relative assets deleted."}