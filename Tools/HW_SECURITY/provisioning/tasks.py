"""
@file       tasks.py
@brief      Standalone Invoke task wrapper for the HW provisioning tool.
@note       Completely independent of task-runner and core.
@note       Sensitive: wraps eFuse burning, flash-encryption and secure-boot key
            operations. Defaults are always the safe virtual mode; hardware mode
            requires explicit opt-in.
"""

import os
import sys
from pathlib import Path
from invoke import Context, task
import yaml

TOOL_DIR = Path(__file__).resolve().parent
if str(TOOL_DIR) not in sys.path:
    sys.path.insert(0, str(TOOL_DIR))

from factory.partition_parser import PartitionTableParser

TOOL_NAME = "provisioning"

VALID_STEPS = ("all", "nvs", "sign", "provision")


def _execution_env() -> dict:
    """Builds env with workspace/embedded_system on PYTHONPATH for main.py imports."""
    tool_dir = Path(__file__).resolve().parent
    embedded_sys_dir = tool_dir.parents[2]
    workspace_root = tool_dir.parents[3]
    task_runner_dir = workspace_root / "Tools" / "task-runner"

    env_paths = [str(workspace_root), str(task_runner_dir), str(embedded_sys_dir)]
    existing = os.environ.get("PYTHONPATH", "")
    if existing and existing not in env_paths:
        env_paths.append(existing)

    return {**os.environ, "PYTHONPATH": os.pathsep.join(env_paths)}


def _resolve_config_path(config: str, tool_dir: Path) -> Path:
    """Resolves a config path relative to the tool directory when needed."""
    config_path = Path(config)
    if not config_path.is_absolute() and not config_path.exists():
        config_path = (tool_dir / config_path).resolve()
    return config_path


@task(
    help={
        "config": "Path to provisioning YAML config (default: config.example.yaml)",
        "step": f"Target step: {', '.join(VALID_STEPS)} (default: all)",
        "target_mode": "Execution target: 'virtual' (default, zero risk) or 'hardware' (physical silicon)",
        "burn_password": "Passphrase required when --target-mode=hardware to burn eFuses",
        "continuous": "Run in automated continuous fixture loop awaiting device insertion",
        "project_root": "Explicit project root directory override",
        "opts": "Forward arbitrary extra flags directly to runner.py",
        "dry_run": "Print execution command without running",
    }
)
def run(
    c: Context,
    config: str = "config.example.yaml",
    step: str = "all",
    target_mode: str = "virtual",
    burn_password: str = "",
    continuous: bool = False,
    project_root: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Execute the provisioning runner (NVS -> Sign -> Provision pipeline)."""
    if step not in VALID_STEPS:
        raise ValueError(
            f"Invalid step '{step}'. Available options: {', '.join(VALID_STEPS)}"
        )

    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "runner.py"
    config_path = _resolve_config_path(config, tool_dir)

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd_parts = [
        f'"{sys.executable}" "{script_path}"',
        f'--config="{config_path}"',
        f"--step {step}",
        f"--target-mode {target_mode}",
    ]
    if burn_password:
        cmd_parts.append(f'--burn-password "{burn_password}"')
    if continuous:
        cmd_parts.append("--continuous")
    if project_root:
        cmd_parts.append(f'--project-root "{project_root}"')
    if dry_run:
        cmd_parts.append("--dry-run")

    cmd = " ".join(cmd_parts)
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"), env=_execution_env())


@task(
    help={
        "config": "Path to provisioning YAML config (default: config.example.yaml)",
        "project_root": "Explicit project root directory override",
        "opts": "Forward arbitrary extra flags directly to runner.py",
        "dry_run": "Print execution command without running",
    }
)
def menu(
    c: Context,
    config: str = "config.example.yaml",
    project_root: str = "",
    opts: str = "",
    dry_run: bool = False,
) -> None:
    """Launch the interactive provisioning selection menu (no step flags)."""
    tool_dir = Path(__file__).resolve().parent
    script_path = tool_dir / "runner.py"
    config_path = _resolve_config_path(config, tool_dir)

    # CRITICAL: Always use sys.executable (never hardcoded 'python')
    cmd = f'"{sys.executable}" "{script_path}" --config="{config_path}"'
    if project_root:
        cmd += f' --project-root "{project_root}"'
    if opts:
        cmd += f" {opts}"

    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"), env=_execution_env())


@task(
    help={
        "config_path": "Path to resolved provisioning YAML config",
        "port": "Serial port override",
        "baud": "Baud rate override",
        "dry_run": "Print execution command without running",
    }
)
def flash_nvs(
    c: Context,
    config_path: str = "config.example.yaml",
    port: str = "",
    baud: str = "",
    dry_run: bool = False,
) -> None:
    """Flash the configured NVS key and factory partitions."""
    config_file = _resolve_config_path(config_path, TOOL_DIR)
    with config_file.open("r", encoding="utf-8") as source:
        config = yaml.safe_load(source)

    paths = config["paths"]
    layout = config["flash_layout"]
    hardware = config["hardware"]
    partitions_csv = Path(paths["partitions_csv"]).resolve()
    output_dir = (
        Path(config["nvs_generation"]["output_key_partition_bin"]).resolve().parent
    )
    parser = PartitionTableParser(
        partitions_csv_path=partitions_csv,
        partition_table_offset=int(str(layout["partition_table_offset"]), 0),
        bootloader_offset=int(str(layout["bootloader_offset"]), 0),
        app_alignment=int(str(layout["app_alignment"]), 0),
        data_alignment=int(str(layout["data_alignment"]), 0),
        partition_table_size=int(str(layout["partition_table_size"]), 0),
    )

    key_entry = parser.get_partition("nvs_keys")
    key_image = output_dir / (
        "nvs_keys_encrypted.bin" if key_entry.is_encrypted else "nvs_keys.bin"
    )
    factory_image = Path(config["nvs_generation"]["output_encrypted_bin"]).resolve()
    for image in (key_image, factory_image):
        if not image.is_file():
            raise FileNotFoundError(
                f"Provisioned NVS image missing: {image}. Run inv es.crypto.provision-nvs first."
            )

    key_offset = parser.get_offset("nvs_keys")
    factory_offset = parser.get_offset("fctry")
    target_port = port or str(hardware.get("port") or hardware.get("default_port", ""))
    target_baud = baud or str(
        hardware.get("baud") or hardware.get("flash_baud", "460800")
    )
    target_chip = str(config["target"]["chip"]).lower()
    cmd = (
        f'"{sys.executable}" -m esptool -p "{target_port}" -b "{target_baud}" '
        f'--chip "{target_chip}" write_flash {hex(key_offset)} "{key_image}" '
        f'{hex(factory_offset)} "{factory_image}"'
    )
    print(
        f"Flashing NVS key at {hex(key_offset)} and factory data at {hex(factory_offset)}"
    )
    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"), env=_execution_env())


@task(
    help={
        "config_path": "Path to resolved provisioning YAML config",
        "dry_run": "Print execution command without running",
    }
)
def verify_nvs(
    c: Context, config_path: str = "config.example.yaml", dry_run: bool = False
) -> None:
    """Verify generated NVS artifacts without device I/O."""
    config_file = _resolve_config_path(config_path, TOOL_DIR)
    verifier_script = TOOL_DIR / "verify_nvs.py"
    cmd = f'"{sys.executable}" "{verifier_script}" --config "{config_file}"'
    c.run(cmd, dry=dry_run, pty=(sys.platform != "win32"), env=_execution_env())
