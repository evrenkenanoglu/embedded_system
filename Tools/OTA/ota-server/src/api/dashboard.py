"""
@file       dashboard.py
@brief      Web dashboard API endpoints reading from decoupled release-catalog.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import json
import logging
import os
import shutil
import subprocess
import sys
from pathlib import Path
from fastapi import APIRouter, Request, UploadFile, File, Form, HTTPException, status
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates

from src.core.config import settings
from src.core.manifest_schema import OtaManifest

logger = logging.getLogger("uvicorn.error")
router = APIRouter()
templates = Jinja2Templates(directory=str(settings.TEMPLATES_DIR))


def load_manifest() -> OtaManifest:
    if settings.MANIFEST_FILE.exists():
        try:
            with open(settings.MANIFEST_FILE, "r", encoding="utf-8") as f:
                return OtaManifest.model_validate(json.load(f))
        except Exception as exc:
            logger.error(f"Error loading manifest: {exc}")
    return OtaManifest()


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
    target_signature: str = Form(""),
    file: UploadFile = File(...),
):
    """Saves uploaded binary and invokes the decoupled diff_worker.py to update release-catalog."""
    if not file.filename.endswith(settings.FIRMWARE_EXTENSION):
        raise HTTPException(status_code=400, detail="Only .bin files are accepted.")

    settings.BINARIES_DIR.mkdir(parents=True, exist_ok=True)
    temp_upload_path = settings.BINARIES_DIR / f"temp_{file.filename}"

    with open(temp_upload_path, "wb") as buffer:
        while chunk := await file.read(settings.CHUNK_SIZE_BYTES):
            buffer.write(chunk)

    diff_worker_script = settings.CATALOG_DIR / "diff_worker.py"
    if not diff_worker_script.exists():
        raise HTTPException(status_code=500, detail=f"diff_worker.py not found at {diff_worker_script}")

    # Invoke standalone diff_worker to register release and generate 1-hop diff
    cmd = [
        sys.executable,
        str(diff_worker_script),
        "--catalog-dir", str(settings.CATALOG_DIR),
        "--binary", str(temp_upload_path),
        "--version", version,
        "--signature", target_signature,
        "--hardware", hardware,
        "--channel", channel,
        "--hsvn", str(hsvn),
        "--canary", str(canary_percentage),
        "--notes", release_notes,
    ]

    try:
        subprocess.run(cmd, check=True)
    finally:
        if temp_upload_path.exists():
            temp_upload_path.unlink()

    return RedirectResponse(url="/", status_code=303)


@router.post("/delete/{version}")
async def delete_version(version: str):
    """Deletes release metadata and binary assets from release-catalog."""
    manifest = load_manifest()
    if version not in manifest.releases:
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Version not found.")

    target_release = manifest.releases[version]
    target_channel = target_release.channel

    # Delete binary file
    bin_file = settings.BINARIES_DIR / target_release.binary.file_name
    if bin_file.exists():
        bin_file.unlink()

    # Delete patch files
    for _, patch_info in target_release.patches.items():
        patch_file = settings.CATALOG_DIR / patch_info.file_name
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

    # Save manifest
    temp_path = settings.MANIFEST_FILE.with_suffix(".tmp")
    with open(temp_path, "w", encoding="utf-8") as f:
        json.dump(manifest.model_dump(), f, indent=4)
    shutil.move(temp_path, settings.MANIFEST_FILE)

    return {"success": True, "message": f"Version {version} deleted."}
