import hashlib
import json
import os
import time
import logging
from pathlib import Path
from fastapi import APIRouter, Request, UploadFile, File, Form, HTTPException, status
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates

from src.core.config import settings
from src.core.manifest_schema import OtaManifest, ReleaseEntry, ChannelEntry, BinaryArtifact
from src.core.patch import generate_delta_patch
from src.core.security import sign_file

logger = logging.getLogger("uvicorn.error")
router = APIRouter()
templates = Jinja2Templates(directory=str(settings.TEMPLATES_DIR))


def calculate_sha256(file_path: Path) -> str:
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as f:
        for byte_block in iter(lambda: f.read(settings.CHUNK_SIZE_BYTES), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()


def load_manifest() -> OtaManifest:
    if settings.MANIFEST_FILE.exists():
        try:
            with open(settings.MANIFEST_FILE, "r", encoding="utf-8") as f:
                raw = json.load(f)
                return OtaManifest.model_validate(raw)
        except Exception as exc:
            logger.error(f"Error loading manifest: {exc}")
    return OtaManifest()


def save_manifest(manifest: OtaManifest) -> None:
    manifest.updated_at = int(time.time())
    temp_path = settings.MANIFEST_FILE.with_suffix(".tmp")
    with open(temp_path, "w", encoding="utf-8") as f:
        json.dump(manifest.model_dump(), f, indent=2)
    os.replace(temp_path, settings.MANIFEST_FILE)


def update_manifest(
    hardware: str,
    version: str,
    filename: str,
    file_size: int,
    sha256_hash: str,
    release_notes: str,
    channel: str,
    hsvn: int,
    canary_percentage: int,
):
    settings.FIRMWARE_DIR.mkdir(parents=True, exist_ok=True)
    patches_dir = settings.FIRMWARE_DIR / "patches"
    patches_dir.mkdir(parents=True, exist_ok=True)

    manifest = load_manifest()

    previous_version = manifest.channels.get(channel, ChannelEntry(latest_version="", hardware=hardware)).latest_version
    new_file_path = settings.FIRMWARE_DIR / filename
    full_signature = sign_file(new_file_path)

    patches_map = {}
    if previous_version and previous_version in manifest.releases:
        prev_bin_name = manifest.releases[previous_version].binary.file_name
        base_file_path = settings.FIRMWARE_DIR / prev_bin_name
        patch_filename = f"patch_{previous_version}_to_{version}.bin"
        patch_file_path = patches_dir / patch_filename

        if base_file_path.exists():
            patch_success = generate_delta_patch(
                str(base_file_path), str(new_file_path), str(patch_file_path)
            )
            if patch_success:
                patch_size = patch_file_path.stat().st_size
                patch_hash = calculate_sha256(patch_file_path)
                patch_signature = sign_file(patch_file_path)

                patches_map[previous_version] = {
                    "file_name": f"patches/{patch_filename}",
                    "size_bytes": patch_size,
                    "sha256": patch_hash,
                    "signature": patch_signature,
                }

    # Register release entry
    manifest.releases[version] = ReleaseEntry(
        hardware=hardware,
        channel=channel,
        status="active",
        hsvn=hsvn,
        canary_percentage=canary_percentage,
        min_loader_version=settings.DEFAULT_MIN_LOADER_VERSION,
        binary=BinaryArtifact(
            file_name=filename,
            size_bytes=file_size,
            sha256=sha256_hash,
            signature=full_signature,
        ),
        release_notes=release_notes,
        patches=patches_map,
    )

    # Point channel latest to new version
    manifest.channels[channel] = ChannelEntry(latest_version=version, hardware=hardware)
    save_manifest(manifest)


@router.get("/", response_class=HTMLResponse)
async def get_dashboard(request: Request):
    manifest = load_manifest()
    return templates.TemplateResponse(
        request=request,
        name="index.html",
        context={"manifest": manifest.model_dump(), "project_name": settings.PROJECT_NAME},
    )


@router.post("/upload")
async def upload_firmware(
    hardware: str = Form(...),
    version: str = Form(...),
    release_notes: str = Form(""),
    channel: str = Form(settings.DEFAULT_CHANNEL),
    hsvn: int = Form(settings.DEFAULT_HSVN),
    canary_percentage: int = Form(settings.DEFAULT_CANARY_PERCENTAGE),
    file: UploadFile = File(...),
):
    if not file.filename.endswith(settings.FIRMWARE_EXTENSION):
        raise HTTPException(
            status_code=400,
            detail=f"Invalid file type. Only {settings.FIRMWARE_EXTENSION} files are accepted.",
        )

    settings.FIRMWARE_DIR.mkdir(parents=True, exist_ok=True)
    file_path = settings.FIRMWARE_DIR / file.filename
    file_size = 0

    with open(file_path, "wb") as buffer:
        while chunk := await file.read(settings.CHUNK_SIZE_BYTES):
            buffer.write(chunk)
            file_size += len(chunk)

    sha256_hash = calculate_sha256(file_path)
    update_manifest(
        hardware,
        version,
        file.filename,
        file_size,
        sha256_hash,
        release_notes,
        channel,
        hsvn,
        canary_percentage,
    )

    return RedirectResponse(url="/", status_code=303)


@router.post("/delete/{version}")
async def delete_version(version: str):
    manifest = load_manifest()

    if version not in manifest.releases:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND,
            detail=f"Version {version} not found in manifest.",
        )

    target_release = manifest.releases[version]
    target_channel = target_release.channel

    # Delete binary file
    bin_file = settings.FIRMWARE_DIR / target_release.binary.file_name
    if bin_file.exists():
        bin_file.unlink()

    # Delete patch files
    for _, patch_info in target_release.patches.items():
        patch_file = settings.FIRMWARE_DIR / patch_info.file_name
        if patch_file.exists():
            patch_file.unlink()

    del manifest.releases[version]

    # Recalculate channel latest
    remaining = [
        v for v, r in manifest.releases.items()
        if r.channel == target_channel and r.status == "active"
    ]
    if remaining:
        def semver_key(v):
            try:
                return [int(x) for x in v.split(".")]
            except ValueError:
                return [0]
        manifest.channels[target_channel].latest_version = sorted(remaining, key=semver_key)[-1]
    else:
        if target_channel in manifest.channels:
            del manifest.channels[target_channel]

    save_manifest(manifest)
    return {"success": True, "message": f"Version {version} deleted."}
