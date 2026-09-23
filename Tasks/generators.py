# ==============================================================================
# SSoT Hardware Artifacts Generator
# Domain: Deterministic Generation of partitions.csv and sdkconfig.hardware
# Master SSoT Reference: configs/config_project.yaml
# ==============================================================================

from pathlib import Path
from typing import Any, Dict
import yaml


def generate_partitions_csv(config: Dict[str, Any], output_path: Path) -> None:
    """Generates standard ESP-IDF partitions.csv from SSoT flash_layout."""
    layout = config.get("flash_layout", {})
    partitions = layout.get("partitions", [])

    lines = [
        "# ESP-IDF Partition Table (Auto-generated from configs/config_project.yaml)",
        "# Name, Type, SubType, Offset, Size, Flags",
    ]

    for p in partitions:
        name = p.get("name")
        ptype = p.get("type")
        subtype = p.get("subtype")
        offset = p.get("offset", "")
        size = p.get("size")
        flags = p.get("flags", "")
        lines.append(f"{name}, {ptype}, {subtype}, {offset}, {size}, {flags}".strip())

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"✅ Generated partition table: {output_path}")


def generate_sdkconfig_hardware(config: Dict[str, Any], output_path: Path) -> None:
    """Generates Kconfig hardware overlay from SSoT hardware definitions."""
    hw = config.get("hardware", {})
    flash_size = hw.get("flash_size", "8MB").upper()
    flash_mode = hw.get("flash_mode", "dio").lower()
    flash_freq = hw.get("flash_freq", "80m").lower()

    lines = [
        "# ESP32 Hardware Overlay (Auto-generated from configs/config_project.yaml)",
        f"CONFIG_ESPTOOLPY_FLASHSIZE_{flash_size}=y",
        f'CONFIG_ESPTOOLPY_FLASHSIZE="{flash_size}"',
        f"CONFIG_ESPTOOLPY_FLASHMODE_{flash_mode.upper()}=y",
        f'CONFIG_ESPTOOLPY_FLASHMODE="{flash_mode}"',
        f"CONFIG_ESPTOOLPY_FLASHFREQ_{flash_freq.upper()}=y",
        f'CONFIG_ESPTOOLPY_FLASHFREQ="{flash_freq}"',
    ]

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"✅ Generated Kconfig hardware overlay: {output_path}")


def generate_all(config_project_path: Path, workspace_root: Path) -> None:
    """Loads SSoT project configuration and regenerates all hardware build files."""
    if not config_project_path.exists():
        raise FileNotFoundError(
            f"Master project config not found at: {config_project_path}"
        )

    with open(config_project_path, "r", encoding="utf-8") as f:
        config = yaml.safe_load(f) or {}

    partitions_path = workspace_root / "partitions.csv"
    sdkconfig_hw_path = workspace_root / "sdkconfig.hardware"

    generate_partitions_csv(config, partitions_path)
    generate_sdkconfig_hardware(config, sdkconfig_hw_path)


if __name__ == "__main__":
    workspace = Path(__file__).resolve().parent.parent.parent
    cfg_path = workspace / "configs" / "config_project.yaml"
    generate_all(cfg_path, workspace)
