import hashlib
import json
import os
from fastapi import APIRouter, Request, UploadFile, File, Form, HTTPException
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates
from src.core.config import settings

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
    """Safely updates the local manifest.json with the new upload metadata."""
    manifest_path = settings.MANIFEST_FILE
    settings.FIRMWARE_DIR.mkdir(parents=True, exist_ok=True)

    data = {}
    if manifest_path.exists():
        with open(manifest_path, "r") as f:
            try:
                data = json.load(f)
            except json.JSONDecodeError:
                pass

    data["hardware_device"] = hardware
    data["latest_version"] = version
    
    if "updates" not in data:
        data["updates"] = {}

    data["updates"][version] = {
        "binary_path": f"/firmware_storage/{filename}",
        "file_size_bytes": file_size,
        "sha256": sha256_hash,
        "minimum_required_loader_version": settings.DEFAULT_MIN_LOADER_VERSION,
        "release_notes": release_notes
    }

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