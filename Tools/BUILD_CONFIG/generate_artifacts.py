#!/usr/bin/env python3
"""
@file       generate_artifacts.py
@brief      Deterministic generator of partitions.csv, sdkconfig.hardware,
            project_version.cmake, and ota_generated_config.h from Master SSoT.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import sys
from pathlib import Path
from typing import Any, Dict
import yaml


def load_ssot(config_path: Path) -> Dict[str, Any]:
    """Loads and validates the Master SSoT config_project.yaml."""
    if not config_path.exists():
        raise FileNotFoundError(f"Master SSoT missing at: {config_path}")
    with open(config_path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def generate_partitions_csv(cfg: Dict[str, Any], out_path: Path) -> Path:
    """Generates aligned partitions.csv from flash_layout.partitions."""
    layout = cfg.get("flash_layout", {})
    partitions = layout.get("partitions", [])

    lines = [
        "# ESP-IDF Partition Table (Auto-generated from configs/config_project.yaml)",
        "# Name,            Type, SubType,  Offset,  Size,        Flags",
    ]

    for p in partitions:
        name = f"{p.get('name', '')},"
        raw_type = p.get("type", "")
        ptype = f"{hex(raw_type) if isinstance(raw_type, int) else raw_type},"

        raw_subtype = p.get("subtype", "")
        subtype = (
            f"{hex(raw_subtype) if isinstance(raw_subtype, int) else raw_subtype},"
        )

        raw_offset = p.get("offset")
        offset = (
            f"{hex(raw_offset) if isinstance(raw_offset, int) else (raw_offset or '')},"
        )

        raw_size = p.get("size", "")
        size = f"{hex(raw_size) if isinstance(raw_size, int) else raw_size},"

        flags = str(p.get("flags") or "").strip()

        line = f"{name:<18} {ptype:<6} {subtype:<10} {offset:<8} {size:<12} {flags}".rstrip()
        lines.append(line)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


def generate_sdkconfig_hardware(cfg: Dict[str, Any], out_path: Path) -> Path:
    """Generates Kconfig hardware overlay from SSoT with mandatory Development Mode safety defaults."""
    hw = cfg.get("hardware", {})
    sec = cfg.get("security", {})
    layout = cfg.get("flash_layout", {})

    flash_size = str(hw.get("flash_size", "8MB")).upper()
    flash_mode = str(hw.get("flash_mode", "dio")).lower()
    flash_freq = str(hw.get("flash_freq", "80m")).lower()
    baud = hw.get("monitor_baud", 115200)

    pt_offset = str(layout.get("partition_table_offset", "0x10000"))

    raw_keys_dir = cfg.get("paths", {}).get("keys_dir", "keys")
    clean_keys_dir = raw_keys_dir.replace("{paths.workspace_dir}/", "").replace(
        "{paths.workspace_dir}", "."
    )
    signing_key = f"{clean_keys_dir}/secure_boot_signing_key.pem".replace("./", "")

    hsvn = sec.get("hsvn", cfg.get("project", {}).get("version_number", 1))
    sb_scheme = str(sec.get("secure_boot_scheme", "rsa3072")).lower()

    lines = [
        "# ESP32 Hardware & Security Overlay (Auto-generated from configs/config_project.yaml)",
        f"CONFIG_ESPTOOLPY_FLASHSIZE_{flash_size}=y",
        f'CONFIG_ESPTOOLPY_FLASHSIZE="{flash_size}"',
        f"CONFIG_ESPTOOLPY_FLASHMODE_{flash_mode.upper()}=y",
        f'CONFIG_ESPTOOLPY_FLASHMODE="{flash_mode}"',
        f"CONFIG_ESPTOOLPY_FLASHFREQ_{flash_freq.upper()}=y",
        f'CONFIG_ESPTOOLPY_FLASHFREQ="{flash_freq}"',
        f"CONFIG_ESPTOOLPY_MONITOR_BAUD={baud}",
        "",
        "# Partition Table Linkage (Resolved from SSoT)",
        "CONFIG_PARTITION_TABLE_CUSTOM=y",
        'CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"',
        f"CONFIG_PARTITION_TABLE_OFFSET={pt_offset}",
        "",
        "# =========================================================",
        "# FLASH ENCRYPTION SETTINGS (PRODUCTION HARDENED)",
        "# =========================================================",
        "# Zero-Brick Safety Invariant: Hardware Development Mode Default",
        "CONFIG_SECURE_FLASH_ENC_ENABLED=y",
        "CONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT=y",
        "# CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE is not set",
        "CONFIG_SECURE_FLASH_UART_BOOTLOADER_ALLOW_CACHE=y",
        "CONFIG_SECURE_FLASH_REQUIRE_ALREADY_ENABLED=n",
        "",
        "# =========================================================",
        "# SECURE BOOT V2 SETTINGS",
        "# =========================================================",
        "",
        "# Hardware hardening & ROM lockdown:",
        "CONFIG_SECURE_BOOT_ALLOW_JTAG=y",
        "CONFIG_SECURE_BOOT_ALLOW_ROM_BASIC=y",
        "# CONFIG_SECURE_ENABLE_SECURE_ROM_DL_MODE is not set",
        "",
        "# Security Hardware Keys & Anti-Rollback (Resolved from SSoT)",
    ]

    if "ecdsa" in sb_scheme:
        lines.append("CONFIG_SECURE_SIGNED_APPS_ECDSA_V2_SCHEME=y")
        lines.append("# CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME is not set")
    else:
        lines.append("CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME=y")
        lines.append("# CONFIG_SECURE_SIGNED_APPS_ECDSA_V2_SCHEME is not set")

    lines.extend(
        [
            f'CONFIG_SECURE_BOOT_SIGNING_KEY="{signing_key}"',
            f"CONFIG_BOOTLOADER_APP_SECURE_VERSION={hsvn}",
        ]
    )

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


def generate_project_version_cmake(cfg: Dict[str, Any], out_path: Path) -> Path:
    """Generates project_version.cmake for native CMake inclusion."""
    proj = cfg.get("project", {})
    sec = cfg.get("security", {})

    ver = proj.get("version", "1.0.0-dev1")
    ver_num = sec.get("hsvn", proj.get("version_number", 1))

    lines = [
        "# Auto-generated from configs/config_project.yaml - DO NOT EDIT MANUALLY",
        f'set(PROJECT_VER "{ver}")',
        f"set(PROJECT_VER_NUMBER {ver_num})",
    ]

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


def generate_ota_header(cfg: Dict[str, Any], out_path: Path) -> Path:
    """Generates C/C++ macro header contract for OTA runtime parameters."""
    proj = cfg.get("project", {})
    hw = cfg.get("hardware", {})
    sec = cfg.get("security", {})
    net = cfg.get("network", {})
    ota = cfg.get("ota", {})

    routes = ota.get("routes", {})
    headers = ota.get("headers", {})
    transport = ota.get("transport", {})
    policy = ota.get("policy", {})
    storage = ota.get("storage", {})

    lines = [
        "/** @file       ota_generated_config.h",
        " *  @brief      Auto-generated OTA parameters from configs/config_project.yaml - DO NOT EDIT.",
        " *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved",
        " */",
        "",
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "// Firmware & Hardware Baseline",
        f'#define OTA_FIRMWARE_VERSION            "{proj.get("version", "1.0.0-dev1")}"',
        f'#define OTA_HARDWARE_MODEL              "{hw.get("model", "ESP32-S3-WROOM")}"',
        f'#define OTA_SECURE_VERSION              {proj.get("version_number", 1)}',
        f'#define OTA_DEFAULT_CHANNEL             "{ota.get("channel", "stable")}"',
        "",
        "// Network Gateway & Endpoints",
        f'#define OTA_DEFAULT_GATEWAY_URL         "{net.get("gateway_url", "https://192.168.1.100:8443")}"',
        f'#define OTA_API_CHECK_ROUTE             "{routes.get("check", "/api/v1/ota/check")}"',
        f'#define OTA_API_STATUS_ROUTE            "{routes.get("status", "/api/v1/ota/status")}"',
        f'#define OTA_DEFAULT_API_KEY             "{sec.get("api_key", "secure-device-token-factory-001")}"',
        "",
        "// HTTP Protocol Headers",
        f'#define OTA_HDR_API_KEY                 "{sec.get("api_key_header", "X-Device-API-Key")}"',
        f'#define OTA_HDR_VERSION                 "{headers.get("version", "x-ESP32-version")}"',
        f'#define OTA_HDR_HARDWARE                "{headers.get("hardware", "x-ESP32-hardware")}"',
        f'#define OTA_HDR_DEVICE_ID               "{headers.get("device_id", "x-ESP32-device-id")}"',
        f'#define OTA_HDR_CHANNEL                 "{headers.get("channel", "x-ESP32-channel")}"',
        f'#define OTA_HDR_HSVN                    "{headers.get("hsvn", "x-ESP32-hsvn")}"',
        "",
        "// Transport & Socket Limits",
        f'#define OTA_STREAM_CHUNK_SIZE           {transport.get("chunk_size", 8192)}',
        f'#define OTA_SOCKET_TIMEOUT_MS           {transport.get("timeout_ms", 30000)}',
        f'#define OTA_TELEMETRY_TIMEOUT_MS        {transport.get("telemetry_timeout_ms", 10000)}',
        "",
        "// Policy, Scheduling & Battery Thresholds",
        f'#define OTA_CHECK_INTERVAL_SEC          {policy.get("check_interval_sec", 86400)}',
        f'#define OTA_JITTER_RANGE_SEC            {policy.get("jitter_range_sec", 1800)}',
        f'#define OTA_MIN_BATTERY_PERCENT         {policy.get("min_battery_pct", 80)}',
        f'#define OTA_MAINTENANCE_START_HOUR      {policy.get("allowed_start_hour", 2)}',
        f'#define OTA_MAINTENANCE_END_HOUR        {policy.get("allowed_end_hour", 4)}',
        f'#define OTA_BYPASS_CONDITIONS           {1 if policy.get("bypass_conditions", False) else 0}',
        "",
        "// Storage Partition & Namespace Mapping",
        f'#define OTA_FACTORY_PARTITION_NAME      "{storage.get("fctry_partition", "fctry")}"',
        f'#define OTA_FACTORY_PKI_NAMESPACE       "{storage.get("pki_namespace", "sec_pki")}"',
        f'#define OTA_FACTORY_CFG_NAMESPACE       "{storage.get("cfg_namespace", "sys_cfg")}"',
        f'#define OTA_KEY_ROOT_CA                 "{storage.get("root_ca_key", "root_ca_pem")}"',
        f'#define OTA_KEY_ROOT_CA_BACKUP          "{storage.get("root_ca_backup_key", "root_ca_backup_pem")}"',
        f'#define OTA_KEY_CHECKPOINT              "{storage.get("checkpoint_key", "ota_chkpt")}"',
        f'#define OTA_KEY_ROLLBACK_DIAG           "{storage.get("rollback_diag_key", "ota_rollback")}"',
        "",
        "// Cryptographic Constraints",
        f'#define OTA_CRYPTO_ALGORITHM            "{sec.get("crypto_curve", "ec-secp256r1")}"',
        "#define OTA_SIGNATURE_RAW_LEN           64",
        "#define OTA_DIGEST_LEN                  32",
    ]

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Deterministic SSoT Build Artifacts Generator."
    )
    parser.add_argument(
        "--config", "-c", type=Path, required=True, help="Path to config_project.yaml"
    )
    parser.add_argument(
        "--workspace",
        "-w",
        type=Path,
        default=Path("."),
        help="Workspace root directory",
    )
    parser.add_argument(
        "--target",
        "-t",
        choices=["all", "partitions", "sdkconfig", "version", "ota_header"],
        default="all",
        help="Artifact target to generate",
    )
    args = parser.parse_args()

    cfg = load_ssot(args.config.resolve())
    ws = args.workspace.resolve()

    if args.target in ["all", "partitions"]:
        p = generate_partitions_csv(cfg, ws / "partitions.csv")
        print(f"✅ Generated: {p}")

    if args.target in ["all", "sdkconfig"]:
        p = generate_sdkconfig_hardware(cfg, ws / "sdkconfig.hardware")
        print(f"✅ Generated: {p}")

    if args.target in ["all", "version"]:
        p = generate_project_version_cmake(cfg, ws / "project_version.cmake")
        print(f"✅ Generated: {p}")

    if args.target in ["all", "ota_header"]:
        p = generate_ota_header(
            cfg, ws / "main" / "swConfig" / "ota_generated_config.h"
        )
        print(f"✅ Generated: {p}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
