import hashlib
import json
import os
import logging
from fastapi import APIRouter, Request, UploadFile, File, Form, HTTPException, status
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates
from src.core.config import settings
from src.core.patch import generate_delta_patch

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


def update_manifest(hardware: str, version: str, filename: str, file_size: int, sha256_hash: str, release_notes: str):
    """Updates manifest.json and triggers generation of delta updates using the patch engine module."""
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

    # Record the previous version before updating the root
    previous_version = data.get("latest_version")
    previous_bin_relative_path = None

    if previous_version and "updates" in data and previous_version in data["updates"]:
        previous_bin_relative_path = data["updates"][previous_version].get("binary_path")

    data["hardware_device"] = hardware
    data["latest_version"] = version
    
    if "updates" not in data:
        data["updates"] = {}

    # Initialize the new version dictionary
    data["updates"][version] = {
        "binary_path": f"/{settings.FIRMWARE_DIR.name}/{filename}",
        "file_size_bytes": file_size,
        "sha256": sha256_hash,
        "minimum_required_loader_version": settings.DEFAULT_MIN_LOADER_VERSION,
        "release_notes": release_notes,
        "patches": {} # Placeholder for backward compatibility delta mappings
    }

    # Generate Delta Patch from previous version to this new version
    if previous_version and previous_bin_relative_path:
        previous_bin_name = os.path.basename(previous_bin_relative_path)
        base_file_path = settings.FIRMWARE_DIR / previous_bin_name
        new_file_path = settings.FIRMWARE_DIR / filename
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
                
                # Write patch metadata into the newly uploaded version
                data["updates"][version]["patches"][previous_version] = {
                    "patch_path": f"/{settings.FIRMWARE_DIR.name}/patches/{patch_filename}",
                    "file_size_bytes": patch_size,
                    "sha256": patch_hash
                }
        else:
            logger.warning(f"Base firmware version {previous_version} missing from disk. Skipping patch creation.")

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
    update_manifest(hardware, version, file.filename, file_size, sha256_hash, release_notes)

    return RedirectResponse(url="/", status_code=303)


@router.post("/delete/{version}")
async def delete_version(version: str):
    """Deletes the specified version metadata and cleans up its associated binary and patch files."""
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

    # 1. Capture primary binary of the version being deleted
    target_update = data["updates"][version]
    bin_path_rel = target_update.get("binary_path")
    if bin_path_rel:
        bin_name = os.path.basename(bin_path_rel)
        files_to_delete.append(settings.FIRMWARE_DIR / bin_name)

    # 2. Capture patches to this version (stored in target version's patch configuration)
    patches = target_update.get("patches", {})
    for prev_ver, patch_meta in patches.items():
        patch_path_rel = patch_meta.get("patch_path")
        if patch_path_rel:
            patch_name = os.path.basename(patch_path_rel)
            files_to_delete.append(settings.FIRMWARE_DIR / "patches" / patch_name)

    # 3. Capture and clear external patches originating from this version inside newer updates
    for other_ver, other_data in list(data["updates"].items()):
        if other_ver == version:
            continue
        other_patches = other_data.get("patches", {})
        if version in other_patches:
            patch_path_rel = other_patches[version].get("patch_path")
            if patch_path_rel:
                patch_name = os.path.basename(patch_path_rel)
                files_to_delete.append(settings.FIRMWARE_DIR / "patches" / patch_name)
            del other_data["patches"][version]

    # 4. Remove target update configuration
    del data["updates"][version]

    # 5. Re-evaluate the latest_version dynamically
    remaining_versions = list(data["updates"].keys())
    if remaining_versions:
        def semver_key(v):
            try:
                return [int(x) for x in v.split(".")]
            except ValueError:
                return [0]
        sorted_versions = sorted(remaining_versions, key=semver_key)
        data["latest_version"] = sorted_versions[-1]
    else:
        data["latest_version"] = ""

    # 6. Delete all verified file assets from disk
    for file_path in files_to_delete:
        try:
            if file_path.exists():
                file_path.unlink()
                logger.info(f"Successfully deleted local file: {file_path}")
        except Exception as e:
            logger.error(f"Failed to delete file {file_path}: {e}")

    # 7. Write clean state back to manifest.json
    temp_path = manifest_path.with_suffix(".tmp")
    with open(temp_path, "w") as f:
        json.dump(data, f, indent=2)
    os.replace(temp_path, manifest_path)

    return {"success": True, "message": f"Version {version} and relative assets deleted."}