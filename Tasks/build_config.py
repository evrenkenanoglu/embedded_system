# ==============================================================================
# Build Configuration Artifacts Generator
# Domain: Deterministic Generation of partitions.csv, sdkconfig.hardware, project_version.cmake
# Master SSoT Reference: configs/config_project.yaml
# ==============================================================================

from pathlib import Path
from typing import Any, Dict
from invoke import Context, task
import yaml
from core import CONFIG


def _load_ssot_config(workspace_root: Path) -> Dict[str, Any]:
    """Loads configs/config_project.yaml Master SSoT file."""
    ssot_path = workspace_root / "configs" / "config_project.yaml"
    if not ssot_path.exists():
        raise FileNotFoundError(f"Master project configuration not found at: {ssot_path}")

    with open(ssot_path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def _generate_partitions_csv(workspace_root: Path, cfg: Dict[str, Any]) -> Path:
    """Generates aligned partitions.csv from flash_layout.partitions."""
    out_path = workspace_root / "partitions.csv"
    layout = cfg.get("flash_layout", {})
    partitions = layout.get("partitions", [])

    lines = [
        "# ESP-IDF Partition Table (Auto-generated from configs/config_project.yaml)",
        "# Name,            Type, SubType,  Offset,  Size,        Flags",
    ]

    for p in partitions:
        name = f"{p.get('name', '')},"
        
        # Format numeric or string types safely
        raw_type = p.get("type", "")
        ptype = f"{hex(raw_type) if isinstance(raw_type, int) else raw_type},"
        
        raw_subtype = p.get("subtype", "")
        subtype = f"{hex(raw_subtype) if isinstance(raw_subtype, int) else raw_subtype},"
        
        raw_offset = p.get("offset")
        offset = f"{hex(raw_offset) if isinstance(raw_offset, int) else (raw_offset or '')},"
        
        raw_size = p.get("size", "")
        size = f"{hex(raw_size) if isinstance(raw_size, int) else raw_size},"
        
        flags = str(p.get("flags") or "").strip()

        # Format with fixed column widths matching ESP-IDF tabular conventions
        line = f"{name:<18} {ptype:<6} {subtype:<10} {offset:<8} {size:<12} {flags}".rstrip()
        lines.append(line)

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


def _generate_sdkconfig_hardware(workspace_root: Path, cfg: Dict[str, Any]) -> Path:
    """Generates sdkconfig.hardware overlay from hardware & security invariants."""
    out_path = workspace_root / "sdkconfig.hardware"
    hw = cfg.get("hardware", {})
    sec = cfg.get("security", {})
    
    flash_size = str(hw.get("flash_size", "8MB")).upper()
    flash_mode = str(hw.get("flash_mode", "dio")).lower()
    flash_freq = str(hw.get("flash_freq", "80m")).lower()
    baud = hw.get("monitor_baud", 115200)

    # Resolve partition table offset dynamically (SSoT)
    pt_offset = str(hw.get("partition_table_offset", "0x10000"))

    # Resolve signing key path relative to workspace cleanly
    raw_keys_dir = cfg.get("paths", {}).get("keys_dir", "keys")
    clean_keys_dir = raw_keys_dir.replace("{paths.workspace_dir}/", "").replace("{paths.workspace_dir}", ".")
    signing_key = f"{clean_keys_dir}/secure_boot_signing_key.pem".replace("./", "")

    # Resolve anti-rollback version from security SSoT
    hsvn = sec.get("hsvn", cfg.get("project", {}).get("version_number", 1))

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
        "# Security Hardware Keys & Anti-Rollback (Resolved from SSoT)",
        f'CONFIG_SECURE_BOOT_SIGNING_KEY="{signing_key}"',
        f"CONFIG_BOOTLOADER_APP_SECURE_VERSION={hsvn}",
    ]

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


def _generate_project_version_cmake(workspace_root: Path, cfg: Dict[str, Any]) -> Path:
    """Generates project_version.cmake for native CMake inclusion."""
    out_path = workspace_root / "project_version.cmake"
    proj = cfg.get("project", {})
    sec = cfg.get("security", {})
    
    ver = proj.get("version", "1.0.0-dev1")
    ver_num = sec.get("hsvn", proj.get("version_number", 1))

    lines = [
        "# Auto-generated from configs/config_project.yaml - DO NOT EDIT MANUALLY",
        f'set(PROJECT_VER "{ver}")',
        f"set(PROJECT_VER_NUMBER {ver_num})",
    ]

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return out_path


# ==============================================================================
# CLI TASKS
# ==============================================================================

@task(help={"dry_run": "Print target paths without writing files"})
def partitions(c: Context, dry_run: bool = False) -> None:
    """Generate partitions.csv from Master SSoT (config_project.yaml)."""
    workspace = Path(getattr(CONFIG.paths, "workspace_dir", ".")).resolve()
    if dry_run:
        print(f"[DRY-RUN] Would generate: {workspace / 'partitions.csv'}")
        return
    cfg = _load_ssot_config(workspace)
    path = _generate_partitions_csv(workspace, cfg)
    print(f"✅ Generated: {path}")


@task(help={"dry_run": "Print target paths without writing files"})
def sdkconfig(c: Context, dry_run: bool = False) -> None:
    """Generate sdkconfig.hardware overlay from Master SSoT."""
    workspace = Path(getattr(CONFIG.paths, "workspace_dir", ".")).resolve()
    if dry_run:
        print(f"[DRY-RUN] Would generate: {workspace / 'sdkconfig.hardware'}")
        return
    cfg = _load_ssot_config(workspace)
    path = _generate_sdkconfig_hardware(workspace, cfg)
    print(f"✅ Generated: {path}")


@task(help={"dry_run": "Print target paths without writing files"})
def version(c: Context, dry_run: bool = False) -> None:
    """Generate project_version.cmake for CMake from Master SSoT."""
    workspace = Path(getattr(CONFIG.paths, "workspace_dir", ".")).resolve()
    if dry_run:
        print(f"[DRY-RUN] Would generate: {workspace / 'project_version.cmake'}")
        return
    cfg = _load_ssot_config(workspace)
    path = _generate_project_version_cmake(workspace, cfg)
    print(f"✅ Generated: {path}")


@task(name="all", default=True, help={"dry_run": "Print target paths without writing files"})
def generate(c: Context, dry_run: bool = False) -> None:
    """Generate partitions.csv, sdkconfig.hardware, and project_version.cmake from SSoT."""
    workspace = Path(getattr(CONFIG.paths, "workspace_dir", ".")).resolve()
    if dry_run:
        print(f"[DRY-RUN] Would generate all build configuration artifacts in: {workspace}")
        return

    cfg = _load_ssot_config(workspace)
    p1 = _generate_partitions_csv(workspace, cfg)
    p2 = _generate_sdkconfig_hardware(workspace, cfg)
    p3 = _generate_project_version_cmake(workspace, cfg)

    print(f"✅ Generated: {p1}")
    print(f"✅ Generated: {p2}")
    print(f"✅ Generated: {p3}")