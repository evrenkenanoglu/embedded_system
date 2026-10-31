"""
@file       build_config.py
@brief      Invoke tasks for SSoT build configuration generators using config_resolver.
@copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
"""

import sys
from pathlib import Path
from invoke import Context, task
from core import CONFIG
from core.config_resolver import resolve_to_file


def _run_generator(
    c: Context, target: str, dry_run: bool, config_override: str = ""
) -> None:
    workspace_root = Path(CONFIG.paths.workspace_dir).resolve()

    # 1. Resolve raw input config path (defaults to configs/config_project.yaml)
    raw_config = (
        Path(config_override)
        if config_override and str(config_override).strip()
        else workspace_root / "configs" / "config_project.yaml"
    )
    if not raw_config.is_absolute():
        raw_config = (workspace_root / raw_config).resolve()

    # 2. Compile into flat, standalone YAML in build/configs/
    resolved_config_file = resolve_to_file(
        raw_config, workspace_root=str(workspace_root)
    )

    # 3. Delegate to the tool's runtime task (zero raw python script paths)
    c.run(
        f'"{sys.executable}" -m invoke tools.build-config.generate '
        f'--config "{resolved_config_file}" '
        f'--workspace "{workspace_root}" '
        f"--target {target}",
        dry=dry_run,
        pty=(sys.platform != "win32"),
    )


@task(
    help={
        "dry_run": "Print execution command without running",
        "config_path": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def partitions(c: Context, dry_run: bool = False, config_path: str = "") -> None:
    """Generate partitions.csv from Master SSoT."""
    _run_generator(c, "partitions", dry_run=dry_run, config_override=config_path)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config_path": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def sdkconfig(c: Context, dry_run: bool = False, config_path: str = "") -> None:
    """Generate sdkconfig.hardware overlay from Master SSoT."""
    _run_generator(c, "sdkconfig", dry_run=dry_run, config_override=config_path)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config_path": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def version(c: Context, dry_run: bool = False, config_path: str = "") -> None:
    """Generate project_version.cmake from Master SSoT."""
    _run_generator(c, "version", dry_run=dry_run, config_override=config_path)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config_path": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def ota_header(c: Context, dry_run: bool = False, config_path: str = "") -> None:
    """Generate main/swConfig/ota_generated_config.h from Master SSoT."""
    _run_generator(c, "ota_header", dry_run=dry_run, config_override=config_path)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config_path": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def time_header(c: Context, dry_run: bool = False, config_path: str = "") -> None:
    """Generate main/swConfig/time_generated_config.h from Master SSoT."""
    _run_generator(c, "time_header", dry_run=dry_run, config_override=config_path)


@task(
    help={
        "dry_run": "Print execution command without running",
        "config_path": "Path to custom project config (defaults to configs/config_project.yaml)",
    }
)
def ble_header(c: Context, dry_run: bool = False, config_path: str = "") -> None:
    """Generate main/swConfig/ble_generated_config.h from Master SSoT."""
    _run_generator(c, "ble_header", dry_run=dry_run, config_override=config_path)


@task(
    name="all",
    default=True,
    help={
        "dry_run": "Print execution command without running",
        "config_path": "Path to custom project config (defaults to configs/config_project.yaml)",
    },
)
def generate(c: Context, dry_run: bool = False, config_path: str = "") -> None:
    """Generate partitions.csv, sdkconfig.hardware, project_version.cmake, ota_generated_config.h, time_generated_config.h, and ble_generated_config.h."""
    _run_generator(c, "all", dry_run=dry_run, config_override=config_path)
