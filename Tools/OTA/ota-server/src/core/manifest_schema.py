"""
@file       manifest_schema.py
@brief      Canonical Data Models for OTA Release Manifests with bidirectional backward compatibility.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import time
from typing import Dict, Optional, Any
from pydantic import BaseModel, Field, model_validator


class BinaryArtifact(BaseModel):
    file_name: str
    size_bytes: int
    sha256: str
    signature: str


class DeltaPatch(BaseModel):
    file_name: str
    size_bytes: int
    sha256: str
    signature: str


class ReleaseEntry(BaseModel):
    hardware: str = Field(default="ESP32-C6-WROOM-1")
    channel: str = Field(default="stable")
    status: str = Field(default="active")  # active, revoked, soft-rolled-back
    hsvn: int = Field(default=1)
    canary_percentage: int = Field(default=100)
    min_loader_version: str = Field(default="1.0.0")
    binary: BinaryArtifact
    signing_cert: Optional[str] = None
    release_notes: str = Field(default="")
    patches: Dict[str, DeltaPatch] = Field(default_factory=dict)

    @model_validator(mode="before")
    @classmethod
    def normalize_release(cls, data: Any) -> Any:
        if not isinstance(data, dict):
            return data

        # Normalize binary artifact from flat or legacy keys
        if "binary" not in data:
            file_name = (
                data.get("file_name")
                or data.get("binary_path", "").lstrip("/").split("/")[-1]
                or "firmware.bin"
            )
            size_bytes = (
                data.get("size_bytes")
                or data.get("target_size")
                or data.get("file_size_bytes")
                or 0
            )
            sha256 = data.get("sha256") or data.get("target_hash") or ""
            signature = data.get("signature") or data.get("target_signature") or ""

            data["binary"] = {
                "file_name": file_name,
                "size_bytes": size_bytes,
                "sha256": sha256,
                "signature": signature,
            }

        # Normalize HSVN
        if "hsvn" not in data and "target_hsvn" in data:
            data["hsvn"] = data["target_hsvn"]

        # Normalize patches
        raw_patches = data.get("patches", {})
        norm_patches = {}
        for prev_ver, patch_info in raw_patches.items():
            if isinstance(patch_info, dict):
                p_file = (
                    patch_info.get("file_name")
                    or patch_info.get("patch_path", "").lstrip("/").split("/")[-1]
                )
                p_size = (
                    patch_info.get("size_bytes")
                    or patch_info.get("file_size_bytes")
                    or 0
                )
                p_hash = patch_info.get("sha256") or ""
                p_sig = patch_info.get("signature") or ""
                norm_patches[prev_ver] = {
                    "file_name": (
                        f"patches/{p_file}"
                        if not p_file.startswith("patches/")
                        else p_file
                    ),
                    "size_bytes": p_size,
                    "sha256": p_hash,
                    "signature": p_sig,
                }
        data["patches"] = norm_patches
        return data


class ChannelEntry(BaseModel):
    latest_version: str
    hardware: str = Field(default="ESP32-C6-WROOM-1")


class OtaManifest(BaseModel):
    schema_version: str = Field(default="1.0.0")
    project_name: str = Field(default="Embedded_IoT_BT_WIFI_Base_Project")
    updated_at: int = Field(default_factory=lambda: int(time.time()))
    channels: Dict[str, ChannelEntry] = Field(default_factory=dict)
    releases: Dict[str, ReleaseEntry] = Field(default_factory=dict)

    @model_validator(mode="before")
    @classmethod
    def normalize_root(cls, data: Any) -> Any:
        if not isinstance(data, dict):
            return data

        # Ingest legacy "updates" key as "releases"
        if "releases" not in data and "updates" in data:
            data["releases"] = data["updates"]

        # Ingest root hardware_device into channels if missing
        root_hw = data.get("hardware_device", "ESP32-C6-WROOM-1")
        channels = data.get("channels", {})
        for ch_name, ch_info in channels.items():
            if isinstance(ch_info, dict) and "hardware" not in ch_info:
                ch_info["hardware"] = ch_info.get("hardware_device", root_hw)

        return data
