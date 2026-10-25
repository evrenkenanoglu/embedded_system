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
