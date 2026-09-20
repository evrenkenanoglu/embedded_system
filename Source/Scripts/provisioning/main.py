#!/usr/bin/env python3
"""
@file       main.py
@brief      Generic data-driven orchestrator driven entirely by project config.yaml.
            Coordinates NVS encryption, detached code-signing, and physical eFuse silicon locking.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import argparse
import sys
from pathlib import Path
from typing import Dict, Any, Tuple, Optional, List

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

import yaml
from factory.audit_logger import AuditLogger
from factory.partition_parser import PartitionTableParser
from factory.provision_hardware import (
    read_chip_mac,
    burn_efuse_key,
    protect_efuse_key,
    burn_efuse_register,
    flash_dynamic_layout
)
from nvs.partition_nvs_generator import (
    generate_nvs_keys,
    render_template_csv,
    invoke_nvs_partition_gen
)
from signing.hsm_sign_digest import (
    compute_sha256,
    sign_digest_ec_secp256r1,
    sign_digest_rsa_2048
)
from signing.ota_manifest_packager import main as package_manifest_entry


def auto_detect_roots() -> Tuple[Path, Path]:
    """
    Auto-detects (project_root, embedded_system_root) by traversing upward from SCRIPT_DIR.
    Structure: <project_root>/embedded_system/Source/Scripts/provisioning
    """
    curr = SCRIPT_DIR
    embedded_system_root = None
    project_root = None

    while curr != curr.parent:
        if curr.name == "embedded_system" or ((curr / "Source").exists() and (curr / "CMakeLists.txt").exists()):
            embedded_system_root = curr
            project_root = curr.parent
            break
        curr = curr.parent

    if not embedded_system_root:
        if len(SCRIPT_DIR.parents) >= 3:
            embedded_system_root = SCRIPT_DIR.parents[2]
            project_root = SCRIPT_DIR.parents[3]
        else:
            embedded_system_root = Path.cwd()
            project_root = Path.cwd()

    return project_root.resolve(), embedded_system_root.resolve()


def expand_variables(data: Any, env_map: Dict[str, str]) -> Any:
    """Recursively replaces {VARIABLE} placeholders in config data."""
    if isinstance(data, str):
        for key, val in env_map.items():
            data = data.replace(f"{{{key}}}", str(val))
        return data
    elif isinstance(data, dict):
        return {k: expand_variables(v, env_map) for k, v in data.items()}
    elif isinstance(data, list):
        return [expand_variables(item, env_map) for item in data]
    return data


def load_config(config_path: Path, cli_project_root: Optional[Path] = None) -> Tuple[Dict[str, Any], Path, Tuple[Path, Path]]:
    """Loads pre-resolved SSoT configuration and returns configuration tuple (cfg, config_dir, roots)."""
    if not config_path.exists():
        candidate = (Path.cwd() / config_path).resolve()
        if candidate.exists():
            config_path = candidate
        else:
            raise FileNotFoundError(f"Configuration file missing: {config_path}")

    with open(config_path, "r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f) or {}

    auto_proj_root, auto_embed_root = auto_detect_roots()
    project_root = cli_project_root.resolve() if cli_project_root else auto_proj_root
    embedded_system_root = auto_embed_root
    roots = (project_root, embedded_system_root)

    return cfg, config_path.parent.resolve(), roots


def find_partitions_csv(configured_path_str: str, roots: Tuple[Path, Path]) -> Path:
    """Finds partitions.csv checking configured path and standard fallback roots."""
    configured_path = Path(configured_path_str).resolve()
    if configured_path.exists():
        return configured_path

    project_root, embedded_system_root = roots
    candidates = [
        project_root / "partitions.csv",
        embedded_system_root / "partitions.csv",
        embedded_system_root / "Source" / "partitions.csv",
        Path.cwd() / "partitions.csv"
    ]

    for candidate in candidates:
        if candidate.exists():
            return candidate.resolve()

    raise FileNotFoundError(
        f"Partition table CSV missing: {configured_path}\n"
        f"Checked candidates:\n" + "\n".join(f"  - {c}" for c in candidates)
    )


def step_generate_nvs(cfg: Dict[str, Any], parser: PartitionTableParser) -> Path:
    print("\n" + "=" * 60)
    print(" STEP 1: GENERATING ENCRYPTED NVS PARTITION")
    print("=" * 60)

    paths_cfg = cfg.get("paths", {})
    nvs_cfg = cfg.get("nvs_generation", {})

    target_name = nvs_cfg["target_partition"]
    partition_size = parser.get_size(target_name)

    template_file = Path(nvs_cfg["template_csv"]).resolve()
    output_dir = Path(paths_cfg["output_dir"]).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    rendered_csv = output_dir / f"nvs_{target_name}_rendered.csv"
    nvs_key_file = Path(nvs_cfg["output_key_bin"]).resolve()
    nvs_bin_file = Path(nvs_cfg["output_encrypted_bin"]).resolve()
    project_root = Path(paths_cfg.get("workspace_dir", ".")).resolve()

    print(f"[*] Target Partition : '{target_name}' (Size: {hex(partition_size)} / {partition_size} bytes)")
    print(f"[*] Template File    : {template_file}")

    if not nvs_key_file.exists():
        print(f"[*] Generating new NVS key: {nvs_key_file}")
        generate_nvs_keys(nvs_key_file)

    render_template_csv(template_file, rendered_csv, nvs_cfg["template_variables"], project_root)
    print(f"[OK] Rendered CSV written to: {rendered_csv}")

    if not invoke_nvs_partition_gen(rendered_csv, nvs_bin_file, partition_size, nvs_key_file) or not nvs_bin_file.exists():
        raise RuntimeError("Failed to generate encrypted NVS binary.")

    print(f"[SUCCESS] Encrypted NVS partition binary created: {nvs_bin_file}")
    return nvs_bin_file


def step_sign_release(cfg: Dict[str, Any]) -> str:
    print("\n" + "=" * 60)
    print(" STEP 2: CODE SIGNING & RELEASE MANIFEST PACKAGING")
    print("=" * 60)

    paths_cfg = cfg["paths"]
    signing_cfg = cfg["signing"]
    manifest_meta = signing_cfg.get("manifest_metadata", {})

    fw_dir = Path(paths_cfg["firmware_dir"]).resolve()
    fw_path = (fw_dir / signing_cfg["binary_name"]).resolve()
    signing_key = Path(signing_cfg["signing_key_pem"]).resolve()
    signing_cert = Path(signing_cfg["signing_cert_pem"]).resolve()
    manifest_path = Path(paths_cfg["server_manifest"]).resolve()

    digest = compute_sha256(fw_path)
    print(f"[*] Binary SHA-256 Digest: {digest.hex()}")

    with open(signing_key, "rb") as f:
        key_bytes = f.read()

    raw_sig = sign_digest_ec_secp256r1(digest, key_bytes) if signing_cfg["key_type"] == "ec-secp256r1" else sign_digest_rsa_2048(digest, key_bytes)
    sig_hex = raw_sig.hex()
    print(f"[*] Target Signature ({signing_cfg['key_type']}): {sig_hex}")

    if "out_signature_bin" in signing_cfg:
        out_sig_file = Path(signing_cfg["out_signature_bin"]).resolve()
        out_sig_file.parent.mkdir(parents=True, exist_ok=True)
        with open(out_sig_file, "wb") as f:
            f.write(raw_sig)

    sys.argv = [
        "ota_manifest_packager.py",
        "--manifest", str(manifest_path.resolve()),
        "--version", str(signing_cfg["version"]),
        "--binary", str(fw_path.resolve()),
        "--signature", sig_hex,
        "--signing-cert", str(signing_cert.resolve()),
        "--channel", str(manifest_meta.get("channel", "stable")),
        "--hsvn", str(manifest_meta.get("target_hsvn", 1)),
        "--hardware", str(manifest_meta.get("hardware_device", "ESP32-S3-WROOM"))
    ]
    if package_manifest_entry() != 0:
        raise RuntimeError("Failed to update manifest catalog.")

    print(f"[SUCCESS] Release v{signing_cfg['version']} packaged into server manifest.")
    return sig_hex


def step_provision_hardware(cfg: Dict[str, Any], parser: PartitionTableParser, override_dry_run: Optional[bool] = None) -> None:
    print("\n" + "=" * 60)
    print(" STEP 3: SILICON FACTORY PROVISIONING & FLASHING")
    print("=" * 60)

    paths_cfg = cfg.get("paths", {})
    hw_cfg = cfg.get("hardware", {})

    # 1. Resolve communication port and baud rate safely
    port = hw_cfg.get("port") or hw_cfg.get("default_port", "/dev/ttyUSB0")
    if isinstance(port, str) and port.startswith("{"):
        port = "/dev/ttyUSB0"

    raw_baud = hw_cfg.get("baud") or hw_cfg.get("flash_baud", 460800)
    baud = int(raw_baud)

    chip = hw_cfg.get("chip", "esp32s3")

    # 2. Resolve flash geometry with defensive fallbacks against unexpanded placeholders
    flash_mode = hw_cfg.get("flash_mode", "dio")
    if not flash_mode or flash_mode.startswith("{"):
        flash_mode = "dio"

    flash_freq = hw_cfg.get("flash_freq", "80m")
    if not flash_freq or flash_freq.startswith("{"):
        flash_freq = "80m"

    flash_size = hw_cfg.get("flash_size", "8MB")
    if not flash_size or flash_size.startswith("{"):
        flash_size = "8MB"

    dry_run = hw_cfg.get("dry_run", True) if override_dry_run is None else override_dry_run
    if not dry_run and not hw_cfg.get("force_burn", False):
        raise RuntimeError("Set 'hardware.force_burn: true' in config.yaml to execute on real silicon.")

    binary_mapping: Dict[str, Path] = {}
    for target_name, path_str in hw_cfg.get("flash_targets", {}).items():
        binary_mapping[target_name] = Path(path_str).resolve()

    mac_addr = read_chip_mac(port, baud) if not dry_run else "AA-BB-CC-DD-EE-FF"

    print(f"[*] Port : {port} @ {baud} baud")
    print(f"[*] MAC  : {mac_addr}")
    print(f"[*] Mode : {'DRY-RUN' if dry_run else 'REAL SILICON FLASH'}")

    # 3. Burn cryptographic keys into physical eFuse blocks
    for efuse_k in hw_cfg.get("efuse_keys", []):
        key_path = Path(efuse_k["key_file"]).resolve()
        block = efuse_k["block"]
        purpose = efuse_k["purpose"]
        burn_efuse_key(port, baud, block, key_path, purpose, dry_run)

        # 4. Enforce permanent hardware protection locks
        # Flash Encryption key must be read-protected (hardware AES only) and write-protected
        # Secure Boot digest must be write-protected (public digest remains readable)
        read_protect = efuse_k.get("read_protect", (purpose == "FLASH_ENCRYPTION"))
        write_protect = efuse_k.get("write_protect", True)
        protect_efuse_key(port, baud, block, read_protect, write_protect, dry_run)

    # 5. Burn anti-rollback monotonic counters and silicon security registers
    for reg in hw_cfg.get("efuse_registers", []):
        burn_efuse_register(port, baud, reg["name"], str(reg["value"]), dry_run)

    # 6. Flash dynamic partition layout
    flash_dynamic_layout(
        port, baud, chip, flash_mode,
        flash_freq, flash_size, parser, binary_mapping, dry_run
    )

    # 7. Extract specific key files by purpose for audit trail
    flash_key_file = "NONE"
    sb_key_file = "NONE"
    for k in hw_cfg.get("efuse_keys", []):
        if k.get("purpose") == "FLASH_ENCRYPTION":
            flash_key_file = str(k.get("key_file", "NONE"))
        elif k.get("purpose") == "SECURE_BOOT_DIGEST0":
            sb_key_file = str(k.get("key_file", "NONE"))

    # 8. Capture permanent audit log
    audit_dir = Path(paths_cfg.get("audit_dir", "build/audit_logs")).resolve()
    audit_logger = AuditLogger(audit_dir)
    audit_file = audit_logger.record_provisioning_event(
        mac_address=mac_addr,
        device_id=cfg.get("nvs_generation", {}).get("template_variables", {}).get("DEVICE_ID", "UNKNOWN"),
        hsvn=int(cfg.get("nvs_generation", {}).get("template_variables", {}).get("HSVN", 1)),
        flash_key_file=flash_key_file,
        sb_key_file=sb_key_file,
        nvs_key_file=str(cfg.get("nvs_generation", {}).get("output_key_bin", "NONE")),
        status="PROVISIONED_SUCCESS" if not dry_run else "DRY_RUN_SUCCESS"
    )

    print(f"[SUCCESS] Unit {mac_addr} provisioned. Audit record: {audit_file}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generic ESP-IDF Provisioning Orchestrator.")
    parser.add_argument("--config", "-c", type=Path, default=Path("Source/Scripts/provisioning/config.yaml"), help="Path to config.yaml")
    parser.add_argument("--project-root", "-r", type=Path, default=None, help="Explicit project root directory override")
    parser.add_argument("--step", "-s", choices=["all", "nvs", "sign", "provision"], default="all", help="Target step")
    parser.add_argument("--dry-run", action="store_true", help="Force dry-run mode")
    args = parser.parse_args()

    try:
        config, config_dir, roots = load_config(args.config, args.project_root)

        partitions_csv_path = find_partitions_csv(config["paths"]["partitions_csv"], roots)

        # Dynamic SSoT resolution of flash offsets
        pt_offset_raw = config.get("hardware", {}).get("partition_table_offset", "0x10000")
        pt_offset = int(pt_offset_raw, 0)
        boot_offset = 0x0000 if config.get("hardware", {}).get("chip", "esp32s3") == "esp32s3" else 0x1000

        pt_parser = PartitionTableParser(
            partitions_csv_path=partitions_csv_path,
            partition_table_offset=pt_offset,
            bootloader_offset=boot_offset
        )

        if args.step in ["all", "nvs"]:
            step_generate_nvs(config, pt_parser)

        if args.step in ["all", "sign"]:
            step_sign_release(config)

        if args.step in ["all", "provision"]:
            override_dry_run = True if args.dry_run else None
            step_provision_hardware(config, pt_parser, override_dry_run)

        return 0

    except Exception as e:
        print(f"\n[FATAL ERROR] {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())